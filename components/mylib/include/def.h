// include/def.h
#ifndef DEF_H
#define DEF_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_timer.h"

// Configurações de Teste via ou #define

#define TESTE 0 

#define NUM_TEST_FREQUENCIES 9
extern const float test_frequencies[NUM_TEST_FREQUENCIES];

// Ativar verificação (definir como 1 para ativar, 0 para desativar)
#define ENABLE_VERIFICATION 1

// Pinos do I2S 
#define I2S_PORT        (I2S_NUM_0)    // I2S número 0
#define I2S_SCK         GPIO_NUM_12    // Pino de Clock BCLK
#define I2S_WS          GPIO_NUM_11    // Pino de Word Select LRCLK
#define I2S_SD          GPIO_NUM_10    // Pino de Dados DIN do microfone INMP441

// Configurações de Amostragem
#define SAMPLE_RATE     (44100)        // Taxa de amostragem em Hz (16kHz ou 48kHz são comuns para INMP441)
#define BUFFER_SIZE     (1 << 11)         // Tamanho do buffer de áudio para leitura e processamento

// Definições de LED e Temporizador
#define LED_GPIO        GPIO_NUM_9     // Pino do LED indicador
#define LED_BLINK_HZ     (1)            // Frequência de piscar do LED (1 Hz -> 1 segundo)

// Definições do Threshold Fixo
#define FIXED_THRESHOLD 0.05f          // Threshold fixo para o algoritmo YIN

#define SMOOTHING_WINDOW_SIZE 10  // Ajuste conforme necessário

// Definição da Frequência de Referência para A4
#define A4_FREQUENCY    (440.0f)        // Frequência padrão para a nota A4 em Hz

// Definições Adicionais (Se Necessário)
// Exemplo: Configurações para Filtros
#define BANDPASS_LOW_FREQ 80.0f          // Frequência de corte inferior do filtro passa-banda em Hz
#define BANDPASS_HIGH_FREQ 1000.0f       // Frequência de corte superior do filtro passa-banda em Hz

// Definições para Algoritmo YIN
#define YIN_THRESHOLD 0.1f               // Limite de detecção de pitch
#define YIN_PROBABILITY 0.1f             // Probabilidade mínima para confirmação de pitch
#define YIN_MIN_LAG 40                    // Lag mínimo para detecção de pitch
#define YIN_MAX_LAG 1000                  // Lag máximo para detecção de pitch
#define YIN_ENABLE_INTERPOLATE 1          // Habilitar interpolação parabólica (1: habilitado, 0: desabilitado)

// Definições de Botões para Controle do Sistema
#define BTN_OFF       GPIO_NUM_16       // Botão para desligar o sistema
#define BTN_CONT      GPIO_NUM_17       // Botão para continuar a operação
#define BTN_TIMED     GPIO_NUM_18       // Botão para ativar o modo temporizado

// Configuração de Tempo Limite para Modo Temporizado
#define TIMED_DURATION_MS (5000)        // Duração do modo temporizado em milissegundos (5 segundos)

#endif // DEF_H
