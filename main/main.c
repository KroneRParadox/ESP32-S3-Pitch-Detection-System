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
    float  *samples;   // Dados no domínio do tempo
    size_t length;                 // Número de amostras
    float  *frequency;   // Frequências correspondentes aos bins da FFT
    float  *magnitude;   // Magnitudes da FFT
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
float frequencies_waves[NUM_WAVES] = {27.5f, 28.0f}; 
float amplitudes_waves[NUM_WAVES]  = {1.0f, 0.5f};
float phases_waves[NUM_WAVES]      = {0.0f, 0.0f};
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
        // Gera seno
        generate_sine_wave(blk->samples, BUFFER_SIZE, 220.0f, SAMPLE_RATE, &phase);
        blk->length = BUFFER_SIZE;
    #elif TESTE == 2
        // Gera onda composta
        generate_complex_wave(blk->samples, BUFFER_SIZE, SAMPLE_RATE, frequencies_waves, amplitudes_waves, phases_waves, NUM_WAVES);
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
                                 0.02f, 0.1f, 0.01f);
    if (ret_yin != ESP_OK) {
        ESP_LOGE(TAG_TAUD, "Falha ao inicializar YIN.");
        vTaskDelete(NULL);
    }

    // Filtro Band-Pass
    biquad_t bandpass_filter;
    bandpass_init(&bandpass_filter, SAMPLE_RATE, LOW_FREQ, HIGH_FREQ);

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
            float *breal  = heap_caps_malloc(FBUF_SIZE * sizeof(float), MALLOC_CAP_SPIRAM);
            float *bimg   = heap_caps_malloc(FBUF_SIZE * sizeof(float), MALLOC_CAP_SPIRAM);
            float *mag    = heap_caps_malloc(FBUF_SIZE * sizeof(float), MALLOC_CAP_SPIRAM);
            
            if (!breal || !bimg || !mag) {
                ESP_LOGE(TAG_TAUD, "Falha ao alocar breal/bimg/mag.");
                free(raw);
                if(breal) heap_caps_free(breal);
                if(bimg)  heap_caps_free(bimg);
                if(mag)   heap_caps_free(mag);
                continue;
            }   

            for (size_t i = 0; i < FBUF_SIZE; i++) {
                breal[i] = raw->samples[i];
                bimg[i]  = 0.0f;
            }

            fft(breal, bimg, FBUF_SIZE);
            calculate_magnitude(breal, bimg, mag, FBUF_SIZE);

            // Calcula todas as frequências dos bins
            if (frequency(mag, breal, FBUF_SIZE, SAMPLE_RATE) != 0) {
                ESP_LOGW(TAG_TAUD, "Erro ao calcular frequências (bins).");
            }

            // YIN
            float freq_detected = 0.0f;
            if (yin_detect_pitch(&yin, raw->samples, &freq_detected) != 0 || freq_detected < 0.0f) {
                ESP_LOGW(TAG_TAUD, "YIN não detectou pitch válido.");
                freq_detected = -1.0f;
            }

            // Aloca estrutura de saída
            audio_data_t *out = (audio_data_t *)malloc(sizeof(audio_data_t));
            if (!out) {
                ESP_LOGE(TAG_TAUD, "Falha ao alocar audio_data_t.");
                free(raw);
                heap_caps_free(breal);
                heap_caps_free(bimg);
                heap_caps_free(mag); 
                continue;
            }

            float *clone_samples = heap_caps_malloc(raw->length * sizeof(float), MALLOC_CAP_SPIRAM);
            if (!clone_samples) {
                // Falha
                free(out);
                free(raw);
                heap_caps_free(breal);
                heap_caps_free(bimg);
                heap_caps_free(mag);
                continue;
            }

            memcpy(clone_samples, raw->samples, raw->length*sizeof(float));

            out->samples    = clone_samples;
            out->length     = raw->length;
            out->frequency  = breal;  // cede “a posse” do breal
            out->magnitude  = mag;    // cede “a posse” do mag
            out->fund_frequency = freq_detected;// ou smoothing_update(&smoothing, freq_detected);

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
                heap_caps_free(out->samples);
                heap_caps_free(out->frequency);
                heap_caps_free(out->magnitude);
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
 *    - Imprime no formato esperado
 *    - Fun_Freq;Note;Samples;Magnitude\n 
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

            /* 2) Enviar todas as frequências da FFT
            printf("FREQS=");
            for (size_t i = 0; i < FBUF_SIZE; i++) {
                printf("%.2f,", rcv->frequency[i]);
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            printf("\n");
            printf("MAGN=");
            for (size_t i = 0; i < FBUF_SIZE; i++) {
                printf("%.2f,", rcv->magnitude[i]);
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            printf("\n");
            
                        // 1) Enviar a frequência fundamental e a nota
            printf("%.2f;%s;", rcv->fund_frequency, rcv->note);

            // 2) Enviar as samples e a magnitude
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                printf("%.2f;", rcv->samples[i]);
                if(i > FBUF_SIZE)
                    printf("%.2f", rcv->magnitude[FBUF_SIZE]);
                printf("%.2f", rcv->magnitude[FBUF_SIZE]);
            }
            printf("\n");
            */

            // Libera
            heap_caps_free(rcv->samples);
            heap_caps_free(rcv->frequency);
            heap_caps_free(rcv->magnitude);
            free(rcv);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

/** ----------------------------------------------------------------
 *  app_main
 *  ---------------------------------------------------------------- */
void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando sistema de detecção de pitch...");

    // 1) LED Timer (opcional)
    configure_led_timer();

    // 2) Inicializa I2S
    ESP_LOGI(TAG, "Inicializando I2S...");
    esp_err_t ret = i2s_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar I2S. Reiniciando...");
        esp_restart();
    }
    #if TESTE == 0
    printf("Microfone;");
    #elif TESTE == 1
    printf("Teste com onda simples;");
    #elif TESTE == 2
    printf("Teste com onda composta;");
    #endif

    // 3) Cria Filas
    xRawQueue    = xQueueCreate(8, sizeof(raw_block_t *));
    xResultQueue = xQueueCreate(8, sizeof(audio_data_t *));
    if (!xRawQueue || !xResultQueue) {
        ESP_LOGE(TAG, "Erro ao criar filas. Reiniciando...");
        esp_restart();
    }

    // 4) Cria tasks
    ESP_LOGI(TAG, "Criando mic_task...");
    xTaskCreatePinnedToCore(mic_task,   "mic_task",   1 << 13,  NULL, 5, NULL, 0);

    ESP_LOGI(TAG, "Criando audio_task...");
    xTaskCreatePinnedToCore(audio_task, "audio_task", 1 << 15, NULL, 4, NULL, 1);

    ESP_LOGI(TAG, "Criando comm_task...");
    xTaskCreatePinnedToCore(comm_task,  "comm_task",  1 << 12,  NULL, 3, NULL, 1);

    // 5) Loop de monitoramento
    while (1) {
        ESP_LOGI(TAG, "Memória heap livre: %ld bytes", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
