// components/mylib/src/filters.c

#include <math.h>
#include "filters.h"
#include "esp_log.h"

#define PI 3.14159265358979323846

/*
 * Exemplo simples de band-pass: 
 *   - Calcula dois filtros passa-altas e passa-baixas de 2ª ordem e "combina", 
 *     ou
 *   - Usa fórmula de band-pass pico em freq central = sqrt(fLow*fHigh).
 * 
 * Abaixo, implemento uma abordagem mais direta para band-pass 2ª ordem 
 * (pico em sqrt(fLow*fHigh)), com Q razoável.
 * Observação: para maior precisão, convém usar bibliotecas de design de filtros 
 * ou gerar coeficientes via engenharia/tabela.
 */

/**
 * @brief Calcula os coeficientes para um filtro passa-banda de 2ª ordem.
 *
 * @param f            Ponteiro para a estrutura do filtro biquad.
 * @param sampleRate   Taxa de amostragem em Hz.
 * @param fLow         Frequência de corte inferior em Hz.
 * @param fHigh        Frequência de corte superior em Hz.
 */
static void design_bandpass(biquad_t *f, float sampleRate, float fLow, float fHigh)
{
    if (f == NULL || sampleRate <= 0.0f || fLow <= 0.0f || fHigh <= 0.0f || fLow >= fHigh) {
        ESP_LOGE("FILTERS", "Parâmetros inválidos em design_bandpass.");
        return;
    }

    // Frequência central (geometric mean) e largura de banda
    float wLow  = 2.0f * PI * fLow  / sampleRate; // rad/sample
    float wHigh = 2.0f * PI * fHigh / sampleRate; // rad/sample
    float w0    = sqrtf(wLow * wHigh);            // rad/sample
    float bw    = wHigh - wLow;                   // rad/sample

    // Cálculo de alpha com base na largura de banda
    float alpha = sinf(w0) * sinhf(logf(2.0f)/2.0f * bw / sinf(w0));
    float cos_w0 = cosf(w0);

    // Fórmula do filtro bandpass "peak gain = Q"
    // b0 =   alpha
    // b1 =   0
    // b2 =  -alpha
    // a0 =   1 + alpha
    // a1 =  -2 cos(w0)
    // a2 =   1 - alpha
    // Normaliza b0,b1,b2 dividindo por a0
    float a0 = 1.0f + alpha;
    f->b0 =  alpha / a0;
    f->b1 =  0.0f;
    f->b2 = -alpha / a0;
    f->a1 = -2.0f * cos_w0 / a0;
    f->a2 = (1.0f - alpha) / a0;

    // Inicializa estados
    f->z1 = 0.0f;
    f->z2 = 0.0f;

    ESP_LOGI("FILTERS", "Filtro passa-banda inicializado: fLow=%.2fHz, fHigh=%.2fHz", fLow, fHigh);
}

/**
 * @brief Inicializa o filtro passa-banda.
 *
 * @param filter       Ponteiro para a estrutura do filtro biquad.
 * @param sampleRate   Taxa de amostragem em Hz.
 * @param fLow         Frequência de corte inferior em Hz.
 * @param fHigh        Frequência de corte superior em Hz.
 */
void bandpass_init(biquad_t *filter, float sampleRate, float fLow, float fHigh)
{
    if (filter == NULL) {
        ESP_LOGE("FILTERS", "Filtro nulo passado para bandpass_init.");
        return;
    }

    design_bandpass(filter, sampleRate, fLow, fHigh);
}

/**
 * @brief Aplica um filtro biquad a um buffer de amostras usando a Forma Direta Transposta II.
 *
 * @param filter    Ponteiro para a estrutura do filtro biquad.
 * @param in        Buffer de entrada de amostras.
 * @param out       Buffer de saída para as amostras filtradas.
 * @param length    Número de amostras a serem processadas.
 */
void biquad_process(biquad_t *filter, const float *in, float *out, size_t length)
{
    if (filter == NULL || in == NULL || out == NULL) {
        ESP_LOGE("FILTERS", "Ponteiro nulo passado para biquad_process.");
        return;
    }

    float b0 = filter->b0;
    float b1 = filter->b1;
    float b2 = filter->b2;
    float a1 = filter->a1;
    float a2 = filter->a2;

    float z1 = filter->z1;
    float z2 = filter->z2;

    for (size_t i = 0; i < length; i++) {
        float input = in[i];
        // Transposed Direct Form II
        float out_temp = input * b0 + z1;
        z1 = input * b1 + z2 - a1 * out_temp;
        z2 = input * b2 - a2 * out_temp;
        out[i] = out_temp;
    }

    filter->z1 = z1;
    filter->z2 = z2;
}
