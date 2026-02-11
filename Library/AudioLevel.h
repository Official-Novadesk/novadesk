
#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys.h>
#include "kiss_fft130/kiss_fftr.h"

// Constants
#define MAX_CHANNELS 2

struct AudioLevelConfig {
    std::string port = "output"; // "output" or "input"
    std::wstring deviceId = L"";
    
    int fftSize = 1024;
    int fftOverlap = 512;
    int bands = 10;
    
    double freqMin = 20.0;
    double freqMax = 20000.0;
    double sensitivity = 35.0;
    
    // Smoothing (Attack/Decay in ms)
    int rmsAttack = 300;
    int rmsDecay = 300;
    int peakAttack = 50;
    int peakDecay = 2500;
    int fftAttack = 300;
    int fftDecay = 300;

    double rmsGain = 1.0;
    double peakGain = 1.0;
};

class AudioLevel
{
public:
    struct AudioData {
        float rms[MAX_CHANNELS];
        float peak[MAX_CHANNELS];
        std::vector<float> bands;
    };

    AudioLevel(const AudioLevelConfig& config);
    ~AudioLevel();

    const AudioData& GetStats();

private:
    HRESULT InitDevice();
    void ProcessAudio(BYTE* buffer, UINT32 frames);
    void ProduceFFT(int channel);
    void Update(); 
    void Release();
    void CalculateCoefficients(double sampleRate);

    AudioLevelConfig m_config;

    // Audio Client
    IMMDeviceEnumerator* m_pEnumerator = nullptr;
    IMMDevice* m_pDevice = nullptr;
    IAudioClient* m_pAudioClient = nullptr;
    IAudioCaptureClient* m_pCaptureClient = nullptr;
    WAVEFORMATEX* m_pwfx = nullptr;

    // FFT State
    kiss_fftr_cfg m_fftCfg[MAX_CHANNELS];
    float* m_fftIn[MAX_CHANNELS];     // Ring buffer for input samples
    float* m_fftOut[MAX_CHANNELS];    // Output raw FFT
    float* m_fftWindow = nullptr;     // Window function
    float* m_fftTmpIn = nullptr;      // Temp buffer for FFT calc
    kiss_fft_cpx* m_fftTmpOut = nullptr; // Temp buffer for complex output

    int m_fftBufW = 0; // Write pointer for ring buffer
    int m_fftBufP = 0; // Process counter (overlap logic)

    // Data State
    AudioData m_Data;
    bool m_Initialized = false;
    
    // Internal State
    float m_rms[MAX_CHANNELS];
    float m_peak[MAX_CHANNELS];
    
    // Calculated Smoothing factors
    float m_kRMS[2];
    float m_kPeak[2];
    float m_kFFT[2];

    // Timing
    LARGE_INTEGER m_pcPoll;
    double m_pcMult;
};
