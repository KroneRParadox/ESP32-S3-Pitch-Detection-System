// include/filters.h
#ifndef FILTERS_H
#define FILTERS_H

#include <stddef.h>

/**
 * @brief Estrutura para armazenar estado de um filtro BiQuad
 */
typedef struct {
    // Coeficientes
    float b0, b1, b2;
    float a1, a2;
    // Estado (delays)
    float z1, z2;
} biquad_t;

/**
 * @brief Inicializa um filtro band-pass (2ª ordem) na faixa [fLow, fHigh].
 * @param f            Ponteiro para a estrutura do filtro biquad.
 * @param sampleRate   Taxa de amostragem em Hz.
 * @param fLow         Frequência de corte inferior em Hz.
 * @param fHigh        Frequência de corte superior em Hz.
 */
void design_bandpass(biquad_t *f, float sampleRate, float fLow, float fHigh);

/**
 * @brief Inicializa o filtro passa-banda.
 * @param filter       Ponteiro para a estrutura do filtro biquad.
 * @param sampleRate   Taxa de amostragem em Hz.
 * @param fLow         Frequência de corte inferior em Hz.
 * @param fHigh        Frequência de corte superior em Hz.
 */
void bandpass_init(biquad_t *filter, float sampleRate, float fLow, float fHigh);

/**
 * @brief Aplica o filtro BiQuad a um buffer de amostras.
 * @param filter    Ponteiro para biquad_t
 * @param in        Buffer de entrada
 * @param out       Buffer de saída
 * @param length    Número de amostras a serem processadas.
 */
void biquad_process(biquad_t *filter, const float *in, float *out, size_t length);

#endif // FILTERS_H
