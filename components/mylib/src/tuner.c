// src/note.c
#include "tuner.h"
#include "def.h"
#include "utils.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG_NOTE = "NOTE";

// Notas musicais básicas
static const char *notes[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

/**
 * @brief Retorna a nota musical mais próxima da frequência fornecida.
 *
 * @param frequency Frequência em Hz.
 * @param result    Estrutura note_t para armazenar a nota e a oitava calculadas.
 * @return int 0 se bem-sucedido, -1 se houver erro.
 */
int get_note(float frequency, note_t *result) {
    if (frequency <= 0.0f || result == NULL) {
        ESP_LOGE(TAG_NOTE, "Frequência inválida ou ponteiro nulo passado para get_note.");
        return -1;
    }

    // Limites das notas musicais MIDI (A0 ~27.5Hz a C8 ~4186Hz)
    if (frequency < 27.5f || frequency > 4186.0f) {
        ESP_LOGE(TAG_NOTE, "Frequência %.2f Hz fora do intervalo das notas musicais MIDI.", frequency);
        return -1;
    }

    // Cálculo matemático para determinar a nota e a oitava
    float ratio = frequency / A4_FREQUENCY;
    float log2_ratio = log2f(ratio); // Substitui log2f_vect para um único elemento

    float note_number_f = 12.0f * log2_ratio + 69.0f; // A4 é a nota número 69
    int rounded_note = (int)roundf(note_number_f); // REMOVIDO o -1

    // Atualizado para incluir o MIDI 108 (C8)
    if (rounded_note < 21 || rounded_note > 108) { // Assumindo 88 teclas de piano (A0 a C8)
        ESP_LOGE(TAG_NOTE, "Nota calculada fora do intervalo de 88 teclas do piano.");
        return -1;
    }

    // Determinar a oitava e o índice da nota
    int octave = (rounded_note / 12) - 1; // Correção na fórmula da oitava
    int note_index = rounded_note % 12;

    if (note_index < 0 || note_index >= 12) {
        ESP_LOGE(TAG_NOTE, "Índice da nota calculada inválido.");
        return -1; // Verificação de segurança
    }

    // Preencher a estrutura com o nome da nota e a oitava usando strncpy para evitar overflow
    strncpy(result->note, notes[note_index], sizeof(result->note) - 1);
    result->note[sizeof(result->note) - 1] = '\0'; // Assegura terminação nula
    result->octave = octave;

    // Calcula a frequência da nota mapeada para ajuste
    float pow_input = ((float)(rounded_note - 69)) / 12.0f;
    float pow_result = powf(2.0f, pow_input);
    float mapped_frequency = A4_FREQUENCY * pow_result;
    result->frequency = mapped_frequency;

    return 0; // Sucesso
}
