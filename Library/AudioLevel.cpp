
#include "AudioLevel.h"
#include "Logging.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mmdevapi.lib")

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

#define CLAMP01(x) max(0.0f, min(1.0f, (x)))

AudioLevel::AudioLevel(const AudioLevelConfig& config)
    : m_config(config)
{
    // Defaults
    for (int i = 0; i < MAX_CHANNELS; i++) {
        m_rms[i] = 0;
        m_peak[i] = 0;
        m_Data.rms[i] = 0;
        m_Data.peak[i] = 0;
        m_fftCfg[i] = nullptr;
        m_fftIn[i] = nullptr;
        m_fftOut[i] = nullptr;
    }
    m_Initialized = false;

    // Validate config
    if (m_config.fftSize < 2) m_config.fftSize = 0;
    if (m_config.fftSize > 0 && m_config.fftSize % 2 != 0) m_config.fftSize++; 
    if (m_config.fftOverlap >= m_config.fftSize) m_config.fftOverlap = m_config.fftSize / 2;

    if (m_config.fftSize > 0) {
        m_fftWindow = new float[m_config.fftSize];
        m_fftTmpIn = new float[m_config.fftSize];
        m_fftTmpOut = new kiss_fft_cpx[m_config.fftSize]; // Allocation size generic
        
        // Hann Window
        for (int i = 0; i < m_config.fftSize; i++) {
             m_fftWindow[i] = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * i / (m_config.fftSize - 1)));
        }

        for (int i = 0; i < MAX_CHANNELS; i++) {
            m_fftCfg[i] = kiss_fftr_alloc(m_config.fftSize, 0, NULL, NULL);
            m_fftIn[i] = new float[m_config.fftSize];
            memset(m_fftIn[i], 0, m_config.fftSize * sizeof(float));
            
            // Output size is (nfft/2 + 1)
            m_fftOut[i] = new float[m_config.fftSize / 2 + 1];
            memset(m_fftOut[i], 0, (m_config.fftSize / 2 + 1) * sizeof(float));
        }
    }

    m_Data.bands.resize(m_config.bands, 0.0f);
    
    // Performance counter for timing
    LARGE_INTEGER pcFreq;
    QueryPerformanceFrequency(&pcFreq);
    m_pcMult = 1000.0 / (double)pcFreq.QuadPart; // ms per tick
    QueryPerformanceCounter(&m_pcPoll);

    if (SUCCEEDED(InitDevice())) {
        m_Initialized = true;
    } else {
        Logging::Log(LogLevel::Error, L"AudioLevel: Failed to initialize audio device.");
    }
}

AudioLevel::~AudioLevel()
{
    Release();
}

void AudioLevel::Release()
{
    if (m_pAudioClient) m_pAudioClient->Stop();
    SAFE_RELEASE(m_pCaptureClient);
    SAFE_RELEASE(m_pAudioClient);
    SAFE_RELEASE(m_pDevice);
    SAFE_RELEASE(m_pEnumerator);
    
    if (m_pwfx) {
        CoTaskMemFree(m_pwfx);
        m_pwfx = nullptr;
    }

    if (m_fftWindow) { delete[] m_fftWindow; m_fftWindow = nullptr; }
    if (m_fftTmpIn) { delete[] m_fftTmpIn; m_fftTmpIn = nullptr; }
    if (m_fftTmpOut) { delete[] m_fftTmpOut; m_fftTmpOut = nullptr; }

    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (m_fftCfg[i]) { free(m_fftCfg[i]); m_fftCfg[i] = nullptr; }
        if (m_fftIn[i]) { delete[] m_fftIn[i]; m_fftIn[i] = nullptr; }
        if (m_fftOut[i]) { delete[] m_fftOut[i]; m_fftOut[i] = nullptr; }
    }
    m_Initialized = false;
}

const AudioLevel::AudioData& AudioLevel::GetStats()
{
    if (m_Initialized) {
        Update();
    }
    return m_Data;
}

