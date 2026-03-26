# Driver USART para AVR ATmega328P

Driver en C para comunicación serial asíncrona (USART) usando el módulo
por hardware del ATmega328P. Toda la configuración se resuelve en tiempo
de compilación.

## Características

- Configuración en un único archivo (`usart_config.h`) — solo velocidad y formato
- Pines **fijos por hardware** — TXD en PD1, RXD en PD0
- Selección **automática de U2X** (doble velocidad) cuando mejora la precisión
- Envío de texto, enteros y hexadecimales
- Recepción **bloqueante** y **no bloqueante**

---

## Pines (fijos en el ATmega328P)

```
ATmega328P          Adaptador USB-Serie
──────────          ────────────────────
  PD1  (D1)  ───►   RX  del adaptador
  PD0  (D0)  ◄───   TX  del adaptador
  GND        ───    GND del adaptador
```

> Los pines son los mismos que usa el chip USB-serie de la placa Arduino
> (CH340 o FT232). Al usar el driver con Arduino Nano/Uno, la comunicación
> con la PC ya está conectada por hardware.

---

## Configuración rápida

Abrir `usart_config.h` y ajustar la velocidad y el formato:

```c
#define USART_BAUD       9600UL  // velocidad en bps
#define USART_DATA_BITS  8       // 5, 6, 7 u 8
#define USART_PARITY     0       // 0=ninguna, 1=par, 2=impar
#define USART_STOP_BITS  1       // 1 o 2
```

La configuración más común (compatible con cualquier terminal serie) es
**9600 8N1** o **115200 8N1** — los valores por defecto.

El valor exacto de UBRR y el modo U2X se calculan y verifican en tiempo
de compilación a partir de `F_CPU` y `USART_BAUD`.

---

## Uso rápido

```c
#include "usart.h"

int main(void) {
    usart_init();

    usart_send_line("Iniciando...");

    usart_send_string("Valor: ");
    usart_send_int(42);
    usart_send_char('\n');

    // Esperar un carácter del usuario
    char c = usart_recv_wait();
    usart_send_char(c);   // eco
}
```

---

## Referencia de la API

### Inicialización

| Función | Descripción |
|---|---|
| `usart_init()` | Configura la USART. Llamar antes de todo lo demás. |

### Transmisión

| Función | Descripción |
|---|---|
| `usart_send_char(c)` | Envía un carácter (bloquea si el buffer está lleno). |
| `usart_send_string(str)` | Envía una cadena terminada en `'\0'`. |
| `usart_send_line(str)` | Envía una cadena seguida de `"\r\n"`. |
| `usart_send_int(value)` | Envía un `int32_t` en decimal. |
| `usart_send_hex(value, digits)` | Envía un `uint32_t` en hexadecimal con `digits` dígitos. |

### Recepción

| Función | Descripción |
|---|---|
| `usart_available()` | Devuelve `1` si hay un byte en el buffer de recepción. |
| `usart_recv(&c)` | Lee un byte sin bloquear. Devuelve `1` si había dato, `0` si no. |
| `usart_recv_wait()` | Bloquea hasta recibir un byte y lo devuelve. |

---

## Ejemplos

### Debug de variables

```c
usart_init();

usart_send_string("temperatura: ");
usart_send_int(temperatura);
usart_send_line(" grados");

usart_send_string("registro: 0x");
usart_send_hex(PORTD, 2);
usart_send_char('\n');
```

Salida en el terminal:
```
temperatura: 23 grados
registro: 0x3F
```

### Menú interactivo (recepción no bloqueante)

```c
char tecla;

while (1) {

    /* hacer otras cosas sin bloquear */

    if (usart_recv(&tecla)) {
        switch (tecla) {
            case '1': usart_send_line("opcion 1"); break;
            case '2': usart_send_line("opcion 2"); break;
            default:  usart_send_line("?");        break;
        }
    }
}
```

### Recibir un comando completo (bloqueante)

```c
char buf[16];
uint8_t i = 0;

// Leer hasta '\n' o llenar el buffer
while (i < sizeof(buf) - 1) {
    char c = usart_recv_wait();
    if (c == '\n') break;
    buf[i++] = c;
}
buf[i] = '\0';

usart_send_string("Recibido: ");
usart_send_line(buf);
```

---

## Velocidades comunes y precisión

Con **F_CPU = 16 MHz**, los valores habituales y su error real:

| Baud rate | UBRR | U2X | Baud real | Error |
|:---:|:---:|:---:|:---:|:---:|
| 9 600 | 103 | no | 9 615 | 0,16 % |
| 19 200 | 51 | no | 19 231 | 0,16 % |
| 38 400 | 25 | no | 38 462 | 0,16 % |
| 57 600 | 16 | no | 58 824 | 2,12 % |
| 57 600 | 34 | sí | 57 143 | 0,79 % ← U2X elegido |
| 115 200 | 8 | no | 111 111 | 3,55 % |
| 115 200 | 16 | sí | 117 647 | 2,12 % ← U2X elegido |

> El driver selecciona automáticamente el modo con menor error.
> Un error menor al 2 % es aceptable para comunicación serial.

---

## Detalles de implementación

### Registros del módulo USART

| Registro | Función |
|---|---|
| `UBRR0H:L` | Baud rate (12 bits) |
| `UCSR0A` | Estado: `UDRE0` (TX listo), `RXC0` (dato recibido), `U2X0` |
| `UCSR0B` | Control: `TXEN0` (habilitar TX), `RXEN0` (habilitar RX) |
| `UCSR0C` | Formato: modo, paridad, stop bits, bits de datos |
| `UDR0` | Registro de datos — escribir para TX, leer para RX |

### Cálculo de baud rate en tiempo de compilación

```
Modo normal:   UBRR = F_CPU / (16 × BAUD) − 1
Modo U2X:      UBRR = F_CPU / (8  × BAUD) − 1
```

El preprocesador calcula ambos valores, calcula el error de cada uno
y elige el modo con menor diferencia respecto al baud rate pedido.
Todo resuelto en compilación, sin costo en ejecución.

---

## Estructura del proyecto

```
lib/usart/
├── usart_config.h   ← EDITAR ESTE ARCHIVO (velocidad y formato)
├── usart.h          Declaraciones de la API
├── usart.c          Implementación del driver
└── README.md        Esta documentación
```
