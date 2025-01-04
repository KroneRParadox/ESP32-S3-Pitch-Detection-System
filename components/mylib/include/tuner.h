#ifndef TUNER_H
#define TUNER_H

#include <stddef.h>

/**
 * @brief Retorna a nota musical mais próxima da frequência fornecida.
 *
 * @param frequency    Frequência em Hz
 * @param buffer       Buffer para armazenar a string da nota
 * @param buffer_size  Tamanho do buffer
 * @return const char*  Ponteiro para o buffer contendo a nota
 */
int get_note(float frequency, char *buffer, size_t buffer_size);

#endif // TUNER_H
