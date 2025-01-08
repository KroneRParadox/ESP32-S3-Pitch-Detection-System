// src/filter.c
#include "filters.h"
#include "utils.h"
#include "esp_log.h"

const char *TAG_FILTER = "FILTER";

/**
 * @brief Inicializa os coeficientes para um filtro passa-banda de 2ª ordem.
 *
 * @param f            Ponteiro para a estrutura do filtro biquad.
 * @param sampleRate   Taxa de amostragem em Hz.
 * @param fLow         Frequência de corte inferior em Hz.
 * @param fHigh        Frequência de corte superior em Hz.
 */
void design_bandpass(biquad_t *f, float sampleRate, float fLow, float fHigh) {
    if (!f || sampleRate <= 0 || fLow <= 0 || fHigh <= fLow) {
        ESP_LOGE(TAG_FILTER, "Parâmetros inválidos em design_bandpass.");
        return;
    }

    // Cálculo da frequência central e largura de banda
    float f0 = (fLow + fHigh) / 2.0f;   // Frequência central
    float bandwidth = fHigh - fLow;     // Largura de banda
    float Q = f0 / bandwidth;           // Fator Q

    // Frequência angular normalizada
    float w0 = 2.0f * M_PI * f0 / sampleRate;

    // Cálculos utilizando funções vetoriais personalizadas
    float cos_w0 = 0.0f;
    float sin_w0 = 0.0f;
    cos_vect(&w0, &cos_w0, 1);     // Calcula cos(w0) usando função vetorial
    sin_vect(&w0, &sin_w0, 1);     // Calcula sin(w0) usando função vetorial

    float alpha = sin_w0 / (2.0f * Q);
    float a0 = 1.0f + alpha;

    // Coeficientes do filtro passa-banda
    f->b0 = alpha / a0;
    f->b1 = 0.0f;
    f->b2 = -alpha / a0;
    f->a1 = (-2.0f * cos_w0) / a0;
    f->a2 = (1.0f - alpha) / a0;

#if ENABLE_VERIFICATION
    // Recalcula cos(w0) e sin(w0) para verificação
    float cos_check = 0.0f;
    float sin_check = 0.0f;
    cos_vect(&w0, &cos_check, 1);  // Recalcula cos(w0) para verificação
    sin_vect(&w0, &sin_check, 1);  // Recalcula sin(w0) para verificação

    if (fabsf(cos_w0 - cos_check) > 1e-6 || fabsf(sin_w0 - sin_check) > 1e-6) {
        ESP_LOGW(TAG_FILTER, "Diferença detectada nos cálculos de sin e cos!");
    }
#endif

    // Inicializa os estados
    f->z1 = 0.0f;
    f->z2 = 0.0f;

    ESP_LOGI(TAG_FILTER, "Filtro passa-banda configurado: f0=%.2f Hz, Q=%.2f", f0, Q);
}

/**
 * @brief Inicializa o filtro passa-banda.
 *
 * @param filter       Ponteiro para a estrutura do filtro biquad.
 * @param sampleRate   Taxa de amostragem em Hz.
 * @param fLow         Frequência de corte inferior em Hz.
 * @param fHigh        Frequência de corte superior em Hz.
 */
void bandpass_init(biquad_t *filter, float sampleRate, float fLow, float fHigh) {
    if (!filter) {
        ESP_LOGE(TAG_FILTER, "Filtro nulo passado para bandpass_init.");
        return;
    }

    design_bandpass(filter, sampleRate, fLow, fHigh);
}

/**
 * @brief Aplica um filtro biquad a um buffer de amostras.
 *
 * @param filter    Ponteiro para a estrutura do filtro biquad.
 * @param in        Buffer de entrada de amostras.
 * @param out       Buffer de saída para as amostras filtradas.
 * @param length    Número de amostras a serem processadas.
 */
void biquad_process(biquad_t *filter, const float *in, float *out, size_t length) {
    if (!filter || !in || !out) {
        ESP_LOGE(TAG_FILTER, "Ponteiro nulo passado para biquad_process.");
        return;
    }

    // Coeficientes
    float b0 = filter->b0;
    float b1 = filter->b1;
    float b2 = filter->b2;
    float a1 = filter->a1;
    float a2 = filter->a2;

    // Estados
    float z1 = filter->z1;
    float z2 = filter->z2;

    for (size_t i = 0; i < length; i++) {
        float input = in[i];
        float output_val = b0 * input + z1;
        z1 = b1 * input + z2 - a1 * output_val;
        z2 = b2 * input - a2 * output_val;
        out[i] = output_val;
    }

    // Atualiza os estados
    filter->z1 = z1;
    filter->z2 = z2;
}
