// src/yin.c
#include "yin.h"
#include "utils.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG_YIN = "YIN";

/**
 * @brief Inicializa a configuração do algoritmo YIN.
 *
 * @param yin          Ponteiro para a estrutura Yin.
 * @param buffer_size  Tamanho do buffer de áudio.
 * @param sample_rate  Taxa de amostragem em Hz.
 * @param threshold    Threshold para detecção de pitch.
 * @param mode         Modo de threshold (fixo ou adaptativo).
 * @param adaptive_min Threshold mínimo para adaptativo.
 * @param adaptive_max Threshold máximo para adaptativo.
 * @param adaptive_step Passo de ajuste para adaptativo.
 * @return esp_err_t   ESP_OK em sucesso, ou código de erro correspondente.
 */
esp_err_t yin_init(Yin *yin, size_t buffer_size, float sample_rate, float threshold, yin_threshold_mode_t mode, float adaptive_min, float adaptive_max, float adaptive_step) {
    if (!yin || buffer_size == 0 || sample_rate <= 0.0f) {
        ESP_LOGE(TAG_YIN, "Parâmetros inválidos passados para yin_init.");
        return ESP_ERR_INVALID_ARG;
    }

    // Configurações principais
    yin->config.buffer_size = buffer_size;
    yin->config.sample_rate = sample_rate;
    yin->config.threshold = threshold;
    yin->config.adaptive_threshold_min = adaptive_min;
    yin->config.adaptive_threshold_max = adaptive_max;
    yin->config.adaptive_threshold_step = adaptive_step;
    yin->config.current_adaptive_threshold = adaptive_max; // Inicia com o valor máximo

    // Inicializa tau_min e tau_max (ajuste conforme necessário)
    yin->config.tau_min = (size_t)(sample_rate / HIGH_FREQ); // Frequência máxima de 4186 Hz
    yin->config.tau_max = (size_t)(sample_rate / LOW_FREQ);   // Frequência mínima de 27.5 Hz

    // Aloca memória para os buffers
    yin->config.cumulative_difference = (float *)heap_caps_malloc(buffer_size * sizeof(float), MALLOC_CAP_8BIT);
    yin->config.cumulative_mean_difference = (float *)heap_caps_malloc(buffer_size * sizeof(float), MALLOC_CAP_8BIT);

    if (!yin->config.cumulative_difference || !yin->config.cumulative_mean_difference) {
        ESP_LOGE(TAG_YIN, "Falha na alocação de memória para buffers YIN.");
        if (yin->config.cumulative_difference) {
            heap_caps_free(yin->config.cumulative_difference);
            yin->config.cumulative_difference = NULL;
        }
        if (yin->config.cumulative_mean_difference) {
            heap_caps_free(yin->config.cumulative_mean_difference);
            yin->config.cumulative_mean_difference = NULL;
        }
        return ESP_ERR_NO_MEM;
    }

    // Inicializa os buffers
    memset(yin->config.cumulative_difference, 0, buffer_size * sizeof(float));
    memset(yin->config.cumulative_mean_difference, 0, buffer_size * sizeof(float));

    // Define o modo de threshold
    yin->threshold_mode = mode;

    ESP_LOGI(TAG_YIN, "YIN inicializado com buffer_size=%zu, sample_rate=%.2f Hz, threshold=%.2f, mode=%s",
             buffer_size, sample_rate, threshold,
             mode == YIN_THRESHOLD_FIXED ? "Fixo" : "Adaptativo");
    return ESP_OK;
}

/**
 * @brief Executa o algoritmo YIN para detectar a frequência fundamental.
 *
 * @param yin          Ponteiro para a estrutura Yin.
 * @param buffer       Buffer de entrada de amostras de áudio (float).
 * @param frequency    Ponteiro para armazenar a frequência detectada em Hz.
 * @return int          0 se uma frequência foi detectada, -1 caso contrário.
 */
