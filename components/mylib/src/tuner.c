// components/mylib/src/tuner.c

#include "tuner.h"
#include <math.h>
#include <string.h>
#include "var.h"

// Estrutura para armazenar notas musicais e suas frequências correspondentes
typedef struct {
    const char *note;
    float frequency;
} note_t;

// Notas básicas
static const char *note_names[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

// Tabela de notas musicais de C0 a B8
static const note_t notes_table[] = {
    {"C0",  16.35}, {"C#0", 17.32}, {"D0",  18.35}, {"D#0", 19.45}, {"E0",  20.60},
    {"F0",  21.83}, {"F#0", 23.12}, {"G0",  24.50}, {"G#0", 25.96}, {"A0",  27.50},
    {"A#0", 29.14}, {"B0",  30.87},
    {"C1",  32.70}, {"C#1", 34.65}, {"D1",  36.71}, {"D#1", 38.89}, {"E1",  41.20},
    {"F1",  43.65}, {"F#1", 46.25}, {"G1",  49.00}, {"G#1", 51.91}, {"A1",  55.00},
    {"A#1", 58.27}, {"B1",  61.74},
    {"C2",  65.41}, {"C#2", 69.30}, {"D2",  73.42}, {"D#2", 77.78}, {"E2",  82.41},
    {"F2",  87.31}, {"F#2", 92.50}, {"G2",  98.00}, {"G#2", 103.83}, {"A2",  110.00},
    {"A#2", 116.54}, {"B2",  123.47},
    {"C3",  130.81}, {"C#3", 138.59}, {"D3",  146.83}, {"D#3", 155.56}, {"E3",  164.81},
    {"F3",  174.61}, {"F#3", 185.00}, {"G3",  196.00}, {"G#3", 207.65}, {"A3",  220.00},
    {"A#3", 233.08}, {"B3",  246.94},
    {"C4",  261.63}, {"C#4", 277.18}, {"D4",  293.66}, {"D#4", 311.13}, {"E4",  329.63},
    {"F4",  349.23}, {"F#4", 369.99}, {"G4",  392.00}, {"G#4", 415.30}, {"A4",  440.00},
    {"A#4", 466.16}, {"B4",  493.88},
    {"C5",  523.25}, {"C#5", 554.37}, {"D5",  587.33}, {"D#5", 622.25}, {"E5",  659.25},
    {"F5",  698.46}, {"F#5", 739.99}, {"G5",  783.99}, {"G#5", 830.61}, {"A5",  880.00},
    {"A#5", 932.33}, {"B5",  987.77},
    {"C6",  1046.50}, {"C#6", 1108.73}, {"D6",  1174.66}, {"D#6", 1244.51}, {"E6",  1318.51},
    {"F6",  1396.91}, {"F#6", 1479.98}, {"G6",  1567.98}, {"G#6", 1661.22}, {"A6",  1760.00},
    {"A#6", 1864.66}, {"B6",  1975.53},
    {"C7",  2093.00}, {"C#7", 2217.46}, {"D7",  2349.32}, {"D#7", 2489.02}, {"E7",  2637.02},
    {"F7",  2793.83}, {"F#7", 2959.96}, {"G7",  3135.96}, {"G#7", 3322.44}, {"A7",  3520.00},
    {"A#7", 3729.31}, {"B7",  3951.07},
    {"C8",  4186.01}, {"C#8", 4434.92}, {"D8",  4698.64}, {"D#8", 4978.04}, {"E8",  5274.04},
    {"F8",  5587.65}, {"F#8", 5919.91}, {"G8",  6271.93}, {"G#8", 6644.88}, {"A8",  7040.00},
    {"A#8", 7458.62}, {"B8",  7902.13}
};

// Número total de notas na tabela
static const size_t notes_count = sizeof(notes_table) / sizeof(note_t);

/**
 * @brief Retorna a nota musical mais próxima da frequência fornecida.
 *
 * @param frequency    Frequência em Hz
 * @param buffer       Buffer para armazenar a string da nota
 * @param buffer_size  Tamanho do buffer
 * @return int          0 se bem-sucedido, -1 caso contrário
 */
int get_note(float frequency, char *buffer, size_t buffer_size)
{
    if (frequency <= 0.0f || buffer == NULL || buffer_size < 3) { // mínimo "A4\0"
        strncpy(buffer, "Unknown", buffer_size);
        buffer[buffer_size - 1] = '\0';
        return -1;
    }

    const char *closest_note = "Unknown";
    float min_diff = 1000.0f; // Diferença mínima inicial

    for (size_t i = 0; i < notes_count; i++) {
        float diff = fabsf(frequency - notes_table[i].frequency);
        if (diff < min_diff) {
            min_diff = diff;
            closest_note = notes_table[i].note;
        }
    }

    // Encontrar a oitava correspondente na tabela
    // Nota: Este exemplo assume que as frequências na tabela estão corretamente mapeadas para suas oitavas
    // e que a tabela está ordenada em ordem crescente de frequência.
    // Se a tabela não estiver ordenada, considere ordenar ou implementar uma busca adequada.

    // Encontrar a nota na tabela para obter a oitava
    for (size_t i = 0; i < notes_count; i++) {
        if (strcmp(notes_table[i].note, closest_note) == 0) {
            // Oitava já está incluída no nome da nota na tabela
            strncpy(buffer, notes_table[i].note, buffer_size);
            buffer[buffer_size - 1] = '\0';
            return 1;
        }
    }

    // Se não encontrar na tabela, usar a fórmula logarítmica para determinar a oitava
    float note_number = 12.0f * log2f(frequency / A4_FREQUENCY) + 49.0f;
    int rounded_note  = (int)roundf(note_number) - 1;
    if (rounded_note < 0) {
        strncpy(buffer, "Unknown", buffer_size);
        buffer[buffer_size - 1] = '\0';
        return -1;
    }

    int octave    = rounded_note / 12;
    int note_index = rounded_note % 12;

    if (note_index < 0 || note_index >= 12) { // Verificação de segurança
        strncpy(buffer, "Unknown", buffer_size);
        buffer[buffer_size - 1] = '\0';
        return -1;
    }

    snprintf(buffer, buffer_size, "%s%d", note_names[note_index], octave);
    return 1;
}
