// src/test_all.c
#include "test.h"
#include "filters.h"   // biquad_t, bandpass_init(), biquad_process()
#include "yin.h"       // yin_init(), yin_detect_pitch(), yin_deinit()
#include "tuner.h"     // get_note()
#include "fft.h"       // fft(), calculate_magnitude(), peak_frequency()
#include "esp_log.h"
#include <math.h>
#include <string.h>

/**
 * @brief Aguarda o usuário pressionar Enter no monitor serial.
 */
void wait_for_enter(void) {
    ESP_LOGI("TEST_ALL", "Pressione Enter para continuar...");
    while (1) {
        int c = getchar();
        if (c == '\n' || c == '\r') {
            break;
        }
        vTaskDelay(10);
    }
}

/**
 * @brief Testa as funções vetoriais personalizadas.
 */
static void test_vector_functions(void *pv) {
    ESP_LOGI("TEST_ALL", "===== Teste das Funções Vetoriais Personalizadas =====");
    
    // Test sub_vect
    ESP_LOGI("TEST_ALL", "Testando sub_vect...");
    float a[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[] = {0.5f, 1.5f, 2.5f, 3.5f};
    float result_sub[4];
    uint32_t start_time = esp_timer_get_time();
    sub_vect(a, b, result_sub, 4);
    uint32_t end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Resultado da Subtração: %.2f, %.2f, %.2f, %.2f", result_sub[0], result_sub[1], result_sub[2], result_sub[3]);
    ESP_LOGI("TEST_ALL", "Tempo de execução: %ld us\n", end_time-start_time);

    // Test mult_vect
    ESP_LOGI("TEST_ALL", "Testando mult_vect...");
    float result_mul[4];
    start_time = esp_timer_get_time();
    mult_vect(a, b, result_mul, 4);
    end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Resultado da Multiplicação: %.2f, %.2f, %.2f, %.2f", result_mul[0], result_mul[1], result_mul[2], result_mul[3]);
    ESP_LOGI("TEST_ALL", "Tempo de execução: %ld us\n", end_time-start_time);

    // Test add_vect
    ESP_LOGI("TEST_ALL", "Testando add_vect...");
    float result_add[4];
    start_time = esp_timer_get_time();
    add_vect(a, b, result_add, 4);
    end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Resultado da Adição: %.2f, %.2f, %.2f, %.2f", result_add[0], result_add[1], result_add[2], result_add[3]);
    ESP_LOGI("TEST_ALL", "Tempo de execução: %ld us\n", end_time-start_time);

    // Test sum_vect
    ESP_LOGI("TEST_ALL", "Testando sum_vect...");
    start_time = esp_timer_get_time();
    float sum = sum_vect(a, 4);
    end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Resultado da Soma: %.2f", sum);
    ESP_LOGI("TEST_ALL", "Tempo de execução: %ld us\n", end_time-start_time);

    // Test cos_vect
    ESP_LOGI("TEST_ALL", "Testando cos_vect...");
    float angles[] = {0.0f, M_PI_2, M_PI, 3.0f * M_PI_2};
    float cos_results[4];
    start_time = esp_timer_get_time();
    cos_vect(angles, cos_results, 4);
    end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Resultados do Cosseno: %.2f, %.2f, %.2f, %.2f", cos_results[0], cos_results[1], cos_results[2], cos_results[3]);
    ESP_LOGI("TEST_ALL", "Tempo de execução: %ld us\n", end_time-start_time);

    // Test sin_vect
    ESP_LOGI("TEST_ALL", "Testando sin_vect...");
    float sin_results[4];
    start_time = esp_timer_get_time();
    sin_vect(angles, sin_results, 4);
    end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Resultados do Seno: %.2f, %.2f, %.2f, %.2f", sin_results[0], sin_results[1], sin_results[2], sin_results[3]);
    ESP_LOGI("TEST_ALL", "Tempo de execução: %ld us\n", end_time-start_time);

    // Test sqrt_vect
    ESP_LOGI("TEST_ALL", "Testando sqrt_vect...");
    float sqrt_input[] = {4.0f, 16.0f, 25.0f, -9.0f};
    float sqrt_results[4];
    start_time = esp_timer_get_time();
    sqrt_vect(sqrt_input, sqrt_results, 4);
    end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Resultados da Sqrt: %.2f, %.2f, %.2f, %.2f", sqrt_results[0], sqrt_results[1], sqrt_results[2], sqrt_results[3]);
    ESP_LOGI("TEST_ALL", "Tempo de execução: %ld us\n", end_time-start_time);

    ESP_LOGI("TEST_ALL", "===== Teste das Funções Vetoriais Personalizadas Concluído =====\n");
    vTaskDelete(NULL);
}

/**
 * @brief Testa a função FFT manual.
 */
static void test_fft_manual(void *pv) {
    ESP_LOGI("TEST_ALL", "===== Teste da FFT Manual =====");
    
    // Parâmetros do teste
    const size_t n = 1024; // Deve ser potência de 2
    const float sample_rate = 48000.0f; // Hz
    const float frequency = 1.0f; // Hz
    float  test_phase = 0.0f;


    // Gerar uma onda senoidal pura
    float real[n];
    float imag[n];
    for (size_t i = 0; i < n; i++) {
        imag[i] = 0.0f;
    }
    generate_sine_wave(real, n, 1000.0f, sample_rate, &test_phase);
    apply_window(real, n, 1);


    // Executar a FFT manual
    uint32_t start_time = esp_timer_get_time();
    fft(real, imag, n);
    uint32_t end_time = esp_timer_get_time();

    // Calcular a magnitude
    float magnitude[n];
    calculate_magnitude(real, imag, magnitude, n);

    // Exibir resultados
    ESP_LOGI("TEST_ALL", "FFT Manual Test:");
    for (size_t i = 0; i < n; i++) {
        ESP_LOGI("TEST_ALL", "Bin %zu: Magnitude = %.5f", i, magnitude[i]);
    }
    ESP_LOGI("TEST_ALL", "Tempo de execução da FFT: %ld us\n", end_time-start_time);

    ESP_LOGI("TEST_ALL", "===== Teste da FFT Manual Concluído =====\n");
    vTaskDelete(NULL);

}

/**
 * @brief Testa o filtro Biquad.
 */
static void test_filter(void *pv) {
    ESP_LOGI("TEST_ALL", "===== Teste do Filtro Biquad =====");
    

    // Configurar parâmetros do filtro
    float sample_rate = 44100.0f;
    float f_low = 300.0f;
    float f_high = 3000.0f;
    biquad_t bandpass_filter;
    
    // Inicializar o filtro
    
    uint32_t start_time1 = esp_timer_get_time();
    bandpass_init(&bandpass_filter, sample_rate, f_low, f_high);
    uint32_t end_time = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Inicialização do Filtro Passa-Banda concluída em %ld us", end_time-start_time1);

    // Gerar um sinal de teste (onda senoidal de 1000 Hz)
    size_t buffer_size = 1024;
    float test_phase = 0.0f;
    float audio_buffer[1024];
    generate_sine_wave(audio_buffer, buffer_size, 1000.0f, sample_rate, &test_phase);

    // Aplicar o filtro
    float filtered_buffer[1024];
    uint32_t start_time = esp_timer_get_time();
    biquad_process(&bandpass_filter, audio_buffer, filtered_buffer, buffer_size);
    uint32_t end_time1 = esp_timer_get_time();
    ESP_LOGI("TEST_ALL", "Processamento do Filtro concluído em %ld us", end_time1-start_time);

    // Opcional: Verificar alguns valores filtrados
    ESP_LOGI("TEST_ALL", "Primeiros 5 valores filtrados:");
    for (int i = 0; i < 5; i++) {
        ESP_LOGI("TEST_ALL", "filtered_buffer[%d] = %.5f", i, filtered_buffer[i]);
    }
    ESP_LOGI("TEST_ALL", "Tempo total do teste do filtro: %ld us\n", end_time1-start_time1);

    ESP_LOGI("TEST_ALL", "===== Teste do Filtro Biquad Concluído =====\n");
    vTaskDelete(NULL);

}

/**
 * @brief Testa a implementação do YIN (Detecção de Pitch).
 */
static void test_yin(void *pv) {
    ESP_LOGI("TEST_ALL", "===== Teste do YIN (Detecção de Pitch) =====");
    
    // Configurar parâmetros do YIN
    size_t buffer_size = 1024;
    int sample_rate = 44100;
    Yin yin;
    uint32_t start_time = esp_timer_get_time();   
    esp_err_t ret = yin_init(&yin, buffer_size, sample_rate, YIN_THRESHOLD, YIN_THRESHOLD_FIXED, 0.1f, 0.2f, 0.01f);
    uint32_t end_time = esp_timer_get_time();
    if (ret != ESP_OK) {
        ESP_LOGE("TEST_ALL", "Falha na inicialização do YIN.");
        return;
    }
    ESP_LOGI("TEST_ALL", "Inicialização do YIN concluída em %ld us", end_time-start_time);

    // Gerar um sinal de teste (onda senoidal de 440 Hz)
    float test_phase = 0.0f;
    float audio_buffer[1024];
    generate_sine_wave(audio_buffer, buffer_size, 440.0f, (float)sample_rate, &test_phase);

    // Detectar pitch
    float detected_pitch = 0.0f;
    start_time = esp_timer_get_time();
    ret = yin_detect_pitch(&yin, audio_buffer, &detected_pitch);
    end_time = esp_timer_get_time();
    if (ret == 0 && detected_pitch > 0.0f) {
        ESP_LOGI("TEST_ALL", "Pitch Detectado: %.2f Hz", detected_pitch);
    } else {
        ESP_LOGW("TEST_ALL", "Pitch não detectado.");
    }
    ESP_LOGI("TEST_ALL", "Tempo de detecção do Pitch: %ld us\n", end_time-start_time);

    // Liberar recursos do YIN
    yin_deinit(&yin);

    ESP_LOGI("TEST_ALL", "===== Teste do YIN Concluído =====\n");
    vTaskDelete(NULL);
}

/**
 * @brief Testa a função get_note.
 */
static void test_get_note(void *pv) {
    ESP_LOGI("TEST_ALL", "===== Teste da Função get_note =====");
    
    // Lista de frequências para testar
    float test_frequencies[] = {440.0f, 261.63f, 329.63f, 0.0f, 5000.0f};
    size_t num_tests = sizeof(test_frequencies) / sizeof(test_frequencies[0]);

    for (size_t i = 0; i < num_tests; i++) {
        float freq = test_frequencies[i];
        note_t note;
        uint32_t start_time = esp_timer_get_time();
        int ret = get_note(freq, &note);
        uint32_t end_time = esp_timer_get_time();

        if (ret == 0) {
            ESP_LOGI("TEST_ALL", "Frequência: %.2f Hz -> Nota: %s%d (%.2f Hz)", freq, note.note, note.octave, note.frequency);
        } else {
            ESP_LOGW("TEST_ALL", "Frequência: %.2f Hz -> Nota não detectada.", freq);
        }
        ESP_LOGI("TEST_ALL", "Tempo de execução do get_note: %ld us\n", end_time-start_time);
    }


    ESP_LOGI("TEST_ALL", "===== Teste da Função get_note Concluído =====\n");
    vTaskDelete(NULL);
}

/**
 * @brief Executa todos os testes consolidando os testes de funções vetoriais, FFT, filtros, YIN e get_note.
 */
void run_all_tests(void) {
    ESP_LOGI("TEST_ALL", "===== Iniciando Testes Consolidados =====\n");
    xTaskCreate(test_vector_functions, "vetores", 16384, NULL, 0, NULL);
    wait_for_enter();
    xTaskCreate(test_fft_manual, "fft", 16384, NULL, 0, NULL);
    wait_for_enter();
    xTaskCreate(test_filter, "filtro", 16384, NULL, 0, NULL);
    wait_for_enter();
    xTaskCreate(test_yin, "yin", 16384, NULL, 0, NULL);
    wait_for_enter();
    xTaskCreate(test_get_note, "note", 16384, NULL, 0, NULL);
    wait_for_enter();
    ESP_LOGI("TEST_ALL", "===== Testes Consolidados Finalizados =====\n");
}
