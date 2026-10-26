#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>

// Naive repartition for now
struct FrequencyBands {
    float bass = 0.0f; // 20 Hz - 250 Hz
    float mid = 0.0f; // 250 Hz - 4 kHz
    float treble = 0.0f; // 4 kHz - 16 kHz
};

class AudioAnalyzer {
public:
    AudioAnalyzer();

    // Analyze raw audio samples and return balanced normalized bands [0.0, 1.0]
    FrequencyBands analyze(const std::vector<float>& samples);

private:
    std::vector<float> m_hann_window;

    // Fast Cooley-Tukey FFT implementation ( O(n.log2(n)) )
    void fft(std::vector<std::complex<float>>& a);

    // Adaptive peak tracking for AGC
    float m_peak_bass = 0.05f;
    float m_peak_mid = 0.02f;
    float m_peak_treble = 0.005f;
};