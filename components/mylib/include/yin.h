#ifndef YIN_H
#define YIN_H


/**
 * @brief Configurações para o algoritmo YIN
 */
typedef struct {
    size_t buffer_size;
    float threshold;
    float probability;
    size_t min_lag;            // Lag mínimo para considerar
    size_t max_lag;            // Lag máximo para considerar
    int enable_interpolate;    // Flag para habilitar interpolação parabólica (1: habilitado, 0: desabilitado)
} yin_config_t;

/**
 * @brief Valores padrão de config do YIN.
 *
 * @param N tamanho do buffer
 */
#define YIN_DEFAULT_CONFIG(N) (yin_config_t){ \
    .threshold          = 0.1f,              \
    .min_lag            = 2,                 \
    .max_lag            = (N)/2,             \
    .enable_interpolate = 1,                 \
}
// Inicializa o YIN com uma configuração padrão
void yin_init(yin_config_t *cfg);

/**
 * @brief  Executa o algoritmo YIN no buffer 'samples' para encontrar a frequência fundamental.
 *
 * @param samples      Buffer de áudio
 * @param N            Tamanho do buffer
 * @param sample_rate  Taxa de amostragem
 * @param cfg          Configurações do YIN
 * @return Frequência fundamental estimada, ou 0.0f se não encontrado.
 */
float yin_compute_pitch(const float *samples, size_t N, float sample_rate, const yin_config_t *cfg);

#endif // YIN_H
