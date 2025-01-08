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

    // Limites das notas musicais (C0 ~16.35Hz a C8 ~4186Hz)
    if (frequency < 16.35f || frequency > 4186.0f) {
        ESP_LOGE(TAG_NOTE, "Frequência %.2f Hz fora do intervalo das notas musicais.", frequency);
        return -1;
    }

    // Cálculo matemático para determinar a nota e a oitava
    float ratio = frequency / A4_FREQUENCY;
    float log2_ratio = 0.0f;
    log2f_vect(&ratio, &log2_ratio, 1); // Substitui dsps_log2f_f32

    float note_number_tmp = 0.0f;
    float mul1 = 12.0f;
    float mul2 = log2_ratio;
    mult_vect(&mul1, &mul2, &note_number_tmp, 1); // Substitui dsps_mul_f32
    float note_number_f = note_number_tmp + 49.0f; // A4 é a nota número 49
    int rounded_note = (int)roundf(note_number_f) - 1;

    if (rounded_note < 0 || rounded_note >= 88) { // Assumindo 88 teclas de piano (C0 a C8)
        ESP_LOGE(TAG_NOTE, "Nota calculada fora do intervalo de 88 teclas do piano.");
        return -1;
    }

    // Determinar a oitava e o índice da nota
    int octave = (rounded_note / 12) - 1; // C0 começa na oitava -1
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
    float pow_input = ((float)(rounded_note - 48)) / 12.0f;
    float pow_result = powf(2.0f, pow_input);
    float mapped_frequency = A4_FREQUENCY * pow_result;
    result->frequency = mapped_frequency;

    return 0; // Sucesso
}
