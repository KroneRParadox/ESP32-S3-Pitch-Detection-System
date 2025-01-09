// src/utils.c
#include "utils.h"
#include "esp_log.h"

static const char *TAG_UTILS = "UTILS";

/**
 * @brief Subtrai elemento a elemento de dois arrays de floats.
 *
 * @param input1 Ponteiro para o primeiro array de entrada.
 * @param input2 Ponteiro para o segundo array de entrada.
 * @param output Ponteiro para o array de saída.
 * @param len    Número de elementos a serem processados.
 */
void sub_vect(const float *input1, const float *input2, float *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        output[i] = input1[i] - input2[i];
    }
}

/**
 * @brief Multiplica elemento a elemento de dois arrays de floats.
 *
 * @param input1 Ponteiro para o primeiro array de entrada.
 * @param input2 Ponteiro para o segundo array de entrada.
 * @param output Ponteiro para o array de saída.
 * @param len    Número de elementos a serem processados.
 */
void mult_vect(const float *input1, const float *input2, float *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        output[i] = input1[i] * input2[i];
    }
}

/**
 * @brief Soma todos os elementos de um array de floats.
 *
 * @param input Ponteiro para o array de entrada.
 * @param len   Número de elementos no array.
 * @return float Soma dos elementos.
 */
float sum_vect(const float *input, size_t len) {
    float sum = 0.0f;
    for (size_t i = 0; i < len; i++) {
        sum += input[i];
    }
    return sum;
}

/**
 * @brief Calcula log2(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (log2(w)).
 * @param length Número de elementos a serem processados.
 */
void log2f_vect(const float *input, float *output, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (input[i] > 0.0f) {
            output[i] = log2f(input[i]);
        } else {
            output[i] = -INFINITY; // Define log2 de zero ou negativo como -infinito
            ESP_LOGW(TAG_UTILS, "Log2f para valor não positivo: input[%zu]=%.2f", i, input[i]);
        }
    }
}

/**
 * @brief Calcula cos(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (cos(w)).
 * @param length Número de elementos a serem processados.
 */
void cos_vect(const float *input, float *output, size_t length) {
    for (size_t i = 0; i < length; i++) {
        output[i] = cosf(input[i]);
    }
}

/**
 * @brief Calcula sin(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (sin(w)).
 * @param length Número de elementos a serem processados.
 */
void sin_vect(const float *input, float *output, size_t length) {
    for (size_t i = 0; i < length; i++) {
        output[i] = sinf(input[i]);
    }
}

/**
 * @brief Adiciona elemento a elemento de dois arrays de floats.
 *
 * @param input1 Ponteiro para o primeiro array de entrada.
 * @param input2 Ponteiro para o segundo array de entrada.
 * @param output Ponteiro para o array de saída.
 * @param len    Número de elementos a serem processados.
 */
void add_vect(const float *input1, const float *input2, float *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        output[i] = input1[i] + input2[i];
    }
}

/**
 * @brief Calcula sqrt(w) para um array de floats.
 *
 * @param input  Ponteiro para o array de entrada (w).
 * @param output Ponteiro para o array de saída (sqrt(w)).
 * @param len    Número de elementos a serem processados.
 */
void sqrt_vect(const float *input, float *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (input[i] >= 0.0f) {
            output[i] = sqrtf(input[i]);
        } else {
            output[i] = NAN; // Define sqrt de valores negativos como NAN
            ESP_LOGW(TAG_UTILS, "Sqrt para valor negativo: input[%zu]=%.2f", i, input[i]);
        }
    }
}

/**
 * @brief Divide elemento a elemento de dois arrays de floats.
 * @param src1 Ponteiro para o primeiro array (numerador).
 * @param src2 Ponteiro para o segundo array (denominador).
 * @param dst Ponteiro para o array de destino onde os resultados serão armazenados.
 * @param size Número de elementos a serem processados.
 */
void div_vect(const float *src1, const float *src2, float *dst, size_t size) {
    if (!src1 || !src2 || !dst) {
        ESP_LOGE(TAG_UTILS, "Ponteiros nulos passados para div_vect.");
        return;
    }

    for (size_t i = 0; i < size; i++) {
        if (fabsf(src2[i]) < 1e-6f) { // Verifica se src2[i] é próximo de zero
            ESP_LOGW(TAG_UTILS, "Divisão por zero detectada no índice %zu. Resultado definido como INFINITY.", i);
            dst[i] = (src1[i] >= 0.0f) ? INFINITY : -INFINITY;
        } else {
            dst[i] = src1[i] / src2[i];
        }
    }
}

/**
 * @brief Gera uma onda senoidal com continuidade de fase.
 *
 * @param buffer      Buffer de saída para a onda senoidal.
 * @param size        Número de amostras.
 * @param frequency   Frequência da onda senoidal em Hz.
 * @param sample_rate Taxa de amostragem em Hz.
 * @param phase       Ponteiro para a variável de fase persistente.
 */
void generate_sine_wave(float *buffer, size_t size, float frequency, float sample_rate, float *phase) {
    if (!buffer || !phase) {
        ESP_LOGE(TAG_UTILS, "Ponteiros nulos passados para generate_sine_wave.");
        return;
    }

    float phase_increment = 2.0f * M_PI * frequency / sample_rate; // Incremento de fase por amostra

    for (size_t i = 0; i < size; i++) {
        buffer[i] = sinf(*phase);
        *phase += phase_increment;
        if (*phase >= 2.0f * M_PI) {
            *phase -= 2.0f * M_PI;
        }
    }
}

