// src/fft.c
#include "fft.h"
#include "def.h"
#include "utils.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG_FFT = "FFT";

/**
 * @brief Rearranjo de bit-reversal.
 * @param real Buffer de partes reais.
 * @param imag Buffer de partes imaginárias.
 * @param n    Número de pontos.
 */
static void bit_reversal(float *real, float *imag, size_t n) { // Marcado como static
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        if (i < j) {
            float tmp_real = real[i];
            float tmp_imag = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = tmp_real;
            imag[j] = tmp_imag;
        }
        size_t m = n >> 1;
        while (j >= m && m > 0) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}

/**
 * @brief Executa FFT (Transformada Rápida de Fourier) in-place (real + imag).
 * @param real Array de floats com parte real
 * @param imag Array de floats com parte imaginária
 * @param n    Tamanho (potência de 2)
 **/
void fft(float *real, float *imag, size_t n) { // Sem 'static'
    if ((n & (n - 1)) != 0) {
        ESP_LOGE(TAG_FFT, "FFT : n não é potência de 2.");
        return;
    }

    bit_reversal(real, imag, n);

    for (size_t s = 1; s < n; s <<= 1) {
        size_t m = s << 1;
        float theta = -M_PI / (float)s;
        float w_real = cosf(theta);
        float w_imag = sinf(theta);

        for (size_t k = 0; k < n; k += m) {
            float wr = 1.0f;
            float wi = 0.0f;
            for (size_t x = 0; x < s; x++) {
                size_t t = k + x + s;

                // Calcula wr * real[t] - wi * imag[t]
                float tr = wr * real[t] - wi * imag[t];

                // Calcula wr * imag[t] + wi * real[t]
                float ti = wr * imag[t] + wi * real[t];

                // Atualiza real[t] e imag[t]
                float new_real_t = real[k + x] - tr;
                float new_imag_t = imag[k + x] - ti;
                real[k + x] = new_real_t;
                imag[k + x] = new_imag_t;

                // Atualiza real[k + x] e imag[k + x]
                real[k + x] += tr;
                imag[k + x] += ti;

                // Atualiza wr e wi
                float tmp_wr_new = wr * w_real - wi * w_imag;
                float tmp_wi_new = wr * w_imag + wi * w_real;
                wr = tmp_wr_new;
                wi = tmp_wi_new;
            }
        }
    }

    ESP_LOGI(TAG_FFT, "FFT concluída.");
}

/**
 * @brief Calcula a magnitude do espectro
 * @param real      Buffer de partes reais.
 * @param imag      Buffer de partes imaginárias.
 * @param magnitude Buffer de saída para as magnitudes.
 * @param length    Número de amostras.
 */
void calculate_magnitude(const float *real, const float *imag, float *magnitude, size_t length) {
    if (!real || !imag || !magnitude) {
        ESP_LOGE(TAG_FFT, "Ponteiros nulos passados para calculate_magnitude.");
        return;
    }

    // Utiliza buffers estáticos para evitar alocações dinâmicas
    // Certifique-se de que BUFFER_SIZE seja grande o suficiente
    static float squared_real_static[BUFFER_SIZE];
    static float squared_imag_static[BUFFER_SIZE];
    static float sum_static[BUFFER_SIZE];

    mult_vect(real, real, squared_real_static, length);
    mult_vect(imag, imag, squared_imag_static, length);
    add_vect(squared_real_static, squared_imag_static, sum_static, length);
    sqrt_vect(sum_static, magnitude, length);
}

/**
 * @brief Encontra a frequência de pico no array 'magnitude'.
 * @param magnitude   Array com magnitudes
 * @param length      número de amostras
 * @param sample_rate Taxa de amostragem em Hz.
 * @return Frequência estimada do pico, ou -1
 */
float peak_frequency(const float *magnitude, size_t length, int sample_rate) {
    if (!magnitude || length == 0) {
        ESP_LOGE(TAG_FFT, "Parâmetros inválidos passados para peak_frequency.");
        return -1.0f;
    }

    float peak_val = 0.0f;
    size_t peak_idx = 0;
    const float MIN_PEAK_THRESHOLD = 0.01f; // Definir conforme necessário

    // Apenas até n/2 para FFT real
    for (size_t i = 1; i < length / 2; i++) {
        if (magnitude[i] > peak_val) {
            peak_val = magnitude[i];
            peak_idx = i;
        }
    }

    if (peak_val < MIN_PEAK_THRESHOLD) {
        return -1.0f; // Nenhum pico significativo detectado
    }

    if (peak_idx == 0) {
        return -1.0f; // Nenhum pico detectado
    }

    return (peak_idx * (float)sample_rate) / (float)length;
}
