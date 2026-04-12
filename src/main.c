#include <Arduino.h>
#include "util.h"
#include "debounce.h"

/* =========================================================================
 * Estados de la FSM
 *
 * Agregar nuevos estados acá y una función fsm_NombreEstado() abajo.
 * ========================================================================= */
typedef enum {
    ST_INIT,
    ST_IDLE,
    ST_PROCESO,
    ST_ESPERA,
    /* agregar estados acá */
} estado_t;

/* =========================================================================
 * Prototipos
 *
 * Cada función recibe el estado actual y devuelve el próximo estado.
 * Si devuelve su propio estado, la máquina se queda donde está.
 * Si devuelve otro estado, la máquina hace la transición.
 * ========================================================================= */
static estado_t fsm_init   (void);
static estado_t fsm_idle   (void);
static estado_t fsm_proceso(void);
static estado_t fsm_espera (void);

/* =========================================================================
 * Main
 *
 * estado_actual es una variable LOCAL: solo el switch la lee y la escribe.
 * Las funciones de estado no saben en qué estado está la máquina — solo
 * evalúan su condición y devuelven a dónde ir.
 * ========================================================================= */
int main(void)
{
    estado_t estado_actual = ST_INIT;

    config_timer0();
    sei();

    while (1) {
        switch (estado_actual) {
            case ST_INIT:    estado_actual = fsm_init();    break;
            case ST_IDLE:    estado_actual = fsm_idle();    break;
            case ST_PROCESO: estado_actual = fsm_proceso(); break;
            case ST_ESPERA:  estado_actual = fsm_espera();  break;
            default:         estado_actual = ST_INIT;       break;
        }
    }
}

/* =========================================================================
 * Funciones de estado
 *
 * Cada función:
 *   - Ejecuta la lógica del estado actual.
 *   - Devuelve el próximo estado (puede ser el mismo para quedarse).
 *
 * Las transiciones son visibles de un vistazo en el switch de main():
 * no hay que abrir cada función para entender el flujo.
 * ========================================================================= */

/* -------------------------------------------------------------------------
 * ST_INIT
 * Inicialización de hardware y variables. Se ejecuta una sola vez al
 * encender. Siempre transiciona a ST_IDLE al terminar.
 * ------------------------------------------------------------------------- */
static estado_t fsm_init(void)
{
    debounce_init();

    /* inicialización de otros periféricos */

    return ST_IDLE;     /* transición incondicional → siempre va a ST_IDLE */
}

/* -------------------------------------------------------------------------
 * ST_IDLE
 * Espera a que el usuario pulse el botón 0.
 *   - Botón pulsado  → ST_PROCESO
 *   - Sin evento     → ST_IDLE  (se queda)
 * ------------------------------------------------------------------------- */
static estado_t fsm_idle(void)
{
    if (debounce_pulsado(0))
        return ST_PROCESO;  /* transición condicional */

    return ST_IDLE;         /* sin evento: quedarse */
}

/* -------------------------------------------------------------------------
 * ST_PROCESO
 * Cuenta 100 ciclos haciendo algo. El botón 0 cancela en cualquier momento.
 *   - Conteo completo  → ST_ESPERA
 *   - Botón pulsado    → ST_IDLE   (cancelar)
 *   - Conteo en curso  → ST_PROCESO (se queda)
 * ------------------------------------------------------------------------- */
static estado_t fsm_proceso(void)
{
    static uint8_t ciclos = 0;

    if (debounce_pulsado(0)) {
        ciclos = 0;             /* resetear para la próxima vez */
        return ST_IDLE;         /* cancelar: volver a esperar */
    }

    ciclos++;
    if (ciclos >= 100) {
        ciclos = 0;
        return ST_ESPERA;       /* proceso terminado */
    }

    return ST_PROCESO;          /* todavía no terminó: quedarse */
}

/* -------------------------------------------------------------------------
 * ST_ESPERA
 * Espera hasta que el botón 0 se sostenga más de 1 segundo para continuar,
 * o un toque corto vuelve a empezar el proceso.
 *   - Pulsación larga  (> 1 s)  → ST_IDLE
 *   - Pulsación corta  (≤ 1 s)  → ST_PROCESO
 *   - Sin evento                → ST_ESPERA (se queda)
 *
 * 200 muestras × 5 ms/muestra = 1000 ms = 1 segundo
 * (ver DEBOUNCE_TICKS_POR_MUESTRA en debounce_config.h)
 * ------------------------------------------------------------------------- */
static estado_t fsm_espera(void)
{
    #define TICKS_PULSACION_LARGA  200

    if (debounce_soltado(0)) {
        if (debounce_ticks_sostenido(0) >= TICKS_PULSACION_LARGA)
            return ST_IDLE;     /* pulsación larga: terminar */
        else
            return ST_PROCESO;  /* pulsación corta: repetir proceso */
    }

    return ST_ESPERA;           /* esperando: quedarse */
}
