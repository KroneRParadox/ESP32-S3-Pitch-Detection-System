// include/fft.h
#ifndef FFT_H
#define FFT_H

#include <stddef.h>

/**
 * @brief Executa FFT (Transformada Rápida de Fourier) in-place (real + imag).
 * @param real Array de floats com parte real
 * @param imag Array de floats com parte imaginária
 * @param n    Tamanho (potência de 2)
 */
void fft(float *real, float *imag, size_t n);

/**
 * @brief Calcula a magnitude do espectro
 * @param real      Buffer de partes reais.
 * @param imag      Buffer de partes imaginárias.
 * @param magnitude Buffer de saída para as magnitudes.
 * @param length    Número de amostras.
 */
void calculate_magnitude(const float *real, const float *imag, float *magnitude, size_t length);

/**
 * @brief Calcula as frequências correspondentes a cada bin de magnitude da FFT.
 * @param magnitude    Array com magnitudes da FFT.
 * @param frequency    Buffer onde serão armazenadas as frequências calculadas.
 * @param length       Número de amostras no array de magnitudes.
 * @param sample_rate  Taxa de amostragem em Hz.
 * @return 0 em sucesso, -1 em erro.
 */
float frequency(const float *magnitude, float *frequency, size_t length, int sample_rate);
#endif // FFT_H
