// include/mic.h
#ifndef MIC_H
#define MIC_H

#include "def.h"

/**
 * @brief Inicializa o I2S para ler dados do INMP441.
 * @return ESP_OK em sucesso, ou código de erro correspondente.
 */
esp_err_t i2s_init(void);

/**
 * @brief Lê amostras do I2S e as converte para float normalizado.
 * @param buffer Buffer de saída para amostras normalizadas
 * @param  length  Número de amostras (floats)
 * @return Número de amostras lidas com sucesso.
 */
size_t i2s_read_samples(float *buffer, size_t length);

/**
 * @brief Libera os recursos alocados para o canal I2S.
 */
void i2s_deinit(void);

#endif // MIC_H