/**
 * @brief Adiciona ruído branco ao buffer de amostras.
 *
 * @param buffer    Buffer de amostras.
 * @param size      Número de amostras.
 * @param amplitude Amplitude do ruído.
 */
void add_noise(float *buffer, size_t size, float amplitude) {
    if (!buffer) {
        ESP_LOGE(TAG_UTILS, "Buffer nulo passado para add_noise.");
        return;
    }

    for (size_t i = 0; i < size; i++) {
        buffer[i] += amplitude * ((float)rand() / RAND_MAX * 2.0f - 1.0f); // Adiciona ruído entre -amplitude e +amplitude
    }
}

/**
 * @brief Monitora a utilização do heap.
 */
void log_heap_usage(void) {
    // Utilize heap_caps_print_heap_info se disponível
    // heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    // Como não temos acesso direto, usamos ESP_LOGI com estimativas

    // Note: Para obter informações mais detalhadas, você pode usar funções específicas do ESP-IDF
    // como heap_caps_get_free_size() e heap_caps_get_minimum_free_size()
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t min_free_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    ESP_LOGI(TAG_UTILS, "Heap Livre: %zu bytes | Mínimo Heap Livre: %zu bytes", free_heap, min_free_heap);
}

/**
 * @brief Precomputa a tabela da janela Hann.
 *
 * @param buffer        Buffer de amostras.
 * @param length        Tamanho do buffer.
 * @param window_type   Tipo de janela (1: Hann, 2: Hamming).
 * @param hann_table    Tabela da janela Hann pré-computada (para tipos que utilizam).
 * @param hamming_table Tabela da janela Hamming pré-computada (para tipos que utilizam).
 */
static int precompute_window(float *buffer, size_t length, int window_type) {
    if (!buffer) {
        ESP_LOGE(TAG_UTILS, "Buffer nulo passado para precompute_window.");
        return -1;
    }

    switch (window_type) {
        case 1: { // Janela Hann
            for (size_t i = 0; i < length; i++) {
                buffer[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (length - 1)));
            }
            break;
        }
        case 2: { // Janela Hamming
            for (size_t i = 0; i < length; i++) {
                buffer[i] = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (length - 1));
            }
        }
        default: // Retangular (padrão)
            // Não faz nada, já está multiplicando por 1
            break;
    }
    return 1;
}

/**
 * @brief Aplica uma janela no buffer de entrada.
 *
 * @param buffer        Buffer de amostras.
 * @param length        Tamanho do buffer.
 * @param window_type   Tipo de janela (0: Retangular, 1: Hann, 2: Hamming).
 * @param hann_table    Tabela da janela Hann pré-computada (para tipos que utilizam).
 * @param hamming_table Tabela da janela Hamming pré-computada (para tipos que utilizam).
 */
void apply_window(float *buffer, size_t length, int window_type) {
    if (!buffer) {
        ESP_LOGE(TAG_UTILS, "Buffer nulo passado para apply_window.");
        return;
    }
    float w_table[length];
    int pre_tab = precompute_window(w_table, length, window_type);
    switch (window_type) {
        case 1: { // Janela Hann
            if (pre_tab) {
                mult_vect(buffer, w_table, buffer, length);
            } else {
                ESP_LOGE(TAG_UTILS, "Hann table não fornecida para janela Hann.");
            }
            break;
        }
        case 2: { // Janela Hamming
            if (pre_tab) {
                mult_vect(buffer, w_table, buffer, length);
            } else {
                ESP_LOGW(TAG_UTILS, "Hamming table não fornecida para janela Hamming.");
            }
            break;
        }
        case 0: // Retangular (padrão)
        default:
            // Não faz nada, já está multiplicando por 1
            break;
    }
}

/**
 * @brief Atualiza o filtro de suavização com um novo valor e retorna a média suavizada.
 *
 * @param s         Ponteiro para a estrutura de suavização.
 * @param new_value Novo valor a ser adicionado ao filtro.
 * @return float    Média suavizada.
 */
float smoothing_update_optimized(smoothing_t *s, float new_value) {
    if (!s) return 0.0f;

    // Subtrai o valor antigo da soma
    float old_value = s->buffer[s->index];
    s->sum -= old_value;

    // Adiciona o novo valor ao buffer e à soma
    s->buffer[s->index] = new_value;
    s->sum += new_value;

    // Incrementa o índice circular
    s->index = (s->index + 1) % SMOOTHING_WINDOW_SIZE;

    // Incrementa o contador até atingir o tamanho da janela
    if (s->count < SMOOTHING_WINDOW_SIZE) {
        s->count++;
    }

    // Calcula a média
    return s->sum / (float)s->count;
}

/**
 * @brief Inicializa a estrutura de smoothing.
 * @param s Ponteiro para a estrutura de smoothing.
 */
void smoothing_init(smoothing_t *s) {
    if (!s) {
        ESP_LOGE(TAG_UTILS, "Ponteiro nulo passado para smoothing_init.");
        return;
    }

    memset(s->buffer, 0, sizeof(s->buffer));
    s->index = 0;
    s->count = 0;
    s->sum = 0.0f;
}