HRESULT AudioLevel::InitDevice()
{
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_pEnumerator);
    if (FAILED(hr)) return hr;

    EDataFlow dataFlow = (m_config.port == "Input") ? eCapture : eRender;
    
    if (!m_config.deviceId.empty()) {
        hr = m_pEnumerator->GetDevice(m_config.deviceId.c_str(), &m_pDevice);
    } 
    
    if (FAILED(hr) || !m_pDevice) {
        hr = m_pEnumerator->GetDefaultAudioEndpoint(dataFlow, eMultimedia, &m_pDevice);
    }
    if (FAILED(hr)) return hr;

    hr = m_pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
    if (FAILED(hr)) return hr;

    hr = m_pAudioClient->GetMixFormat(&m_pwfx);
    if (FAILED(hr)) return hr;

    CalculateCoefficients((double)m_pwfx->nSamplesPerSec);

    // Loopback flag only for render devices
    DWORD flags = (dataFlow == eRender) ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;

    hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, m_pwfx, NULL);
    if (FAILED(hr)) return hr;

    hr = m_pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_pCaptureClient);
    if (FAILED(hr)) return hr;

    hr = m_pAudioClient->Start();
    return hr;
}

void AudioLevel::CalculateCoefficients(double sampleRate)
{
    // k = exp(log(0.01) / (SampleRate * TimeSeconds))
    // we use ms in config, so TimeSeconds = ms * 0.001
    
    auto calc = [&](int ms) -> float {
        return (float)exp(log10(0.01) / (sampleRate * (double)ms * 0.001));
    };

    m_kRMS[0] = calc(m_config.rmsAttack);
    m_kRMS[1] = calc(m_config.rmsDecay);
    
    m_kPeak[0] = calc(m_config.peakAttack);
    m_kPeak[1] = calc(m_config.peakDecay);

    if (m_config.fftSize > 0) {
        // FFT Update rate is sampleRate / stride
        double fftRate = sampleRate / (double)(m_config.fftSize - m_config.fftOverlap);
        m_kFFT[0] = (float)exp(log10(0.01) / (fftRate * (double)m_config.fftAttack * 0.001));
        m_kFFT[1] = (float)exp(log10(0.01) / (fftRate * (double)m_config.fftDecay * 0.001));
    }
}

