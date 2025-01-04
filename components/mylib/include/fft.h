#ifndef FFT_H
#define FFT_H

#include <stddef.h>


/**
 * @brief Aplica a janela de Hamming no buffer
 */
void hamming_window(float *buffer, size_t length);

/**
 * @brief Executa FFT in-place (real + imag).
 * @param real Array de floats com parte real
 * @param imag Array de floats com parte imaginária
 * @param n    Tamanho (potência de 2)
 */
void fft(float *real, float *imag, size_t n);

/**
 * @brief Calcula a magnitude do espectro
 */
void calculate_magnitude(const float *real, const float *imag, float *magnitude, size_t length);

/**
 * @brief Encontra a frequência de pico no array 'magnitude'.
 * @param magnitude   array com magnitudes
 * @param length      length (normalmente n/2)
 * @param sample_rate
 * @return Frequência estimada do pico
 */
float peak_frequency(const float *magnitude, size_t length, int sample_rate);


#endif // FFT_H
