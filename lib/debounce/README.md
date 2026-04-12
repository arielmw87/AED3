# Debounce de pulsadores para AVR ATmega328P

Biblioteca en C para eliminar el rebote de pulsadores por software usando el
algoritmo **"Ultimate Debouncer"** de Elliot Williams (Hackaday, 2015),
basado en el algoritmo original de **Jack Ganssle**.
Toda la configuración se resuelve en **tiempo de compilación**.

## Características

- Configuración en un único archivo (`debounce_config.h`) — pines, cantidad y velocidad
- Soporte de **múltiples pulsadores** con una sola llamada por tick
- Pines asignables a **cualquier puerto** del AVR (B, C o D)
- Detección de **pulsación**, **liberación** y **tiempo sostenido**
- Eventos que se **consumen al leer** — sin flanco duplicado
- Se integra con el **timer de 1 kHz ya existente** en `util.c`
- Verificaciones de configuración en tiempo de compilación

---

## Configuración rápida

**Paso 1** — Abrir `debounce_config.h` y ajustar la cantidad de pulsadores y la velocidad de muestreo:

```c
#define NUM_BOTONES               2   // un botón por fila en debounce.c
#define DEBOUNCE_TICKS_POR_MUESTRA  5   // 5 ticks × 1 ms = 1 muestra cada 5 ms
```

**Paso 2** — En `debounce.c`, editar la tabla `_config[]` con un botón por fila:

```c
static const _debounce_config_t _config[NUM_BOTONES] = {
    BOTON(D, PD2, 1),   /* botón 0 — Arduino pin 2 */
    BOTON(D, PD3, 1),   /* botón 1 — Arduino pin 3 */
};
```

**Paso 3** — Agregar `debounce_tick()` a la ISR del timer en `util.c`:

```c
ISR(TIMER0_COMPA_vect) {
    debounce_tick();
}
```

Eso es todo.

---

## Conexión del hardware

La conexión más habitual es con el pulsador entre el pin y GND, usando la
resistencia pullup interna del AVR:

```
VCC ── [pullup interno del AVR]
             │
ATmega328P   │
  PD2  ──────┤
             │
         [pulsador]
             │
            GND
```

> Con `activo_bajo = 1` en la macro `BOTON(...)`, el pin se configura
> automáticamente como `INPUT_PULLUP`. No se necesita resistencia externa.

Para conectar a VCC en lugar de GND (activo en alto), usar `activo_bajo = 0`
y agregar la resistencia pulldown externa (el AVR no tiene pulldown interno):

```
            GND ── 10 kΩ ──┐
                            │
ATmega328P                  │
  PD2  ─────────────────────┤
                            │
                        [pulsador]
                            │
                           VCC
```

---

## Uso rápido

```c
#include "debounce.h"
#include "util.h"
#include <avr/interrupt.h>

ISR(TIMER0_COMPA_vect) {
    debounce_tick();        // llamar desde la ISR del timer de 1 kHz
}

int main(void) {
    config_timer0();        // timer de 1 kHz (de util.h)
    debounce_init();        // configura los pines
    sei();                  // habilitar interrupciones globales

    while (1) {

        if (debounce_pulsado(0))
            /* acción al pulsar el botón 0 */;

        if (debounce_soltado(0))
            /* acción al soltar el botón 0 */;

        // pulsación larga: más de 1 segundo (200 muestras × 5 ms)
        if (debounce_ticks_sostenido(0) > 200)
            /* acción de pulsación larga */;
    }
}
```

---

## Referencia de la API

### Inicialización

| Función | Descripción |
|---|---|
| `debounce_init()` | Configura los pines de todos los pulsadores. Llamar antes de todo lo demás. |
| `debounce_tick()` | Actualiza el historial de todos los pulsadores. Llamar desde la ISR del timer. |

### Eventos (se consumen al leer)

| Función | Descripción |
|---|---|
| `debounce_pulsado(id)` | Devuelve `1` si el botón fue presionado desde la última consulta. |
| `debounce_soltado(id)` | Devuelve `1` si el botón fue soltado desde la última consulta. |