void AudioLevel::ProcessAudio(BYTE* buffer, UINT32 frames)
{
    if (!m_pwfx) return;
    int channels = m_pwfx->nChannels;
    bool isFloat = m_pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || m_pwfx->wBitsPerSample == 32; 

    if (m_pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE* pEx = (WAVEFORMATEXTENSIBLE*)m_pwfx;
        if (IsEqualGUID(pEx->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) isFloat = true;
    }

    float* pFloat = (float*)buffer;
    short* pShort = (short*)buffer;

    for (UINT32 i = 0; i < frames; i++) {
        float sample[MAX_CHANNELS] = {0};

        // Read samples
        for (int c = 0; c < channels; c++) {
            float val = 0;
            if (isFloat) val = *pFloat++;
            else val = (*pShort++) / 32768.0f;

            if (c < MAX_CHANNELS) sample[c] = val;
        }
        // If mono, duplicate to stereo for simplicity
        if (channels == 1) sample[1] = sample[0];

        // RMS & Peak Processing
        for (int c = 0; c < MAX_CHANNELS; c++) {
             float sqr = sample[c] * sample[c];
             float absVal = fabsf(sample[c]);

             // Smoothing match logic: k[0] (attack) if value increasing (sqr > current), else k[1] (decay)
             float kR = (sqr > m_rms[c]) ? m_kRMS[0] : m_kRMS[1];
             
             m_rms[c] = sqr + m_kRMS[(sqr < m_rms[c])] * (m_rms[c] - sqr);
             m_peak[c] = absVal + m_kPeak[(absVal < m_peak[c])] * (m_peak[c] - absVal);
        }

        // FFT Ring Buffer
        if (m_config.fftSize > 0) {
            for (int c = 0; c < MAX_CHANNELS; c++) {
                m_fftIn[c][m_fftBufW] = sample[c];
            }
            m_fftBufW = (m_fftBufW + 1) % m_config.fftSize;

            if (--m_fftBufP <= 0) {
                // Process FFT
                ProduceFFT(0);
                ProduceFFT(1);
                m_fftBufP = m_config.fftSize - m_config.fftOverlap;
            }
        }
    }
    
    // Finalize RMS/Peak Output
    for (int c = 0; c < MAX_CHANNELS; c++) {
        m_Data.rms[c] = CLAMP01(sqrtf(m_rms[c]) * (float)m_config.rmsGain);
        m_Data.peak[c] = CLAMP01(m_peak[c] * (float)m_config.peakGain);
    }
}

void AudioLevel::ProduceFFT(int channel)
{
    // Linearize ring buffer to temp
    int idx = m_fftBufW;
    for (int i = 0; i < m_config.fftSize; i++) {
        m_fftTmpIn[i] = m_fftIn[channel][idx] * m_fftWindow[i];
        idx = (idx + 1) % m_config.fftSize;
    }
    
    kiss_fftr(m_fftCfg[channel], m_fftTmpIn, m_fftTmpOut);

    int outSize = m_config.fftSize / 2 + 1;
    float scalar = 1.0f / sqrtf((float)m_config.fftSize);

    // Update raw frequency bins with smoothing
    for (int i = 0; i < outSize; i++) {
        float mag = sqrtf(m_fftTmpOut[i].r * m_fftTmpOut[i].r + m_fftTmpOut[i].i * m_fftTmpOut[i].i) * scalar;
        
        float& old = m_fftOut[channel][i];
        old = mag + m_kFFT[(mag < old)] * (old - mag);
    }

    if (channel == 0 && m_config.bands > 0) {
         // Clear bands
         std::fill(m_Data.bands.begin(), m_Data.bands.end(), 0.0f);
         
         double sampleRate = (double)m_pwfx->nSamplesPerSec;
         
         for (int i = 0; i < m_config.bands; i++) {
            // Logarithmic mapping
            float f1 = (float)(m_config.freqMin * pow(m_config.freqMax / m_config.freqMin, (double)i / m_config.bands));
            float f2 = (float)(m_config.freqMin * pow(m_config.freqMax / m_config.freqMin, (double)(i + 1) / m_config.bands));
            
            int idx1 = (int)(f1 * 2 * outSize / sampleRate);
            int idx2 = (int)(f2 * 2 * outSize / sampleRate);
            
            if (idx1 < 0) idx1 = 0;
            if (idx2 >= outSize) idx2 = outSize - 1;
            if (idx1 > idx2) idx2 = idx1;

            float maxVal = 0;
            // Mix L+R logic here (using channel 0 and 1)
            for (int k = idx1; k <= idx2; k++) {
                float valL = m_fftOut[0][k];
                float valR = m_fftOut[1][k];
                maxVal = max(maxVal, max(valL, valR)); // Peak of both channels
            }
            
            // Apply sensitivity dB scaling roughly
            // value = 1.0 + 20*log10(val) / sensitivity
             if (maxVal > 0) {
                 float db = 20.0f * log10f(maxVal);
                 m_Data.bands[i] = CLAMP01(1.0f + db / (float)m_config.sensitivity);
             } else {
                 m_Data.bands[i] = 0.0f;
             }
         }
    }
}

void AudioLevel::Update()
{
    if (!m_pCaptureClient) return;

    UINT32 packetLength = 0;
    HRESULT hr = m_pCaptureClient->GetNextPacketSize(&packetLength);

    while (SUCCEEDED(hr) && packetLength > 0)
    {
        BYTE* pData;
        UINT32 numFramesAvailable;
        DWORD flags;

        hr = m_pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
        if (FAILED(hr)) break;

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
             // Decay
             for (int c=0; c<MAX_CHANNELS; c++) {
                 m_rms[c] *= m_kRMS[1];
                 m_peak[c] *= m_kPeak[1];
                 m_Data.rms[c] = 0; 
             }
        } else {
            ProcessAudio(pData, numFramesAvailable);
        }

        m_pCaptureClient->ReleaseBuffer(numFramesAvailable);
        hr = m_pCaptureClient->GetNextPacketSize(&packetLength);
    }
}
