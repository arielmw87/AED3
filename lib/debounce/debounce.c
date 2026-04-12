/**
 * @file  debounce.c
 * @brief Implementación del debounce de pulsadores para ATmega328P.
 *
 * Algoritmo "Ultimate Debouncer" de Elliot Williams, basado en el de Jack Ganssle.
 * Ver referencias en debounce.h.
 */

#include "debounce.h"
#include <avr/io.h>

/* =========================================================================
 * Macro para definir pulsadores en la tabla config[]
 *
 * El preprocesador arma los nombres de los registros automáticamente:
 *   BOTON(D, PD2, 1)  →  PIND, DDRD, PORTD, PD2, activo_bajo=1
 *
 * Así el compilador no genera código extra: todo se resuelve en tiempo
 * de compilación igual que si se hubiera escrito PIND a mano.
 * ========================================================================= */
#define _REG2(prefix, port)  prefix##port           /* concatena dos tokens */
#define _REG(prefix, port)   _REG2(prefix, port)    /* capa extra necesaria para que funcione con macros */

#define BOTON(puerto, bit, activo_bajo) \
    { &_REG(PIN,  puerto),              \
      &_REG(DDR,  puerto),              \
      &_REG(PORT, puerto),              \
      (bit),                            \
      (activo_bajo) }

/* =========================================================================
 * Configuración de cada pulsador (privado — el usuario no la ve)
 *
 * Almacena en qué registro y bit leer el pin, y con qué polaridad.
 * Es de solo lectura después de la inicialización.
 * ========================================================================= */
typedef struct {
    volatile uint8_t *reg_pin;      /* PINx  — leer el nivel del pin        */
    volatile uint8_t *reg_ddr;      /* DDRx  — configurar como entrada       */
    volatile uint8_t *reg_port;     /* PORTx — habilitar pullup interno      */
    uint8_t           bit;          /* Número de bit dentro del puerto       */
    uint8_t           activo_bajo;  /* 1 = el pin vale 0 cuando está pulsado */
} _debounce_config_t;

/* =========================================================================
 * TABLA DE PULSADORES — AGREGAR O QUITAR FILAS ACÁ
 *
 * Formato:  BOTON( puerto , bit , activo_bajo )
 *
 *   BOTON(D, PD2, 1)  →  Arduino pin 2,  pulsador a GND con pullup
 *   BOTON(D, PD3, 1)  →  Arduino pin 3,  pulsador a GND con pullup
 *   BOTON(B, PB0, 1)  →  Arduino pin 8,  pulsador a GND con pullup
 *
 * Recordar actualizar NUM_BOTONES en debounce_config.h si se cambia la cantidad.
 * ========================================================================= */
static const _debounce_config_t _config[NUM_BOTONES] = {
    BOTON(D, PD2, 1),   /* botón 0 — Arduino pin 2 */
    BOTON(D, PD3, 1),   /* botón 1 — Arduino pin 3 */
};

/* =========================================================================
 * Estado interno de cada pulsador (privado)
 *
 * El usuario accede a estos datos solo a través de las funciones de la API.
 * ========================================================================= */
typedef struct {
    uint8_t  historial;          /* shift register: las últimas 8 muestras  */
    uint8_t  estado;             /* 1 = pulsado ahora, 0 = suelto           */
    uint8_t  flag_pulsado;       /* se pone en 1 al detectar una pulsación  */
    uint8_t  flag_soltado;       /* se pone en 1 al detectar una liberación */
    uint16_t ticks_sostenido;    /* muestras consecutivas en estado pulsado */
} _debounce_estado_t;

static _debounce_estado_t _estado[NUM_BOTONES];

/* Cuenta los ticks del timer entre muestras */
static uint8_t _contador = 0;

/* =========================================================================
 * Lee el pin de un pulsador y normaliza la lectura:
 * devuelve 1 si está pulsado y 0 si está suelto, sin importar la polaridad.
 * ========================================================================= */
static uint8_t _leer_pin(uint8_t id)
{
    uint8_t nivel = (*_config[id].reg_pin >> _config[id].bit) & 0x01;

    if (_config[id].activo_bajo)
        return !nivel;   /* activo en bajo: invertir para normalizar */
    else
        return nivel;    /* activo en alto: sin invertir */
}