> Cada evento se **consume** al leer: si el botón fue pulsado una sola vez,
> `debounce_pulsado()` devuelve `1` la primera vez que se la llama, y luego
> vuelve a devolver `0` hasta la próxima pulsación.

### Estado y duración

| Función | Descripción |
|---|---|
| `debounce_esta_abajo(id)` | Devuelve `1` si el botón está presionado ahora. No consume evento. |
| `debounce_ticks_sostenido(id)` | Devuelve cuántas muestras consecutivas lleva el botón presionado. Se resetea al soltar. |

El parámetro `id` es el índice del pulsador: `0` para el primero, `1` para el segundo, y así sucesivamente.

---

## Cómo agregar pulsadores

1. Aumentar `NUM_BOTONES` en `debounce_config.h`.
2. Agregar una fila `BOTON(...)` en la tabla `_config[]` de `debounce.c`.

Formato de la macro:

```
BOTON( puerto , bit , activo_bajo )
         │       │        └── 1 = pulsador conectado a GND (pullup)
         │       │            0 = pulsador conectado a VCC (pulldown externo)
         │       └────────── nombre del bit: PD2, PB0, PC3, etc.
         └────────────────── letra del puerto AVR: B, C o D
```

Ejemplo con cuatro botones en puertos distintos:

```c
// debounce_config.h
#define NUM_BOTONES  4

// debounce.c
static const _debounce_config_t _config[NUM_BOTONES] = {
    BOTON(D, PD2, 1),   /* botón 0 — Arduino pin 2 */
    BOTON(D, PD3, 1),   /* botón 1 — Arduino pin 3 */
    BOTON(B, PB0, 1),   /* botón 2 — Arduino pin 8 */
    BOTON(C, PC0, 1),   /* botón 3 — Arduino pin A0 */
};
```

---

## Ejemplos

### Contador de pulsaciones

```c
uint16_t contador = 0;

while (1) {

    if (debounce_pulsado(0)) {
        contador++;
        usart_send_string("Pulsaciones: ");
        usart_send_int(contador);
        usart_send_char('\n');
    }
}
```

### Pulsación corta y larga sobre el mismo botón

```c
#define TICKS_LARGA  200   // 200 × 5 ms = 1 segundo

while (1) {

    if (debounce_soltado(0)) {
        if (debounce_ticks_sostenido(0) >= TICKS_LARGA)
            accion_larga();
        else
            accion_corta();
    }
}
```

> Se evalúa `debounce_ticks_sostenido()` dentro del bloque de `debounce_soltado()`
> porque `ticks_sostenido` se resetea a 0 al soltar, pero su último valor
> sigue disponible en el mismo ciclo en que se detecta la liberación.

### Dos botones independientes

```c
while (1) {

    if (debounce_pulsado(0))
        led_toggle(LED_VERDE);

    if (debounce_pulsado(1))
        led_toggle(LED_ROJO);
}
```

### Combinación de botones

```c
while (1) {

    // Los dos pulsadores presionados al mismo tiempo
    if (debounce_esta_abajo(0) && debounce_esta_abajo(1))
        modo_reset();
}
```

---

## Velocidad de muestreo y tolerancia al rebote

`DEBOUNCE_TICKS_POR_MUESTRA` controla cada cuántos ticks del timer de 1 kHz
se toma una muestra del estado de los pines.

Con la máscara del algoritmo, los 3 bits centrales del historial actúan como
"don't care" (zona de rebote ignorada):

| `TICKS_POR_MUESTRA` | Intervalo entre muestras | Rebote tolerado | Latencia máxima |
|:---:|:---:|:---:|:---:|
| 3 | 3 ms | 9 ms | 18 ms |
| 5 | 5 ms | 15 ms | 30 ms |
| 8 | 8 ms | 24 ms | 48 ms |
| 10 | 10 ms | 30 ms | 60 ms |

