// include/yin.h
#ifndef YIN_H
#define YIN_H

#include "def.h"
#include "utils.h"
#include "esp_err.h"

/**
 * @brief Enumeração para os tipos de threshold disponíveis.
 */
typedef enum {
    YIN_THRESHOLD_FIXED = 0,
    YIN_THRESHOLD_ADAPTIVE
} yin_threshold_mode_t;


/**
 * @brief Estrutura de configuração e estado para o algoritmo YIN.
 */
typedef struct {
    size_t buffer_size;                   // Tamanho do buffer de áudio
    float sample_rate;                    // Taxa de amostragem em Hz
    float threshold;                      // Threshold para detecção de pitch (aplicável no modo fixo)
    float adaptive_threshold_min;         // Threshold mínimo para o modo adaptativo
    float adaptive_threshold_max;         // Threshold máximo para o modo adaptativo
    float adaptive_threshold_step;        // Passo de ajuste para o modo adaptativo
    float current_adaptive_threshold;     // Threshold atual no modo adaptativo
    float *cumulative_difference;         // Buffer para a função de diferença cumulativa
    float *cumulative_mean_difference;    // Buffer para a função de diferença média cumulativa
    size_t tau_min;                       // Lag mínimo para busca de pitch
    size_t tau_max;                       // Lag máximo para busca de pitch
} yin_config_t;

/**
 * @brief Estrutura encapsulada para o algoritmo YIN.
 */
typedef struct {
    yin_config_t config;                  // Configurações e estados do YIN
    yin_threshold_mode_t threshold_mode;  // Modo de threshold (fixo ou adaptativo)
} Yin;

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
esp_err_t yin_init(Yin *yin, size_t buffer_size, float sample_rate, float threshold, yin_threshold_mode_t mode, float adaptive_min, float adaptive_max, float adaptive_step);

/**
 * @brief Executa o algoritmo YIN para detectar a frequência fundamental.
 *
 * @param yin          Ponteiro para a estrutura Yin.
 * @param buffer       Buffer de entrada de amostras de áudio (float).
 * @param frequency    Ponteiro para armazenar a frequência detectada em Hz.
 * @return int          0 se uma frequência foi detectada, -1 caso contrário.
 */
int yin_detect_pitch(Yin *yin, const float *buffer, float *frequency);

/**
 * @brief Libera os recursos alocados pelo algoritmo YIN.
 *
 * @param yin          Ponteiro para a estrutura Yin.
 */
void yin_deinit(Yin *yin);

#endif // YIN_H
