/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <NovadeskAPI/novadesk_addon.h>

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <objbase.h>

#include "third_party/kiss_fft130/kiss_fftr.h"

const NovadeskHostAPI *g_Host = nullptr;

namespace
{
    struct AudioLevelStats
    {
        float rms[2] = {0.0f, 0.0f};
        float peak[2] = {0.0f, 0.0f};
        std::vector<float> bands;
    };

    struct AudioLevelConfig
    {
        std::string port = "output"; // "output" or "input"
        std::wstring deviceId;

        int fftSize = 1024;
        int fftOverlap = 512;
        int bands = 10;

        double freqMin = 20.0;
        double freqMax = 20000.0;
        double sensitivity = 35.0;

        int rmsAttack = 300;
        int rmsDecay = 300;
        int peakAttack = 50;
        int peakDecay = 2500;
        int fftAttack = 300;
        int fftDecay = 300;

        double rmsGain = 1.0;
        double peakGain = 1.0;
    };

    static float Clamp01(float v)
    {
        if (v < 0.0f)
            return 0.0f;
        if (v > 1.0f)
            return 1.0f;
        return v;
    }

    static std::wstring Utf8ToWide(const char *s)
    {
        if (!s || !*s)
            return L"";
        const int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (len <= 1)
            return L"";
        std::wstring out(static_cast<size_t>(len - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), len);
        return out;
    }

    class AudioLevelAnalyzer
    {
    public:
        ~AudioLevelAnalyzer() { Release(); }

        bool GetStats(AudioLevelStats &outStats, const AudioLevelConfig &config)
        {
            if (!EnsureInitialized(config))
            {
                return false;
            }

            PollCapture();

            outStats.rms[0] = Clamp01(m_rms[0] * static_cast<float>(m_config.rmsGain));
            outStats.rms[1] = Clamp01(m_rms[1] * static_cast<float>(m_config.rmsGain));
            outStats.peak[0] = Clamp01(m_peak[0] * static_cast<float>(m_config.peakGain));
            outStats.peak[1] = Clamp01(m_peak[1] * static_cast<float>(m_config.peakGain));
            outStats.bands = BuildBands(m_config.bands);
            return true;
        }

    private:
        AudioLevelConfig m_config;

        bool m_comInit = false;
        IMMDeviceEnumerator *m_enumerator = nullptr;
        IMMDevice *m_device = nullptr;
        IAudioClient *m_audioClient = nullptr;
        IAudioCaptureClient *m_captureClient = nullptr;
        WAVEFORMATEX *m_pwfx = nullptr;

        int m_channels = 2;
        int m_sampleRate = 48000;
        bool m_initialized = false;
        int m_fftSize = 1024;
        int m_fftOverlap = 512;
        int m_fftStride = 512;

        kiss_fftr_cfg m_fftCfg = nullptr;
        std::vector<float> m_ring;
        int m_ringWrite = 0;
        int m_ringFilled = 0;
        int m_fftCountdown = 512;

        std::vector<float> m_fftWindow;
        std::vector<float> m_fftIn;
        std::vector<kiss_fft_cpx> m_fftOut;
        std::vector<float> m_spectrum;

        float m_rms[2] = {0.0f, 0.0f};
        float m_peak[2] = {0.0f, 0.0f};
        float m_kRMS[2] = {0.0f, 0.0f};
        float m_kPeak[2] = {0.0f, 0.0f};
        float m_kFFT[2] = {0.0f, 0.0f};

        bool SameConfig(const AudioLevelConfig &c) const
        {
            return m_config.port == c.port &&
                   m_config.deviceId == c.deviceId &&
                   m_config.fftSize == c.fftSize &&
                   m_config.fftOverlap == c.fftOverlap &&
                   m_config.bands == c.bands &&
                   m_config.freqMin == c.freqMin &&
                   m_config.freqMax == c.freqMax &&
                   m_config.sensitivity == c.sensitivity &&
                   m_config.rmsAttack == c.rmsAttack &&
                   m_config.rmsDecay == c.rmsDecay &&
                   m_config.peakAttack == c.peakAttack &&
                   m_config.peakDecay == c.peakDecay &&
                   m_config.fftAttack == c.fftAttack &&
                   m_config.fftDecay == c.fftDecay &&
                   m_config.rmsGain == c.rmsGain &&
                   m_config.peakGain == c.peakGain;
        }