int yin_detect_pitch(Yin *yin, const float *buffer, float *frequency) {
    if (!yin || !buffer || !frequency) {
        ESP_LOGE(TAG_YIN, "Ponteiros nulos passados para yin_detect_pitch.");
        return -1;
    }

    size_t n = yin->config.buffer_size;
    size_t tau_min = yin->config.tau_min;
    size_t tau_max = yin->config.tau_max;

    // Inicializar os buffers para diferenças
    for (size_t i = tau_min; i <= tau_max; i++) {
        yin->config.cumulative_difference[i] = 0.0f;
    }

    // Passo 1 e 2 no mesmo loop
    float running_sum = 0.0f;
    yin->config.cumulative_mean_difference[tau_min] = 0.0f;

    // Parâmetros da heurística
    const size_t  YIELD_INTERVAL         = 5;       // Intervalo para ceder CPU

    for (size_t tau = tau_min; tau <= tau_max; tau++) {
        // 1) diferença cumulativa (combina sub, mult, sum)
        float sum = 0.0f;
        size_t len = n - tau;
        for (size_t j=0; j < len; j++) {
            float diff = buffer[j] - buffer[j + tau];
            sum += diff * diff;
            //vTaskDelay(pdMS_TO_TICKS(5));
        }
        yin->config.cumulative_difference[tau] = sum;

        // 2) média cumulativa
        if (tau == tau_min) {
            running_sum = sum; // start
        } else {
            running_sum += sum;
        }
        yin->config.cumulative_mean_difference[tau] = running_sum;


        // ceder CPU esporadicamente sem travar
        if ((tau % YIELD_INTERVAL) == 0) {
            taskYIELD();
        }
    }

    // Passo 3: Identificação da primeira tau onde d(tau)/mean(d(tau)) < threshold
    size_t tau_found = tau_max + 1; // Indica que não foi encontrado
    float used_threshold = (yin->threshold_mode == YIN_THRESHOLD_ADAPTIVE) ? yin->config.current_adaptive_threshold : yin->config.threshold;
    for (size_t tau = tau_min; tau <= tau_max; tau++) {
        float cum_mean = yin->config.cumulative_mean_difference[tau];
        if (cum_mean == 0.0f) continue;
        float norm_diff = ((float)tau * yin->config.cumulative_difference[tau]) / cum_mean;
        if (norm_diff < used_threshold) {
            tau_found = tau;
            break;
        }
    }

    if (tau_found > tau_max) {
        // Nenhuma frequência detectada
        *frequency = -1.0f;
        ESP_LOGD(TAG_YIN, "Aqui");

        // Ajusta o threshold adaptativo para torná-lo menos sensível na próxima iteração
        if (yin->threshold_mode == YIN_THRESHOLD_ADAPTIVE) {
            yin->config.current_adaptive_threshold += yin->config.adaptive_threshold_step;
            if (yin->config.current_adaptive_threshold > yin->config.adaptive_threshold_max) {
                yin->config.current_adaptive_threshold = yin->config.adaptive_threshold_max;
            }
        }

        return -1;
    }

    // Passo 4: Interpolação parabólica para refinar a estimativa de tau
    if (tau_found + 1 > tau_max || tau_found < tau_min + 1) {
        // Sem pontos suficientes para interpolação
        *frequency = yin->config.sample_rate / (float)tau_found;
        return 0;
    }

    float d0 = yin->config.cumulative_difference[tau_found - 1];
    float d1 = yin->config.cumulative_difference[tau_found];
    float d2 = yin->config.cumulative_difference[tau_found + 1];

    // Verifica se a diferença para interpolação é válida
    if ((2.0f * d1 - d2 - d0) == 0.0f) {
        // Evita divisão por zero na interpolação
        *frequency = yin->config.sample_rate / (float)tau_found;
        return 0;
    }

    // Fórmula de interpolação parabólica
    float better_tau = tau_found + (d2 - d0) / (2.0f * (2.0f * d1 - d2 - d0));

    // Calcula a frequência fundamental
    *frequency = yin->config.sample_rate / better_tau;

    // Atualiza o threshold adaptativo, se estiver habilitado
    if (yin->threshold_mode == YIN_THRESHOLD_ADAPTIVE) {
        // Reduz o threshold para aumentar a sensibilidade
        yin->config.current_adaptive_threshold -= yin->config.adaptive_threshold_step;
        if (yin->config.current_adaptive_threshold < yin->config.adaptive_threshold_min) {
            yin->config.current_adaptive_threshold = yin->config.adaptive_threshold_min;
        }
    }

    return 0;
}

/**
 * @brief Libera os recursos alocados pelo algoritmo YIN.
 *
 * @param yin          Ponteiro para a estrutura Yin.
 */
void yin_deinit(Yin *yin) {
    if (!yin) {
        ESP_LOGE(TAG_YIN, "Ponteiro nulo passado para yin_deinit.");
        return;
    }

    if (yin->config.cumulative_difference) {
        heap_caps_free(yin->config.cumulative_difference);
        yin->config.cumulative_difference = NULL;
    }

    if (yin->config.cumulative_mean_difference) {
        heap_caps_free(yin->config.cumulative_mean_difference);
        yin->config.cumulative_mean_difference = NULL;
    }

    ESP_LOGI(TAG_YIN, "YIN desinicializado e recursos liberados.");
    return;
}
