#ifndef MIC_H
#define MIC_H

#include <stddef.h>


/**
 * @brief Inicializa o I2S para ler dados do INMP441.
 */
void i2s_init(void);

/**
 * @brief Lê amostras (float) do microfone ([-1..+1]).
 *
 * @param[out] buffer  Onde armazenar as amostras
 * @param[in]  length  Número de amostras (floats)
 * @return Quantidade lida efetivamente
 */
size_t i2s_read_samples(float *buffer, size_t length);


#endif // MIC_H
