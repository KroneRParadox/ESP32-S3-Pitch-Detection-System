// main/src/main.c

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h> // Adicionado para usar macros de formato

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Drivers do ESP-IDF
#include "driver/i2s.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

// Cabeçalhos do Projeto
#include "var.h"       // Definições de pinos, buffer, sample rate etc.
#include "mic.h"       // i2s_init(), i2s_read_samples()
#include "filters.h"   // biquad_t, bandpass_init(), biquad_process()
#include "yin.h"       // yin_compute_pitch(), yin_config_t
#include "tuner.h"     // get_note()

/** ----------------------------------------------------------------
 *  Estruturas e Filas
 *  ----------------------------------------------------------------
 *  mic_task -> xRawQueue -> audio_task -> xResultQueue -> comm_task
 */

// Bloco bruto de amostras (mic_task -> audio_task)
typedef struct {
    float  samples[BUFFER_SIZE];
    size_t length;
} raw_block_t;

// Bloco de resultado (audio_task -> comm_task)
typedef struct {
    float  samples[BUFFER_SIZE];
    size_t length;
    float  frequency;
    char   note[8];   // ex.: "A4"
} audio_data_t;

// Filas (queues) globais
static QueueHandle_t xRawQueue    = NULL; // mic_task -> audio_task
static QueueHandle_t xResultQueue = NULL; // audio_task -> comm_task
static const char *TAG = "MAIN";

/** ----------------------------------------------------------------
 *  GPTimer callback -> pisca LED (opcional)
 *  ---------------------------------------------------------------- */
