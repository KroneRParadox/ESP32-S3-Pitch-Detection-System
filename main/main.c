#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h> // Para macros de formato
#include <stdlib.h>   // malloc, free

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ESP-IDF Drivers
#include "driver/i2s.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

// Projeto
#include "def.h"
#include "mic.h"     
#include "filters.h"
#include "yin.h"
#include "tuner.h"
#include "fft.h"
#include "test.h"
#include "utils.h"    // se estiver usando

static const char *TAG = "MAIN";
static const char *TAG_TMIC = "MIC_TASK";
static const char *TAG_TAUD = "AUD_TASK";
static const char *TAG_TCOM = "COM_TASK";

// Estruturas de Dados

typedef struct {
    float  samples[BUFFER_SIZE];
    size_t length;
} raw_block_t;

typedef struct {
    float  samples[BUFFER_SIZE];   // Dados no domínio do tempo
    size_t length;                 // Número de amostras
    float  frequency[FBUF_SIZE];   // Frequências correspondentes aos bins da FFT
    float  magnitude[FBUF_SIZE];   // Magnitudes da FFT
    float  fund_frequency;         // Frequência fundamental detectada
    char   note[16];               // Nota correspondente (ex.: "A4")
} audio_data_t;

// Filas globais
static QueueHandle_t xRawQueue    = NULL; // mic_task -> audio_task
static QueueHandle_t xResultQueue = NULL; // audio_task -> comm_task

// Definição de test_frequencies (se usar)
const float test_frequencies[NUM_TEST_FREQUENCIES] = {
    110.0f, 220.0f, 330.0f, 440.0f,
    550.0f, 660.0f, 770.0f, 880.0f, 990.0f
};

#if TESTE == 1
float phase = 0.0f;
#elif TESTE == 2
float frequencies_waves[NUM_WAVES] = {440.0f, 880.0f, 1320.0f}; 
float amplitudes_waves[NUM_WAVES]  = {1.0f, 0.5f, 0.25f};
float phases_waves[NUM_WAVES]      = {0.0f, 0.0f, 0.0f};
#endif

/** ----------------------------------------------------------------
 *  GPTimer callback -> pisca LED (opcional)
 *  ---------------------------------------------------------------- */
static bool IRAM_ATTR gptimer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    bool *p_led_state = (bool *)user_data;
    if (p_led_state == NULL) {
        return true; // Continuar chamando
    }
    *p_led_state = !(*p_led_state);
    gpio_set_level(LED_GPIO, (*p_led_state) ? 1 : 0);
    return true; // Continuar chamando
}

/** ----------------------------------------------------------------
 *  Configura GPTimer para piscar LED (opcional)
 *  ---------------------------------------------------------------- */
static void configure_led_timer(void)
{
    static bool led_state = false;

    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    gptimer_config_t cfg = {
        .clk_src       = GPTIMER_CLK_SRC_APB,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000 // 1 MHz
    };
    gptimer_handle_t gptimer = NULL;
    esp_err_t err = gptimer_new_timer(&cfg, &gptimer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "GPTimer criado com sucesso.");

    gptimer_alarm_config_t alarm_cfg = {
        .alarm_count              = 500000, // 0.5 segundo
        .reload_count             = 0,
        .flags.auto_reload_on_alarm = true,
    };
    err = gptimer_set_alarm_action(gptimer, &alarm_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar alarme do GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Alarme do GPTimer configurado para %" PRIu64 " ticks.", alarm_cfg.alarm_count);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = gptimer_cb
    };
    err = gptimer_register_event_callbacks(gptimer, &cbs, &led_state);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar callback do GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Callback do GPTimer registrado com sucesso.");

    err = gptimer_enable(gptimer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao habilitar GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "GPTimer habilitado.");

    err = gptimer_start(gptimer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "GPTimer iniciado.");
}

/** ----------------------------------------------------------------
 *  Tarefa: mic_task
 *    - Lê continuamente do I2S
 *    - Envia blocos brutos para xRawQueue
 *    - Prioridade alta, para não perder dados
 *  ---------------------------------------------------------------- */
