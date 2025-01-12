// src/mic.c
#include "mic.h"
#include "utils.h"
#include "esp_log.h"

static const char *TAG_MIC = "MIC";

// Handle do Canal I2S RX
static i2s_chan_handle_t rx_handle = NULL;


esp_err_t i2s_init(void)
{
    if (rx_handle != NULL) {
        ESP_LOGW(TAG_MIC, "Canal I2S já está inicializado.");
        return ESP_OK;
    }
    
    // Cria o canal I2S: Modo MASTER + RX
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    
    ESP_LOGI(TAG_MIC, "Criando novo canal I2S...");
    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MIC, "Falha em i2s_new_channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG_MIC, "Inicializando modo padrão do I2S...");
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode      = I2S_SLOT_MODE_MONO,
            .slot_mask      = I2S_STD_SLOT_LEFT, // pino L/R -> GND => canal esquerdo
            .ws_width       = 32,
            .ws_pol         = false,
            .bit_shift      = true,   // I2S Philips
            .left_align     = false,  // I2S Philips
            .big_endian     = false,
            .bit_order_lsb  = false,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_SCK,
            .ws   = I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_SD,
            .invert_flags = {
                .mclk_inv = false,
                //ruído -> teste .bclk_inv = true
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MIC, "Falha em i2s_channel_init_std_mode: %s", esp_err_to_name(ret));
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
        return ret;
    }

    ESP_LOGI(TAG_MIC, "Habilitando canal I2S...");
    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MIC, "Falha ao habilitar canal I2S: %s", esp_err_to_name(ret));
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
        return ret;
    }

    ESP_LOGI(TAG_MIC, "I2S RX 24-bit inicializado com sucesso.");
    return ESP_OK;
}

size_t i2s_read_samples(float *buffer, size_t length)
{
    if (rx_handle == NULL) {
        ESP_LOGE(TAG_MIC, "I2S não foi inicializado.");
        return 0;
    }

    // Buffer temporário para leitura (int32_t devido a 24 bits)
    int32_t *temp_buf = (int32_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(int32_t), MALLOC_CAP_SPIRAM);
    if (temp_buf == NULL) {
        ESP_LOGE(TAG_MIC, "Falha ao alocar temp_buf.");
        return 0;
    }
    size_t bytes_read = 0;
    size_t to_read = length * sizeof(int32_t);

    // Bloqueia até ler
    esp_err_t ret = i2s_channel_read(rx_handle, temp_buf, to_read, &bytes_read,  pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MIC, "Erro ao ler do I2S: %s", esp_err_to_name(ret));
        return 0;
    }

    size_t samples_read = bytes_read / sizeof(int32_t);
    if (samples_read > BUFFER_SIZE) {
        samples_read = BUFFER_SIZE;
    }

    for (size_t i = 0; i < samples_read; i++) {
        // Conversão para 24 bits se for INMP441: 
        // SHIFT 8 (24 bits significativos) e normaliza p/ [-1, +1]
        int32_t d = (temp_buf[i] >> 8);
        if (d & 0x800000) { // Verifica o bit de sinal (24º bit)
            d |= 0xFF000000;
        }
        // Normalização para float
        buffer[i] = (float)d / (float)(1 << 23);
    }
    ESP_LOGD(TAG_MIC, "Processamento de %zu samples concluído.", samples_read);

    heap_caps_free(temp_buf);
    return samples_read;
}

void i2s_deinit(void)
{
    if (rx_handle != NULL) {
        i2s_channel_disable(rx_handle);
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
        ESP_LOGI(TAG_MIC, "Canal I2S desativado e deletado.");
    }
}
