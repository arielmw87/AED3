#include <Arduino.h>

/* =========================================================================
 * Estados de la FSM
 * ========================================================================= */
typedef enum {
    ST_INIT,
    ST_IDLE,
    /* agregar estados acá */
} estado_t;

/* =========================================================================
 * Variables globales
 * ========================================================================= */
static estado_t estado_actual;

/* =========================================================================
 * Prototipos
 * ========================================================================= */
static void fsm_init(void);
static void fsm_idle(void);

/* =========================================================================
 * Main
 * ========================================================================= */
int main(void)
{
    estado_actual = ST_INIT;

    while (1) {
        switch (estado_actual) {
            case ST_INIT: fsm_init(); break;
            case ST_IDLE: fsm_idle(); break;
            default:      estado_actual = ST_INIT; break;
        }
    }
}

/* =========================================================================
 * Funciones de estado
 * ========================================================================= */
static void fsm_init(void)
{
    /* inicialización de hardware y variables */

    estado_actual = ST_IDLE;
}

static void fsm_idle(void)
{
    /* esperar eventos */
}