        int NormalizeFFTSize(int n) const
        {
            if (n < 2)
                n = 1024;
            if (n % 2 != 0)
                ++n;
            return n;
        }

        int ClampMs(int ms) const { return (ms <= 0) ? 1 : ms; }

        float CalcSmoothCoeff(int ms) const
        {
            const double sr = static_cast<double>(m_sampleRate <= 0 ? 48000 : m_sampleRate);
            const double t = static_cast<double>(ClampMs(ms)) * 0.001;
            return static_cast<float>(std::exp(std::log10(0.01) / (sr * t)));
        }

        void RecalculateCoeffs()
        {
            m_kRMS[0] = CalcSmoothCoeff(m_config.rmsAttack);
            m_kRMS[1] = CalcSmoothCoeff(m_config.rmsDecay);
            m_kPeak[0] = CalcSmoothCoeff(m_config.peakAttack);
            m_kPeak[1] = CalcSmoothCoeff(m_config.peakDecay);

            const double fftRate = static_cast<double>(m_sampleRate <= 0 ? 48000 : m_sampleRate) /
                                   static_cast<double>(m_fftStride <= 0 ? 1 : m_fftStride);
            const auto fftCoeff = [&](int ms) -> float
            {
                const double t = static_cast<double>(ClampMs(ms)) * 0.001;
                return static_cast<float>(std::exp(std::log10(0.01) / (fftRate * t)));
            };
            m_kFFT[0] = fftCoeff(m_config.fftAttack);
            m_kFFT[1] = fftCoeff(m_config.fftDecay);
        }

        bool EnsureInitialized(const AudioLevelConfig &config)
        {
            if (m_initialized && SameConfig(config))
                return true;
            if (m_initialized)
                Release();
            m_config = config;

            const HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (hrCom == RPC_E_CHANGED_MODE)
            {
                m_comInit = false;
            }
            else if (SUCCEEDED(hrCom))
            {
                m_comInit = true;
            }
            else
            {
                return false;
            }

            m_fftSize = NormalizeFFTSize(m_config.fftSize);
            m_fftOverlap = m_config.fftOverlap;
            if (m_fftOverlap < 0)
                m_fftOverlap = m_fftSize / 2;
            if (m_fftOverlap >= m_fftSize)
                m_fftOverlap = m_fftSize / 2;
            m_fftStride = m_fftSize - m_fftOverlap;
            if (m_fftStride <= 0)
                m_fftStride = m_fftSize / 2;
            m_fftCountdown = m_fftStride;

            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&m_enumerator));
            if (FAILED(hr) || !m_enumerator)
                return false;

            EDataFlow dataFlow = (m_config.port == "input") ? eCapture : eRender;
            if (!m_config.deviceId.empty())
                hr = m_enumerator->GetDevice(m_config.deviceId.c_str(), &m_device);
            else
                hr = m_enumerator->GetDefaultAudioEndpoint(dataFlow, eMultimedia, &m_device);
            if (FAILED(hr) || !m_device)
                return false;

            hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&m_audioClient));
            if (FAILED(hr) || !m_audioClient)
                return false;

            hr = m_audioClient->GetMixFormat(&m_pwfx);
            if (FAILED(hr) || !m_pwfx)
                return false;

            m_sampleRate = static_cast<int>(m_pwfx->nSamplesPerSec);
            m_channels = static_cast<int>(m_pwfx->nChannels);
            if (m_channels < 1)
                m_channels = 1;
            if (m_channels > 2)
                m_channels = 2;

            DWORD flags = (dataFlow == eRender) ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
            hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, m_pwfx, nullptr);
            if (FAILED(hr))
                return false;

            hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void **>(&m_captureClient));
            if (FAILED(hr) || !m_captureClient)
                return false;

            hr = m_audioClient->Start();
            if (FAILED(hr))
                return false;

            m_fftCfg = kiss_fftr_alloc(m_fftSize, 0, nullptr, nullptr);
            if (!m_fftCfg)
                return false;

            m_ring.assign(static_cast<size_t>(m_fftSize), 0.0f);
            m_fftWindow.resize(static_cast<size_t>(m_fftSize));
            m_fftIn.resize(static_cast<size_t>(m_fftSize));
            m_fftOut.resize(static_cast<size_t>(m_fftSize));
            m_spectrum.assign(static_cast<size_t>(m_fftSize / 2 + 1), 0.0f);

            for (int i = 0; i < m_fftSize; ++i)
            {
                m_fftWindow[static_cast<size_t>(i)] = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * i / static_cast<float>(m_fftSize - 1)));
            }

            RecalculateCoeffs();
            m_initialized = true;
            return true;
        }

        void Release()
        {
            if (m_audioClient)
                m_audioClient->Stop();
            if (m_captureClient)
            {
                m_captureClient->Release();
                m_captureClient = nullptr;
            }
            if (m_audioClient)
            {
                m_audioClient->Release();
                m_audioClient = nullptr;
            }
            if (m_device)
            {
                m_device->Release();
                m_device = nullptr;
            }
            if (m_enumerator)
            {
                m_enumerator->Release();
                m_enumerator = nullptr;
            }
            if (m_pwfx)
            {
                CoTaskMemFree(m_pwfx);
                m_pwfx = nullptr;
            }
            if (m_fftCfg)
            {
                free(m_fftCfg);
                m_fftCfg = nullptr;
            }
            m_initialized = false;
            if (m_comInit)
            {
                CoUninitialize();
                m_comInit = false;
            }
        }

        void PushMonoSample(float s)
        {
            m_ring[static_cast<size_t>(m_ringWrite)] = s;
            m_ringWrite = (m_ringWrite + 1) % m_fftSize;
            if (m_ringFilled < m_fftSize)
                ++m_ringFilled;
            if (--m_fftCountdown <= 0)
            {
                m_fftCountdown = m_fftStride;
                ComputeFFT();
            }
        }

        void ComputeFFT()
        {
            if (m_ringFilled < m_fftSize)
                return;
            int idx = m_ringWrite;
            for (int i = 0; i < m_fftSize; ++i)
            {
                m_fftIn[static_cast<size_t>(i)] = m_ring[static_cast<size_t>(idx)] * m_fftWindow[static_cast<size_t>(i)];
                idx = (idx + 1) % m_fftSize;
            }
            kiss_fftr(m_fftCfg, m_fftIn.data(), m_fftOut.data());
            const int outSize = m_fftSize / 2 + 1;
            const float scalar = 1.0f / std::sqrt(static_cast<float>(m_fftSize));
            for (int i = 0; i < outSize; ++i)
            {
                const float re = m_fftOut[static_cast<size_t>(i)].r;
                const float im = m_fftOut[static_cast<size_t>(i)].i;
                const float mag = std::sqrt(re * re + im * im) * scalar;
                float &old = m_spectrum[static_cast<size_t>(i)];
                old = mag + m_kFFT[(mag < old)] * (old - mag);
            }
        }

        std::vector<float> BuildBands(int bands) const
        {
            std::vector<float> out(static_cast<size_t>(bands), 0.0f);
            if (m_spectrum.empty())
                return out;
            const int outSize = static_cast<int>(m_spectrum.size());
            const double freqMin = m_config.freqMin <= 0.0 ? 20.0 : m_config.freqMin;
            const double freqMax = (m_config.freqMax <= freqMin) ? 20000.0 : m_config.freqMax;
            const double sensitivity = (m_config.sensitivity <= 0.0) ? 35.0 : m_config.sensitivity;

            for (int i = 0; i < bands; ++i)
            {
                const double f1 = freqMin * std::pow(freqMax / freqMin, static_cast<double>(i) / static_cast<double>(bands));
                const double f2 = freqMin * std::pow(freqMax / freqMin, static_cast<double>(i + 1) / static_cast<double>(bands));
                int idx1 = static_cast<int>(f1 * 2.0 * outSize / static_cast<double>(m_sampleRate));
                int idx2 = static_cast<int>(f2 * 2.0 * outSize / static_cast<double>(m_sampleRate));
                if (idx1 < 0)
                    idx1 = 0;
                if (idx2 >= outSize)
                    idx2 = outSize - 1;
                if (idx2 < idx1)
                    idx2 = idx1;
                float maxVal = 0.0f;
                for (int k = idx1; k <= idx2; ++k)
                    maxVal = std::max(maxVal, m_spectrum[static_cast<size_t>(k)]);
                if (maxVal > 0.0f)
                {
                    const float db = 20.0f * std::log10(maxVal);
                    out[static_cast<size_t>(i)] = Clamp01(1.0f + db / static_cast<float>(sensitivity));
                }
            }
            return out;
        }

        void PollCapture()
        {
            if (!m_captureClient || !m_pwfx)
                return;
            UINT32 packetLength = 0;
            HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);

            bool hadAudio = false;
            float sumSq[2] = {0.0f, 0.0f};
            float peak[2] = {0.0f, 0.0f};
            uint64_t frameCount = 0;

            while (SUCCEEDED(hr) && packetLength > 0)
            {
                BYTE *data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                hr = m_captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
                if (FAILED(hr))
                    break;

                const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
                const bool isFloat = (m_pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) || (m_pwfx->wBitsPerSample == 32);

                if (!silent && data && frames > 0)
                {
                    hadAudio = true;
                    if (isFloat)
                    {
                        const float *in = reinterpret_cast<const float *>(data);
                        for (UINT32 i = 0; i < frames; ++i)
                        {
                            const float l = in[i * m_pwfx->nChannels + 0];
                            const float r = (m_pwfx->nChannels > 1) ? in[i * m_pwfx->nChannels + 1] : l;
                            sumSq[0] += l * l;
                            sumSq[1] += r * r;
                            peak[0] = std::max(peak[0], std::fabs(l));
                            peak[1] = std::max(peak[1], std::fabs(r));
                            PushMonoSample((l + r) * 0.5f);
                        }
                    }
                    else
                    {
                        const int16_t *in = reinterpret_cast<const int16_t *>(data);
                        for (UINT32 i = 0; i < frames; ++i)
                        {
                            const float l = static_cast<float>(in[i * m_pwfx->nChannels + 0]) / 32768.0f;
                            const float r = (m_pwfx->nChannels > 1)
                                                ? static_cast<float>(in[i * m_pwfx->nChannels + 1]) / 32768.0f
                                                : l;
                            sumSq[0] += l * l;
                            sumSq[1] += r * r;
                            peak[0] = std::max(peak[0], std::fabs(l));
                            peak[1] = std::max(peak[1], std::fabs(r));
                            PushMonoSample((l + r) * 0.5f);
                        }
                    }
                    frameCount += frames;
                }

                m_captureClient->ReleaseBuffer(frames);
                hr = m_captureClient->GetNextPacketSize(&packetLength);
            }

            if (hadAudio && frameCount > 0)
            {
                const float n = static_cast<float>(frameCount);
                const float rmsNow0 = std::sqrt(sumSq[0] / n);
                const float rmsNow1 = std::sqrt(sumSq[1] / n);
                m_rms[0] = Clamp01(rmsNow0 + m_kRMS[(rmsNow0 < m_rms[0])] * (m_rms[0] - rmsNow0));
                m_rms[1] = Clamp01(rmsNow1 + m_kRMS[(rmsNow1 < m_rms[1])] * (m_rms[1] - rmsNow1));
                m_peak[0] = Clamp01(peak[0] + m_kPeak[(peak[0] < m_peak[0])] * (m_peak[0] - peak[0]));
                m_peak[1] = Clamp01(peak[1] + m_kPeak[(peak[1] < m_peak[1])] * (m_peak[1] - peak[1]));
            }
            else
            {
                m_rms[0] *= m_kRMS[1];
                m_rms[1] *= m_kRMS[1];
                m_peak[0] *= m_kPeak[1];
                m_peak[1] *= m_kPeak[1];
            }
        }
    };

    AudioLevelAnalyzer g_audioLevelAnalyzer;

    int JsAudioLevelStats(novadesk_context ctx)
    {
        AudioLevelConfig cfg;
        const int top = g_Host->GetTop(ctx);
        const bool hasOptionsObject = (top > 0 && g_Host->IsObject(ctx, 0));

        auto popIfPushed = [&](int beforeTop)
        {
            if (g_Host->GetTop(ctx) > beforeTop)
                g_Host->Pop(ctx);
        };

        auto readPropString = [&](const char *name, std::string &dst)
        {
            const int before = g_Host->GetTop(ctx);
            g_Host->GetProperty(ctx, 0, name);
            if (g_Host->GetTop(ctx) > before && g_Host->IsString(ctx, -1))
            {
                const char *s = g_Host->GetString(ctx, -1);
                if (s)
                    dst = s;
            }
            popIfPushed(before);
        };

        auto readPropWString = [&](const char *name, std::wstring &dst)
        {
            const int before = g_Host->GetTop(ctx);
            g_Host->GetProperty(ctx, 0, name);
            if (g_Host->GetTop(ctx) > before && g_Host->IsString(ctx, -1))
            {
                const char *s = g_Host->GetString(ctx, -1);
                if (s)
                    dst = Utf8ToWide(s);
            }
            popIfPushed(before);
        };

        auto readPropInt = [&](const char *name, int &dst)
        {
            const int before = g_Host->GetTop(ctx);
            g_Host->GetProperty(ctx, 0, name);
            if (g_Host->GetTop(ctx) > before && g_Host->IsNumber(ctx, -1))
                dst = static_cast<int>(g_Host->GetNumber(ctx, -1));
            popIfPushed(before);
        };

        auto readPropDouble = [&](const char *name, double &dst)
        {
            const int before = g_Host->GetTop(ctx);
            g_Host->GetProperty(ctx, 0, name);
            if (g_Host->GetTop(ctx) > before && g_Host->IsNumber(ctx, -1))
                dst = g_Host->GetNumber(ctx, -1);
            popIfPushed(before);
        };

        if (hasOptionsObject)
        {
            readPropString("port", cfg.port);
            readPropWString("deviceId", cfg.deviceId);
            readPropInt("fftSize", cfg.fftSize);
            readPropInt("fftOverlap", cfg.fftOverlap);
            readPropInt("bands", cfg.bands);
            readPropDouble("freqMin", cfg.freqMin);
            readPropDouble("freqMax", cfg.freqMax);
            readPropDouble("sensitivity", cfg.sensitivity);
            readPropInt("rmsAttack", cfg.rmsAttack);
            readPropInt("rmsDecay", cfg.rmsDecay);
            readPropInt("peakAttack", cfg.peakAttack);
            readPropInt("peakDecay", cfg.peakDecay);
            readPropInt("fftAttack", cfg.fftAttack);
            readPropInt("fftDecay", cfg.fftDecay);
            readPropDouble("rmsGain", cfg.rmsGain);
            readPropDouble("peakGain", cfg.peakGain);
        }

        AudioLevelStats stats;
        if (!g_audioLevelAnalyzer.GetStats(stats, cfg))
        {
            g_Host->PushNull(ctx);
            return 1;
        }

        g_Host->PushObject(ctx);

        double rmsVals[2] = {stats.rms[0], stats.rms[1]};
        g_Host->RegisterArrayNumber(ctx, "rms", rmsVals, 2);

        double peakVals[2] = {stats.peak[0], stats.peak[1]};
        g_Host->RegisterArrayNumber(ctx, "peak", peakVals, 2);

        std::vector<double> bandVals;
        bandVals.reserve(stats.bands.size());
        for (float v : stats.bands)
            bandVals.push_back(static_cast<double>(v));
        g_Host->RegisterArrayNumber(ctx, "bands", bandVals.data(), bandVals.size());

        return 1;
    }
}

NOVADESK_ADDON_INIT(ctx, hMsgWnd, host)
{
    (void)hMsgWnd;
    g_Host = host;

    novadesk::Addon addon(ctx, host);
    addon.RegisterString("name", "AudioLevel");
    addon.RegisterString("version", "1.0.0");

    addon.RegisterFunction("stats", JsAudioLevelStats, 1);
}

NOVADESK_ADDON_UNLOAD()
{
}
