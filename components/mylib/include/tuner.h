// include/tuner.h
#ifndef TUNER_H
#define TUNER_H

#include <stddef.h>

/**
 * @brief Estrutura para armazenar a nota musical detectada.
 */
typedef struct {
    char note[8];   // Nome da nota (ex: "A4")
    int octave;     // Oitava da nota
    float frequency; // Frequência mapeada da nota
} note_t;

/**
 * @brief Retorna a nota musical mais próxima da frequência fornecida.
 *
 * @param frequency Frequência em Hz.
 * @param result    Estrutura note_t para armazenar a nota e a oitava calculadas.
 * @return int 0 se bem-sucedido, -1 se houver erro.
 */
int get_note(float frequency, note_t *result);

#endif // TUNER_H
