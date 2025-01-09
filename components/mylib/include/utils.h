// include/utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include "def.h"

/**
 * @brief Estrutura para armazenar estado de um filtro BiQuad
 */
typedef struct {
    float buffer[SMOOTHING_WINDOW_SIZE];  // Buffer circular
    size_t index;                          // Índice atual no buffer
    size_t count;                          // Número de valores inseridos
    float sum;                             // Soma dos valores no buffer
} smoothing_t;

/**
 * @brief Gera uma onda senoidal com continuidade de fase.
 *
 * @param buffer      Buffer de saída para a onda senoidal.
 * @param size        Número de amostras.
 * @param frequency   Frequência da onda senoidal em Hz.
 * @param sample_rate Taxa de amostragem em Hz.
 * @param phase       Ponteiro para a variável de fase persistente.
 */
void generate_sine_wave(float *buffer, size_t size, float frequency, float sample_rate, float *phase);

/**
 * @brief Gera uma onda complexa.
 *
 * @param buffer        Buffer de saída para a onda senoidal.
 * @param size          Número de amostras.
 * @param sample_rate   Taxa de amostragem em Hz.
 * @param frequencies   Frequências das ondas Hz.
 * @param amplitudes    Amplitudes das ondas
 * @param phases        Ponteiro para a variável de fase persistente.
 * @param num_waves     numero de ondas juntas.
 */
void generate_complex_wave(float *buffer, size_t size, float sample_rate, float *frequencies, float *amplitudes, float *phases, size_t num_waves);

/**
 * @brief Adiciona ruído branco ao buffer de amostras.
 *
 * @param buffer    Buffer de amostras.
 * @param size      Número de amostras.
 * @param amplitude Amplitude do ruído.
 */
void add_noise(float *buffer, size_t size, float amplitude);

/**
 * @brief Monitora a utilização do heap.
 */
void log_heap_usage(void);

/**
 * @brief Aplica uma janela no buffer de entrada.
 *
 * @param buffer        Buffer de amostras.
 * @param length        Tamanho do buffer.
 * @param window_type   Tipo de janela (0: Retangular, 1: Hann, 2: Hamming).
 * @param hann_table    Tabela da janela Hann pré-computada (para tipos que utilizam).
 * @param hamming_table Tabela da janela Hamming pré-computada (para tipos que utilizam).
 */
void apply_window(float *buffer, size_t length, int window_type);

/**
 * @brief Atualiza o filtro de suavização com um novo valor e retorna a média suavizada.
 *
 * @param s         Ponteiro para a estrutura de suavização.
 * @param new_value Novo valor a ser adicionado ao filtro.
 * @return float    Média suavizada.
 */
float smoothing_update(smoothing_t *s, float new_value);

/**
 * @brief Inicializa a estrutura de smoothing.
 * @param s Ponteiro para a estrutura de smoothing.
 */
void smoothing_init(smoothing_t *s);

/**
 * @brief Subtrai elemento a elemento de dois arrays de floats.
 *
 * @param input1 Ponteiro para o primeiro array de entrada.
 * @param input2 Ponteiro para o segundo array de entrada.
 * @param output Ponteiro para o array de saída.
 * @param len    Número de elementos a serem processados.
 */
void sub_vect(const float *input1, const float *input2, float *output, size_t len);

/**
 * @brief Multiplica elemento a elemento de dois arrays de floats.
 *
 * @param input1 Ponteiro para o primeiro array de entrada.
 * @param input2 Ponteiro para o segundo array de entrada.
 * @param output Ponteiro para o array de saída.
 * @param len    Número de elementos a serem processados.
 */
void mult_vect(const float *input1, const float *input2, float *output, size_t len);

/**
 * @brief Soma todos os elementos de um array de floats.
 *
 * @param input Ponteiro para o array de entrada.
 * @param len   Número de elementos no array.
 * @return float Soma dos elementos.
 */
float sum_vect(const float *input, size_t len);

/**
 * @brief Calcula log2(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (log2(w)).
 * @param length Número de elementos a serem processados.
 */
void log2f_vect(const float *input, float *output, size_t length);

/**
 * @brief Calcula cos(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (cos(w)).
 * @param length Número de elementos a serem processados.
 */
void cos_vect(const float *input, float *output, size_t length);

/**
 * @brief Calcula sin(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (sin(w)).
 * @param length Número de elementos a serem processados.
 */
void sin_vect(const float *input, float *output, size_t length);

/**
 * @brief Adiciona elemento a elemento de dois arrays de floats.
 *
 * @param input1 Ponteiro para o primeiro array de entrada.
 * @param input2 Ponteiro para o segundo array de entrada.
 * @param output Ponteiro para o array de saída.
 * @param len    Número de elementos a serem processados.
 */
void add_vect(const float *input1, const float *input2, float *output, size_t len);

/**
 * @brief Calcula sqrt(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (sqrt(w)).
 * @param len    Número de elementos a serem processados.
 */
void sqrt_vect(const float *input, float *output, size_t len);

/**
 * @brief Divide elemento a elemento de dois arrays de floats.
 * @param src1 Ponteiro para o primeiro array (numerador).
 * @param src2 Ponteiro para o segundo array (denominador).
 * @param dst Ponteiro para o array de destino onde os resultados serão armazenados.
 * @param size Número de elementos a serem processados.
 */
void div_vect(const float *src1, const float *src2, float *dst, size_t size);

#endif // UTILS_H
