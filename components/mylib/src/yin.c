// components/mylib/src/yin.c

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "yin.h"
#include "esp_log.h"

static const char *TAG = "YIN";

/**
 * @brief Interpolação parabólica para refinar o lag.
 */
static float parabolic_interpolation(const float *d, int tau)
{
    float d0 = d[tau - 1];
    float d1 = d[tau];
    float d2 = d[tau + 1];

    float bottom = 2.0f * d1 - d0 - d2;
    if (fabsf(bottom) < 1e-6f) {
        return (float)tau;
    }
    float delta = (d0 - d2) / (2.0f * bottom);
    return (float)tau + delta;
}

void yin_init(yin_config_t *cfg)
{
    if (cfg == NULL) return;
    cfg->buffer_size = 1024;       // Exemplo de tamanho
    cfg->threshold = 0.1f;         // Exemplo de threshold
    cfg->probability = 0.1f;       // Exemplo de probabilidade mínima
    cfg->min_lag = 40;             // Exemplo: mínimo lag para 50 Hz (48000 / 50 = 960, ajustado conforme buffer)
    cfg->max_lag = 1000;           // Exemplo: máximo lag para 50 Hz
    cfg->enable_interpolate = 1;   // Habilita interpolação parabólica
}

float yin_compute_pitch(const float *samples, size_t N, float sample_rate, const yin_config_t *cfg)
{
    // Limite de tamanho para buffers estáticos
    if (N > 8192) {
        N = 8192;
    }

    // 1) Função de diferença
    float d[8192] = {0.0f};
    for (size_t tau = 1; tau < N; tau++) {
        float sum = 0.0f;
        size_t end = N - tau;
        for (size_t j = 0; j < end; j++) {
            float diff = samples[j] - samples[j + tau];
            sum += diff * diff;
        }
        d[tau] = sum;
    }

    // 2) Função de diferença normalizada cumulativa
    float cmnd[8192] = {1.0f}; // cmnd[0] = 1.0f
    float cumsum = 0.0f;
    for (size_t tau = 1; tau < N; tau++) {
        cumsum += d[tau];
        cmnd[tau] = (d[tau] * tau) / (cumsum > 0.0f ? cumsum : 1.0f);
    }

    // 3) Encontrar o primeiro tau que está abaixo do limiar
    int best_tau = -1;
    size_t min_lag = (cfg->min_lag < 1) ? 1 : cfg->min_lag;
    size_t max_lag = (cfg->max_lag > N - 1) ? (N - 1) : cfg->max_lag;
    for (size_t tau = min_lag; tau <= max_lag; tau++) {
        if (cmnd[tau] < cfg->threshold) {
            best_tau = (int)tau;
            break;
        }
    }
    if (best_tau == -1) {
        ESP_LOGI(TAG, "Nenhuma tau encontrada abaixo do threshold.");
        return 0.0f; // Não encontrou
    }

    // 4) Interpolação parabólica (se habilitada)
    float refined_tau = (float)best_tau;
    if (cfg->enable_interpolate && best_tau > 1 && best_tau < (int)(N - 1)) {
        refined_tau = parabolic_interpolation(cmnd, best_tau);
    }

    // 5) Converter lag para frequência
    float freq = sample_rate / refined_tau;
    return freq;
}
