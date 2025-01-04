#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#include "buttons.h"
#include "var.h"

/**
 * @brief Estrutura para cada botão:
 *        - pino
 *        - timer de debounce
 *        - qual modo “pressionar” esse botão sugere
 */
typedef struct {
    gpio_num_t pin;
    TimerHandle_t debounce_timer;
    operation_mode_t mode_on_press;
} button_t;

/**
 * Variáveis globais definidas aqui (extern em buttons.h)
 */
volatile operation_mode_t current_mode = MODE_OFF;
bool tuner_active = false;
bool continuos    = false;

/**
 * Declara os botões (OFF, CONT, TIMED).
 * Ajuste se quiser nomes BTN_OFF, BTN_CONT, etc.
 */
static button_t btn_off = {
    .pin = BTN_OFF,
    .debounce_timer = NULL,
    .mode_on_press = MODE_OFF
};
static button_t btn_cont = {
    .pin = BTN_CONT,
    .debounce_timer = NULL,
    .mode_on_press = MODE_CONTINUOUS
};
static button_t btn_timed = {
    .pin = BTN_TIMED,
    .debounce_timer = NULL,
    .mode_on_press = MODE_TIMED
};

/**
 * Protótipos
 */
static void IRAM_ATTR button_isr_handler(void *arg);
static void debounce_timer_callback(TimerHandle_t xTimer);

/**
 * @brief Retorna o modo atual
 */
operation_mode_t get_operation_mode(void)
{
    return current_mode;
}

/**
 * @brief Força um modo
 */
void force_mode(operation_mode_t new_mode)
{
    current_mode = new_mode;
    printf("Force mode => %d\n", (int)new_mode);
}

/**
 * @brief Inicializa GPIOs e timers de debounce
 */
void buttons_init(void)
{
    // Configura pinos de botões como input + pull-up, interrupção na borda de descida
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BTN_OFF) | (1ULL << BTN_CONT) | (1ULL << BTN_TIMED),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&cfg);

    // Cria timers de debounce
    btn_off.debounce_timer  = xTimerCreate("debounce_off",  pdMS_TO_TICKS(50), pdFALSE, &btn_off,  debounce_timer_callback);
    btn_cont.debounce_timer = xTimerCreate("debounce_cont", pdMS_TO_TICKS(50), pdFALSE, &btn_cont, debounce_timer_callback);
    btn_timed.debounce_timer= xTimerCreate("debounce_timed",pdMS_TO_TICKS(50), pdFALSE, &btn_timed,debounce_timer_callback);

    // Instala driver de ISR
    gpio_install_isr_service(0);

    // Adiciona ISR p/ cada botão
    gpio_isr_handler_add(btn_off.pin,  button_isr_handler, (void *)&btn_off);
    gpio_isr_handler_add(btn_cont.pin, button_isr_handler, (void *)&btn_cont);
    gpio_isr_handler_add(btn_timed.pin,button_isr_handler, (void *)&btn_timed);

    printf("buttons_init: current_mode=%d, tuner_active=%d, continuos=%d\n",
           (int)current_mode, (int)tuner_active, (int)continuos);
}

/**
 * @brief ISR - Reinicia timer de debounce
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_t *btn = (button_t *)arg;
    // Desabilita interrupção do pino para evitar ruídos
    gpio_intr_disable(btn->pin);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerStartFromISR(btn->debounce_timer, &xHigherPriorityTaskWoken);

    // Se precisar: portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Callback do timer de debounce
 *        Se pino ainda LOW => confirmamos o pressionamento
 */
static void debounce_timer_callback(TimerHandle_t xTimer)
{
    button_t *btn = (button_t *)pvTimerGetTimerID(xTimer);

    // Se ainda está LOW, consideramos pressionado
    if (gpio_get_level(btn->pin) == 0)
    {
        // Lógica do button
        // Se mode_on_press=MODE_OFF => toggla tuner_active
        // Se mode_on_press=MODE_CONTINUOUS => toggla 'continuos'
        // Se mode_on_press=MODE_TIMED => define MODE_TIMED
        operation_mode_t m = btn->mode_on_press;
        if (m == MODE_OFF) {
            // toggla tuner_active
            tuner_active = !tuner_active;
            // se desligou => current_mode=OFF, continuos=false
            if (!tuner_active) {
                current_mode = MODE_OFF;
                continuos = false;
            }
        }
        else if (m == MODE_CONTINUOUS) {
            // toggla cont
            continuos = !continuos;
            // se entrou em CONT => define current_mode=CONTINUOUS
            // (só se tuner_active==true ?)
            if (!tuner_active) {
                tuner_active = true; 
            }
            current_mode = MODE_CONTINUOUS;
        }
        else if (m == MODE_TIMED) {
            // se não estiver ativo, ligue
            if (!tuner_active) {
                tuner_active = true;
            }
            current_mode = MODE_TIMED;
            // continuos=false? se quiser
            continuos = false;
        }

        printf("Button pino=%d => current_mode=%d, tuner_active=%d, continuos=%d\n",
               (int)btn->pin, (int)current_mode, (int)tuner_active, (int)continuos);
    }

    // Reabilita interrupção
    gpio_intr_enable(btn->pin);
}