> La mayoría de los pulsadores mecánicos rebotan entre 5 ms y 20 ms.
> El valor por defecto de 5 es adecuado para pulsadores de buena calidad.
> Para pulsadores más ruidosos o viejos, usar 8 o 10.

---

## Detalles de implementación

### El problema del rebote

Cuando se presiona un pulsador, los contactos metálicos rebotan varias veces
antes de estabilizarse. Lo que debería ser una sola transición, el microcontrolador
lo ve como muchas:

```
Estado real:    ─────────────────┐             ┌──────────────────
                                 └─────────────┘

Pin medido:     ─────────────────┐ ┌─┐ ┌─────┐ ┌────────────────
                                 └─┘ └─┘     └─┘
                                  ↑  rebote  ↑
                                  └──────────┘
                                   5 ms – 20 ms
```

### El algoritmo: registro de desplazamiento con máscara

Cada pulsador mantiene un **historial de 8 bits** donde se acumulan muestras sucesivas.
En cada llamado a `debounce_tick()` (cada `TICKS_POR_MUESTRA` ms) se descarta
la muestra más vieja y se inserta la nueva en el extremo derecho:

```
historial antes:   [ b7  b6  b5  b4  b3  b2  b1  b0 ]
                      ↑ más vieja                más nueva ↑

historial después: [ b6  b5  b4  b3  b2  b1  b0  nueva ]
                     (b7 se descartó)
```

Para detectar una pulsación se aplica una **máscara** que ignora los bits
centrales (donde ocurre el rebote) y evalúa solo los extremos:

```
historial:  [  S   S   x   x   x   N   N   N  ]
               ↑                   ↑
         estado previo          estado nuevo
        (bits 7-6)              (bits 2-0)

máscara:    [  1   1   0   0   0   1   1   1  ]  =  0xC7
```

| Resultado de `historial & máscara` | Evento |
|:---:|---|
| `0b00000111` | **Pulsación**: estaba suelto (S=0), ahora pulsado (N=1) |
| `0b11000000` | **Liberación**: estaba pulsado (S=1), ahora suelto (N=0) |
| cualquier otro | Zona de rebote o sin cambio → ignorar |

Al detectar el evento, el historial se rellena con `0xFF` (pulsado) o `0x00`
(suelto). Esto garantiza que la máscara no vuelva a disparar hasta que haya
una nueva transición real — actúa como **histéresis por software**.

### Selección de pines en tiempo de compilación

La macro `BOTON()` usa concatenación de tokens del preprocesador para
construir los nombres de los registros AVR a partir de la letra del puerto:

```c
#define _REG2(prefix, port)  prefix##port
#define _REG(prefix, port)   _REG2(prefix, port)

// Con BOTON(D, PD2, 1):
_REG(PIN,  D)  →  PIND
_REG(DDR,  D)  →  DDRD
_REG(PORT, D)  →  PORTD
```

El compilador resuelve todo en tiempo de compilación y genera exactamente
el mismo código que si se hubiera escrito `PIND`, `DDRD` y `PORTD` a mano
— sin costo en velocidad ni en memoria RAM.

---

## Referencias

- Elliot Williams — *Embed With Elliot: Debounce Your Noisy Buttons* (Hackaday, 2015)
  - [Parte I — el problema y soluciones básicas](https://hackaday.com/2015/12/09/embed-with-elliot-debounce-your-noisy-buttons-part-i/)
  - [Parte II — el "Ultimate Debouncer"](https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/)
- Jack Ganssle — *A Guide To Debouncing* (algoritmo original en el que se basa el de Williams)
  - [Parte II — implementación con shift register](https://www.ganssle.com/debouncing-pt2.htm)

---

## Estructura del proyecto

```
lib/debounce/
├── debounce_config.h   ← EDITAR ESTE ARCHIVO (cantidad y velocidad)
├── debounce.c          ← EDITAR LA TABLA _config[] (pines de cada botón)
├── debounce.h          Declaraciones de la API
└── README.md           Esta documentación
```
