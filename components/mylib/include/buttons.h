#ifndef BUTTONS_H
#define BUTTONS_H
 
/**
 * @brief Modo de operação
 */
typedef enum {
    MODE_OFF = 0,
    MODE_CONTINUOUS,
    MODE_TIMED
} operation_mode_t;

/**
 * @brief Variáveis globais de estado.
 *        Manteremos “extern” aqui, definidas em buttons.c
 */
extern volatile operation_mode_t current_mode;
extern bool tuner_active;   ///< ON/OFF do tuner
extern bool continuos;      ///< Se modo CONT. está ativo

/**
 * @brief Inicializa botões OFF, CONT, TIMED com timers de debounce
 *        e lógica: OFF => toggla tuner_active, CONT => toggla 'continuos',
 *        TIMED => define current_mode=MODE_TIMED, etc.
 */
void buttons_init(void);

/**
 * @brief Retorna o modo atual (caso precise).
 */
operation_mode_t get_operation_mode(void);

/**
 * @brief Força um modo ignorando botões,
 *        ex.: force_mode(MODE_OFF) para desligar manualmente.
 */
void force_mode(operation_mode_t new_mode);

#endif // BUTTONS_H