static void mic_task(void *pv)
{
    while (1)
    {
        TickType_t start_ticks = xTaskGetTickCount();

        // Aloca dinamicamente um bloco
        raw_block_t *blk = (raw_block_t *)malloc(sizeof(raw_block_t));
        if (blk == NULL) {
            ESP_LOGE(TAG_TMIC, "Falha ao alocar raw_block_t (sem memória).");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

    #if TESTE == 0
        // Lê amostras do microfone (I2S)
        blk->length = i2s_read_samples(blk->samples, BUFFER_SIZE);
    #elif TESTE == 1
        // Gera seno (exemplo: 440Hz)
        generate_sine_wave(blk->samples, BUFFER_SIZE, 440.0f, SAMPLE_RATE, &phase);
        blk->length = BUFFER_SIZE;
    #elif TESTE == 2
        // Gera onda composta
        generate_complex_wave(blk->samples, BUFFER_SIZE, SAMPLE_RATE, 
                              frequencies_waves, 
                              amplitudes_waves, 
                              phases_waves, 
                              NUM_WAVES);
        blk->length = BUFFER_SIZE;
    #endif

        if (blk->length > 0)
        {
            // Trunca se for maior que o BUFFER_SIZE (por garantia)
            if (blk->length > BUFFER_SIZE) {
                blk->length = BUFFER_SIZE;
            }

            // Envia o ponteiro para a fila
            if (xQueueSend(xRawQueue, &blk, portMAX_DELAY) != pdTRUE) {
                ESP_LOGE(TAG_TMIC, "Falha ao enviar para xRawQueue.");
                free(blk); // libera se não conseguiu enfileirar
            } else {
                ESP_LOGD(TAG_TMIC, "mic_task: Enviado bloco para xRawQueue.");
            }
        }
        else {
            ESP_LOGW(TAG_TMIC, "mic_task: Nenhuma amostra lida.");
            free(blk);
        }

        TickType_t end_ticks = xTaskGetTickCount();
        float elapsed_ms = (float)(end_ticks - start_ticks) * portTICK_PERIOD_MS;
        ESP_LOGD(TAG_TMIC, "Tempo de captura mic_task: %.2f ms", elapsed_ms); //21 ms a 48 kHz

        // Cede CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/** ----------------------------------------------------------------
 *  Tarefa: audio_task
 *    - Recebe blocos brutos de xRawQueue
 *    - Aplica filtros (ex.: Band-Pass)
 *    - Executa FFT e YIN
 *    - Envia para xResultQueue
 *  ---------------------------------------------------------------- */
static void audio_task(void *pv)
{
    // Inicializa YIN
    Yin yin;
    esp_err_t ret_yin = yin_init(&yin, BUFFER_SIZE, SAMPLE_RATE,
                                 YIN_THRESHOLD, 
                                 YIN_THRESHOLD_ADAPTIVE,
                                 0.1f, 0.2f, 0.01f);
    if (ret_yin != ESP_OK) {
        ESP_LOGE(TAG_TAUD, "Falha ao inicializar YIN.");
        vTaskDelete(NULL);
    }

    // Filtro Band-Pass
    biquad_t bandpass_filter;
    bandpass_init(&bandpass_filter, SAMPLE_RATE, BANDPASS_LOW_FREQ, BANDPASS_HIGH_FREQ);

    smoothing_t smoothing;
    smoothing_init(&smoothing);

    while (1)
    {
        raw_block_t *raw;
        if (xQueueReceive(xRawQueue, &raw, portMAX_DELAY) == pdTRUE)
        {
            TickType_t start_ticks = xTaskGetTickCount();
            ESP_LOGI(TAG_TAUD, "Recebido bloco com %zu samples.", raw->length);

            // Aplica janela (Hann por ex.)
            apply_window(raw->samples, raw->length, 1);

            // Aplica filtro band-pass in-place
            biquad_process(&bandpass_filter, raw->samples, raw->samples, raw->length);

            // FFT
            float breal[FBUF_SIZE];
            float bimg[FBUF_SIZE];
            float mag[FBUF_SIZE];
            for (size_t i = 0; i < FBUF_SIZE; i++) {
                breal[i] = raw->samples[i];
                bimg[i]  = 0.0f;
            }
            fft(breal, bimg, FBUF_SIZE);
            vTaskDelay(pdMS_TO_TICKS(10));
            calculate_magnitude(breal, bimg, mag, FBUF_SIZE);

            // YIN
            float freq_detected = 0.0f;
            if (yin_detect_pitch(&yin, raw->samples, &freq_detected) != 0 || freq_detected < 0.0f) {
                ESP_LOGW(TAG_TAUD, "YIN não detectou pitch válido.");
                freq_detected = -1.0f;
            }
            vTaskDelay(pdMS_TO_TICKS(10));

            // Aloca estrutura de saída
            audio_data_t *out = (audio_data_t *)malloc(sizeof(audio_data_t));
            if (!out) {
                ESP_LOGE(TAG_TAUD, "Falha ao alocar audio_data_t.");
                free(raw); 
                continue;
            }

            // Copia dados de saída
            memcpy(out->samples, raw->samples, raw->length * sizeof(float));
            out->length = raw->length;
            memcpy(out->magnitude, mag, FBUF_SIZE * sizeof(float));

            // Calcula todas as frequências dos bins
            if (frequency(out->magnitude, out->frequency, FBUF_SIZE, SAMPLE_RATE) != 0) {
                ESP_LOGW(TAG_TAUD, "Erro ao calcular frequências (bins).");
            }

            out->fund_frequency = freq_detected; // ou smoothing_update(&smoothing, freq_detected);

            // Determina a nota
            note_t note;
            if (get_note(out->fund_frequency, &note) != 0) {
                strncpy(out->note, "Unknown", sizeof(out->note) - 1);
                out->note[sizeof(out->note) - 1] = '\0';
            } else {
                snprintf(out->note, sizeof(out->note), "%s%d", note.note, note.octave);
            }

            // Envia para xResultQueue
            if (xQueueSend(xResultQueue, &out, portMAX_DELAY) != pdTRUE) {
                ESP_LOGE(TAG_TAUD, "Falha ao enviar para xResultQueue.");
                free(out);
            }

            // Libera o bloco bruto
            free(raw);

            TickType_t end_ticks = xTaskGetTickCount();
            float elapsed_ms = (float)(end_ticks - start_ticks) * portTICK_PERIOD_MS;
            ESP_LOGD(TAG_TAUD, "Tempo process. audio_task: %.2f ms", elapsed_ms);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/** ----------------------------------------------------------------
 *  Tarefa: comm_task
 *    - Recebe audio_data_t de xResultQueue
 *    - Imprime no formato esperado (para Processing/Python)
 *  ---------------------------------------------------------------- */
static void comm_task(void *pv)
{
    while (1)
    {
        audio_data_t *rcv;
        if (xQueueReceive(xResultQueue, &rcv, portMAX_DELAY) == pdTRUE)
        {
            if (!rcv) {
                ESP_LOGE(TAG_TCOM, "comm_task: Ponteiro NULL recebido.");
                continue;
            }

            // 1) Enviar a frequência fundamental e a nota
            printf("FUND_FREQ=%.2fHz NOTE=%s\n", rcv->fund_frequency, rcv->note);

            // 2) Enviar todas as frequências da FFT
            printf("FREQS=");
            for (size_t i = 0; i < FBUF_SIZE; i++) {
                printf("%.2f,", rcv->frequency[i]);
            }
            printf("\n");

            // Libera
            free(rcv);
        }
    }
}

/** ----------------------------------------------------------------
 *  app_main
 *  ---------------------------------------------------------------- */
void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando sistema de detecção de pitch...");

#if TESTE == 1
    // run_all_tests(); // se quiser rodar
#endif

    // 1) LED Timer (opcional)
    configure_led_timer();

    // 2) Inicializa I2S
    ESP_LOGI(TAG, "Inicializando I2S...");
    esp_err_t ret = i2s_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar I2S. Reiniciando...");
        esp_restart();
    }

    // 3) Cria Filas
    xRawQueue    = xQueueCreate(8, sizeof(raw_block_t *));
    xResultQueue = xQueueCreate(8, sizeof(audio_data_t *));
    if (!xRawQueue || !xResultQueue) {
        ESP_LOGE(TAG, "Erro ao criar filas. Reiniciando...");
        esp_restart();
    }

    // 4) Cria tasks
    ESP_LOGI(TAG, "Criando mic_task...");
    xTaskCreatePinnedToCore(mic_task,   "mic_task",   8192,  NULL, 5, NULL, 0);

    ESP_LOGI(TAG, "Criando audio_task...");
    xTaskCreatePinnedToCore(audio_task, "audio_task", 32768, NULL, 4, NULL, 1);

    ESP_LOGI(TAG, "Criando comm_task...");
    xTaskCreatePinnedToCore(comm_task,  "comm_task",  4096,  NULL, 3, NULL, 1);

    // 5) Loop de monitoramento
    while (1) {
        ESP_LOGI(TAG, "Memória heap livre: %ld bytes", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
