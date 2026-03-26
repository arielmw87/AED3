# Máquinas de Estado Finito (FSM)

Una FSM es un modelo de programa que divide el comportamiento en **estados**
discretos. En cada momento el sistema está en exactamente un estado, y pasa
a otro cuando ocurre un **evento** o se cumple una **condición**.

## Conceptos básicos

| Concepto | Descripción |
|---|---|
| **Estado** | Situación estable del sistema (ej: `ESPERAR`, `MEDIR`, `ALARMA`). |
| **Transición** | Cambio de un estado a otro. |
| **Evento/condición** | Lo que dispara una transición (entrada digital, timer, valor de sensor...). |
| **Acción** | Lo que hace el sistema al entrar, salir o estar en un estado. |

## Estructura en C

```
         evento A              evento B
ST_INIT ──────────► ST_IDLE ──────────► ST_PROCESO
   ▲                                        │
   └────────────────────────────────────────┘
                    evento C
```

Cada estado es una función. La variable `estado_actual` decide cuál
se ejecuta en cada iteración del `while(1)`.

```c
typedef enum {
    ST_INIT,
    ST_IDLE,
    ST_PROCESO,
} estado_t;

static estado_t estado_actual;

// En el loop principal:
switch (estado_actual) {
    case ST_INIT:    fsm_init();    break;
    case ST_IDLE:    fsm_idle();    break;
    case ST_PROCESO: fsm_proceso(); break;
}

// Dentro de cada función, la transición es solo una asignación:
static void fsm_idle(void) {
    if (boton_presionado()) {
        estado_actual = ST_PROCESO;   // transición
    }
}
```

## Reglas prácticas

- **Un estado = una función.** Toda la lógica de ese estado vive ahí.
- **Las transiciones son asignaciones** a `estado_actual`. Nada más.
- **No usar `delay()` dentro de los estados.** Usar timers o flags de tiempo
  para que el `while(1)` nunca se bloquee.
- **El estado `INIT` se ejecuta una sola vez** (al encender) y luego
  transiciona al primer estado operativo.
- Mantener las funciones de estado **cortas y enfocadas**. Si crecen
  demasiado, es señal de que el estado hace más de una cosa.

## Ejemplo: LED con anti-rebote

```
ST_APAGADO ──── boton baja ───► ST_DEBOUNCE ──── 20ms ───► ST_ENCENDIDO
ST_ENCENDIDO ── boton baja ───► ST_DEBOUNCE ──── 20ms ───► ST_APAGADO
```

```c
typedef enum { ST_APAGADO, ST_DEBOUNCE, ST_ENCENDIDO } estado_t;
static estado_t estado_actual;
static uint32_t t_debounce;

static void fsm_apagado(void) {
    if (!digitalRead(PIN_BTN)) {
        t_debounce = millis();
        estado_actual = ST_DEBOUNCE;
    }
}

static void fsm_debounce(void) {
    if (millis() - t_debounce >= 20) {
        estado_actual = digitalRead(PIN_BTN) ? estado_actual : // ruido
                        (estado_actual == ST_APAGADO ? ST_ENCENDIDO : ST_APAGADO);
    }
}

static void fsm_encendido(void) {
    digitalWrite(PIN_LED, HIGH);
    if (!digitalRead(PIN_BTN)) {
        t_debounce = millis();
        estado_actual = ST_DEBOUNCE;
    }
}
```