static bool IRAM_ATTR gptimer_cb(gptimer_handle_t timer,
                                 const gptimer_alarm_event_data_t *edata,
                                 void *user_data)
{
    bool *p_led_state = (bool *)user_data;
    if (p_led_state == NULL) {
        ESP_LOGE(TAG, "LED state pointer is NULL in gptimer_cb.");
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

    // Configura o pino do LED como saída
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // Configuração do GPTimer
    gptimer_config_t cfg = {
        .clk_src       = GPTIMER_CLK_SRC_APB,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000 // 1 MHz (1 microsegundo por tick)
    };
    gptimer_handle_t gptimer = NULL;
    esp_err_t err = gptimer_new_timer(&cfg, &gptimer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "GPTimer criado com sucesso.");

    // Configuração do alarme para piscar o LED a cada segundo
    gptimer_alarm_config_t alarm_cfg = {
        .alarm_count              = 1000000, // 1 segundo
        .reload_count             = 0,
        .flags.auto_reload_on_alarm = true,
    };
    err = gptimer_set_alarm_action(gptimer, &alarm_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar alarme do GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Alarme do GPTimer configurado para %" PRIu64 " ticks.", alarm_cfg.alarm_count);

    // Registro do callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = gptimer_cb
    };
    err = gptimer_register_event_callbacks(gptimer, &cbs, &led_state);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar callback do GPTimer: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Callback do GPTimer registrado com sucesso.");

    // Habilita e inicia o timer
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
 *    - Lê continuamente do I2S (INMP441 ou similar)
 *    - Envia blocos brutos para xRawQueue
 *    - Prioridade alta, para não perder dados
 *  ---------------------------------------------------------------- */
static void mic_task(void *pv)
{
    while (1)
    {
        raw_block_t blk;
        // Lê amostras do microfone
        blk.length = i2s_read_samples(blk.samples, BUFFER_SIZE);
        // (Opcional) Debug local:
        // printf("mic_task: lidos %zu samples.\n", blk.length);

        if (blk.length > 0) {
            // Garante que não excede BUFFER_SIZE
            if (blk.length > BUFFER_SIZE) {
                blk.length = BUFFER_SIZE;
            }
            // Envia para a fila (bloqueia se estiver cheia)
            if (xQueueSend(xRawQueue, &blk, portMAX_DELAY) != pdTRUE) {
                ESP_LOGE(TAG, "Falha ao enviar para xRawQueue.");
            } else {
                ESP_LOGD(TAG, "mic_task: Enviado bloco com %zu samples para xRawQueue.", blk.length);
            }
        } else {
            ESP_LOGW(TAG, "mic_task: Nenhuma amostra lida.");
        }

        // Pequeno delay para ceder CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/** ----------------------------------------------------------------
 *  Tarefa: audio_task
 *    - Recebe blocos brutos de xRawQueue
 *    - Aplica filtros (ex.: Band-Pass)
 *    - Executa YIN para obter frequência e nota
 *    - Envia para xResultQueue
 *  ---------------------------------------------------------------- */
static void audio_task(void *pv)
{
    // Configuração do YIN
    yin_config_t yin_cfg = YIN_DEFAULT_CONFIG(BUFFER_SIZE);

    // Inicializa filtro Band-Pass (opcional)
    biquad_t bandpass_filter;
    // Exemplo: passa banda entre 80 Hz e 1000 Hz
    bandpass_init(&bandpass_filter, SAMPLE_RATE, 80.0f, 1000.0f);
    ESP_LOGI(TAG, "Filtro Band-Pass inicializado entre %.2f Hz e %.2f Hz.", 80.0f, 1000.0f);

    while (1)
    {
        raw_block_t raw;
        // Recebe bloco do mic_task
        if (xQueueReceive(xRawQueue, &raw, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGD(TAG, "audio_task: Recebido bloco com %zu samples.", raw.length);

            // Aplica filtro Band-Pass (opcional)
            float filtered_samples[BUFFER_SIZE];
            biquad_process(&bandpass_filter, raw.samples, filtered_samples, raw.length);
            ESP_LOGD(TAG, "audio_task: Filtro Band-Pass aplicado.");

            // Executa YIN no buffer filtrado
            float freq = yin_compute_pitch(filtered_samples, raw.length, SAMPLE_RATE, &yin_cfg);
            audio_data_t out;
            out.length = raw.length;
            memcpy(out.samples, filtered_samples, raw.length * sizeof(float));
            out.frequency = freq;

            // Chama get_note com o buffer de saída correto
            const char *note_str = get_note(freq, out.note, sizeof(out.note));

            if (note_str) {
                ESP_LOGI(TAG, "audio_task: Frequência detectada: %.2f Hz, Nota: %s", freq, note_str);
            } else {
                strcpy(out.note, "Unknown");
                ESP_LOGW(TAG, "audio_task: Nota desconhecida para a frequência %.2f Hz.", freq);
            }

            // Envia para xResultQueue
            if (xQueueSend(xResultQueue, &out, portMAX_DELAY) != pdTRUE) {
                ESP_LOGE(TAG, "Falha ao enviar para xResultQueue.");
            } else {
                ESP_LOGD(TAG, "audio_task: Enviado resultado para xResultQueue.");
            }
        }

        // Cede CPU
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/** ----------------------------------------------------------------
 *  Tarefa: comm_task
 *    - Recebe audio_data_t de xResultQueue
 *    - Imprime no formato que o script Python/Processing espera:
 *        FREQ=xxxHz NOTE=xxx
 *        SAMPLE=...
 *        ...
 *        END
 *  ---------------------------------------------------------------- */
static void comm_task(void *pv)
{
    audio_data_t rcv;
    while (1)
    {
        if (xQueueReceive(xResultQueue, &rcv, portMAX_DELAY) == pdTRUE)
        {
            // Imprime no formato que o Python/Processing espera
            printf("FREQ=%.2fHz NOTE=%s\n", rcv.frequency, rcv.note);
            ESP_LOGD(TAG, "comm_task: Frequência %.2fHz, Nota %s impressa.", rcv.frequency, rcv.note);

            // (Opcional) imprimir amostras
            for (size_t i = 0; i < rcv.length; i++) {
                printf("%.3f\n", rcv.samples[i]);
                // Considerar reduzir ou remover o delay para melhorar a velocidade
                // vTaskDelay(pdMS_TO_TICKS(10));
            }

            // Sinaliza fim do bloco
            printf("END\n");
            ESP_LOGD(TAG, "comm_task: Sinal de fim de bloco 'END' impresso.");
            vTaskDelay(pdMS_TO_TICKS(5));
        }

    }
}

/** ----------------------------------------------------------------
 *  Função principal
 *    - Configura GPTimer p/ LED (opcional)
 *    - Inicializa I2S
 *    - Cria filas
 *    - Cria tarefas: mic_task, audio_task, comm_task
 *  ---------------------------------------------------------------- */
void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando sistema de detecção de pitch...");

    // 1) Configura GPTimer para piscar LED (opcional)
    configure_led_timer();

    // 2) Inicializa I2S
    i2s_init();
    ESP_LOGI(TAG, "I2S inicializado.");

    // 3) Cria as filas
    xRawQueue    = xQueueCreate(5, sizeof(raw_block_t));
    xResultQueue = xQueueCreate(5, sizeof(audio_data_t));

    if (!xRawQueue || !xResultQueue) {
        ESP_LOGE(TAG, "Erro ao criar filas.");
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    ESP_LOGI(TAG, "Filas de comunicação criadas com sucesso.");

    // 4) Cria tarefas
    TaskHandle_t mic_task_handle = NULL;
    TaskHandle_t audio_task_handle = NULL;
    TaskHandle_t comm_task_handle = NULL;

    if (xTaskCreatePinnedToCore(mic_task, "mic_task", 16384, NULL, 7, &mic_task_handle, 0) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar mic_task.");
    } else {
        ESP_LOGI(TAG, "mic_task criada com sucesso.");
    }

    if (xTaskCreatePinnedToCore(audio_task, "audio_task", 16384, NULL, 6, &audio_task_handle, 1) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar audio_task.");
    } else {
        ESP_LOGI(TAG, "audio_task criada com sucesso.");
    }

    if (xTaskCreatePinnedToCore(comm_task, "comm_task", 8192, NULL, 5, &comm_task_handle, 1) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar comm_task.");
    } else {
        ESP_LOGI(TAG, "comm_task criada com sucesso.");
    }

    // 5) Loop infinito
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