/* =========================================================================
 * Aplica el algoritmo "Ultimate Debouncer" a un único pulsador.
 *
 * Paso 1 — nueva muestra:
 *   Se desplaza el historial 1 bit a la izquierda y se inserta la muestra
 *   nueva en el bit 0. El bit más antiguo (bit 7) cae hacia afuera.
 *
 * Paso 2 — detección con máscara:
 *   DEBOUNCE_MASK ignora los bits centrales (zona de rebote) y compara:
 *
 *     [ S S x x x N N N ]   con   MASK = 1 1 0 0 0 1 1 1
 *
 *   Pulsado  si S=0 antes y N=1 ahora:  resultado == 0b00000111
 *   Soltado  si S=1 antes y N=0 ahora:  resultado == 0b11000000
 *
 * Paso 3 — reset del historial (histéresis):
 *   Al detectar el evento se rellena el historial para que la máscara no
 *   vuelva a disparar hasta que haya una nueva transición real.
 * ========================================================================= */
static void _actualizar(uint8_t id)
{
    uint8_t muestra = _leer_pin(id);

    /* Paso 1: desplazar e insertar nueva muestra en el LSB */
    _estado[id].historial = (uint8_t)((_estado[id].historial << 1) | muestra);

    /* Paso 2: detectar PULSACIÓN — estaba suelto, ahora pulsado y estable */
    if ((_estado[id].historial & DEBOUNCE_MASK) == 0b00000111) {

        _estado[id].historial      = 0xFF;  /* rellenar: evitar re-disparo */
        _estado[id].estado         = 1;
        _estado[id].flag_pulsado   = 1;
        _estado[id].ticks_sostenido = 0;

    }
    /* Paso 2: detectar LIBERACIÓN — estaba pulsado, ahora suelto y estable */
    else if ((_estado[id].historial & DEBOUNCE_MASK) == 0b11000000) {

        _estado[id].historial      = 0x00;  /* rellenar: evitar re-disparo */
        _estado[id].estado         = 0;
        _estado[id].flag_soltado   = 1;
        _estado[id].ticks_sostenido = 0;

    }

    /* Paso 3: acumular tiempo pulsado (saturar en 0xFFFF para no desbordarse) */
    if (_estado[id].estado && _estado[id].ticks_sostenido < 0xFFFF)
        _estado[id].ticks_sostenido++;
}

/* =========================================================================
 * API pública
 * ========================================================================= */

void debounce_init(void)
{
    uint8_t id;

    for (id = 0; id < NUM_BOTONES; id++) {

        /* DDRx bit = 0 → pin como entrada */
        *_config[id].reg_ddr &= ~(1 << _config[id].bit);

        /* PORTx bit = 1 con DDR=0 → habilita la resistencia pullup interna */
        if (_config[id].activo_bajo)
            *_config[id].reg_port |= (1 << _config[id].bit);

        /* Estado inicial: historial vacío, nada pulsado */
        _estado[id].historial       = 0x00;
        _estado[id].estado          = 0;
        _estado[id].flag_pulsado    = 0;
        _estado[id].flag_soltado    = 0;
        _estado[id].ticks_sostenido = 0;
    }
}

void debounce_tick(void)
{
    uint8_t id;

    /* Submuestre: solo procesar cada DEBOUNCE_TICKS_POR_MUESTRA ticks */
    _contador++;
    if (_contador < DEBOUNCE_TICKS_POR_MUESTRA)
        return;
    _contador = 0;

    for (id = 0; id < NUM_BOTONES; id++)
        _actualizar(id);
}

uint8_t debounce_pulsado(uint8_t id)
{
    if (id >= NUM_BOTONES)
        return 0;

    if (_estado[id].flag_pulsado) {
        _estado[id].flag_pulsado = 0;  /* consumir el evento */
        return 1;
    }
    return 0;
}

uint8_t debounce_soltado(uint8_t id)
{
    if (id >= NUM_BOTONES)
        return 0;

    if (_estado[id].flag_soltado) {
        _estado[id].flag_soltado = 0;  /* consumir el evento */
        return 1;
    }
    return 0;
}

uint8_t debounce_esta_abajo(uint8_t id)
{
    if (id >= NUM_BOTONES)
        return 0;

    return _estado[id].estado;
}

uint16_t debounce_ticks_sostenido(uint8_t id)
{
    if (id >= NUM_BOTONES)
        return 0;

    return _estado[id].ticks_sostenido;
}
