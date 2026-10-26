#include "AudioAnalyzer.hpp"

constexpr float PI = 3.14159265358979323846f;

AudioAnalyzer::AudioAnalyzer() {
    constexpr size_t N = 512;
    m_hann_window.resize(N);
    for (size_t i = 0; i < N; ++i) {
        // Hann window formula
        m_hann_window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * static_cast<float>(i) / static_cast<float>(N - 1)));
    }
}

void AudioAnalyzer::fft(std::vector<std::complex<float>>& a) {
    const size_t n = a.size();
    if (n <= 1) return;

    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }

    for (size_t len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * PI / static_cast<float>(len);
        std::complex<float> wlen(std::cos(angle), std::sin(angle));

        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<float> u = a[i + j];
                std::complex<float> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

FrequencyBands AudioAnalyzer::analyze(const std::vector<float>& samples) {
    FrequencyBands bands;
    const size_t n = samples.size();
    if (n == 0) return bands;

    // Apply Hann window
    std::vector<std::complex<float>> complex_data(n);
    for (size_t i = 0; i < n; ++i) {
        float window_factor = (i < m_hann_window.size()) ? m_hann_window[i] : 1.0f;
        complex_data[i] = std::complex<float>(samples[i] * window_factor, 0.0f);
    }

    fft(complex_data);

    // Compute spectral magnitudes (Shannon-Nyquist theorem)
    const size_t half_n = n / 2;
    std::vector<float> magnitudes(half_n);
    for (size_t i = 0; i < half_n; ++i) {
        magnitudes[i] = std::abs(complex_data[i]) / static_cast<float>(n);
    }

    // Extract raw band energy (mixing average and max to catch short transients)
    auto compute_band_energy = [&](size_t start_bin, size_t end_bin) -> float {
        float sum = 0.0f;
        float max_val = 0.0f;
        size_t count = 0;

        for (size_t i = start_bin; i <= end_bin && i < half_n; ++i) {
            sum += magnitudes[i];
            max_val = std::max(max_val, magnitudes[i]);
            count++;
        }
        if (count == 0) return 0.0f;

        float avg = sum / static_cast<float>(count);

        // Blend 60% peak and 40% average
        return max_val * 0.6f + avg * 0.4f;
    };

    // Random frequency bands assuming Shannon-Nyquist theorem
    // But the 140 is randomly set (< 512/2 = 256) in order to avoid too high-pitched sounds which are not very relevants for the visualizer
    // - Bass: bins 1 to 4
    // - Mid: bins 5 to 35
    // - Treble: bins 36 to 140 
    float raw_bass = compute_band_energy(1, 4);
    float raw_mid = compute_band_energy(5, 35);
    float raw_treble = compute_band_energy(36, 140);

    // Peaks adapt to track dynamics (ACG)
    constexpr float decay_rate = 0.992f;
    m_peak_bass = std::max(raw_bass, m_peak_bass * decay_rate);
    m_peak_mid = std::max(raw_mid, m_peak_mid * decay_rate);
    m_peak_treble = std::max(raw_treble, m_peak_treble * decay_rate);

    // Prevent divide-by-zero during complete silence
    m_peak_bass = std::max(m_peak_bass, 0.005f);
    m_peak_mid = std::max(m_peak_mid, 0.002f);
    m_peak_treble = std::max(m_peak_treble, 0.001f);

    // Normalize into clean [0.0, 1.0]
    bands.bass = std::clamp(raw_bass / m_peak_bass, 0.0f, 1.0f);
    bands.mid = std::clamp(raw_mid / m_peak_mid, 0.0f, 1.0f);
    bands.treble = std::clamp(raw_treble / m_peak_treble, 0.0f, 1.0f);

    return bands;
}