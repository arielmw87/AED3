# Driver LCD HD44780 para AVR ATmega

Driver en C para displays LCD alfanuméricos con controlador **HD44780** (y compatibles).
Toda la configuración de pines se resuelve en **tiempo de compilación**: cero overhead en ejecución.

## Características

- Configuración en un único archivo (`lcd_config.h`) — solo letra de puerto y número de pin
- Interfaz de **4 bits u 8 bits** seleccionable
- Pines asignables a **cualquier puerto** del AVR (A, B, C, D)
- Soporte de hasta **8 caracteres personalizados** (CGRAM)
- Compatible con displays de 1, 2 y 4 líneas
- Línea **RW opcional** — puede conectarse directamente a GND
- Verificaciones de configuración en tiempo de compilación

---

## Configuración rápida

Abrir `lcd_config.h` y editar los valores según el circuito:

```c
#define LCD_MODE    4      // interfaz: 4 u 8 bits

#define LCD_COLS   16      // columnas del display
#define LCD_ROWS    2      // filas del display

// Pines de control: indicar LETRA del puerto y NÚMERO de bit
#define LCD_RS_PORT   D    // RS → PD2
#define LCD_RS_PIN    2

#define LCD_EN_PORT   D    // EN → PD4
#define LCD_EN_PIN    4

#define LCD_USE_RW    0    // 0 = RW a GND (recomendado)
#define LCD_RW_PORT   D    // ignorado si LCD_USE_RW = 0
#define LCD_RW_PIN    3

// Bus de datos: 4 pines consecutivos en el mismo puerto
#define LCD_DATA_PORT    B   // D4–D7 del LCD en PB0–PB3
#define LCD_DATA_OFFSET  0   // empezando en el bit 0
```

Eso es todo. No hay estructuras que rellenar ni punteros que pasar.

---

## Conexión del hardware

### Modo 4 bits (recomendado — ahorra 4 pines)

```
ATmega328          LCD HD44780
──────────         ─────────────────────────────────
  PD2       ──►    RS   (pin 4)  Register Select
  PD4       ──►    EN   (pin 6)  Enable
  GND       ──►    RW   (pin 5)  (o PD3 si LCD_USE_RW = 1)
  PB0       ──►    D4   (pin 11)
  PB1       ──►    D5   (pin 12)
  PB2       ──►    D6   (pin 13)
  PB3       ──►    D7   (pin 14)
  GND       ──►    VSS  (pin 1)
  VCC       ──►    VDD  (pin 2)
 (pot.)     ──►    V0   (pin 3)  contraste (0–5 V)
  VCC       ──►    A    (pin 15) backlight +
  GND       ──►    K    (pin 16) backlight −
```

> D0–D3 (pines 7–10) quedan sin conectar en modo 4 bits.

### Modo 8 bits

Igual que arriba, más:

```
  PB4       ──►    D0   (pin 7)
  PB5       ──►    D1   (pin 8)
  PB6       ──►    D2   (pin 9)
  PB7       ──►    D3   (pin 10)
```

> En modo 8 bits los 8 pines de datos deben ser consecutivos en un mismo puerto.
> Con `LCD_DATA_PORT B` y `LCD_DATA_OFFSET 0` se usan PB0–PB7.

---

## Uso

### 1. Incluir la biblioteca

```c
#include "lcd.h"
```

### 2. Inicializar y escribir

```c
lcd_init();
lcd_write_string("Hola mundo!");

lcd_set_cursor(0, 1);          // columna 0, fila 1
lcd_write_int(-42);
```

No hay estructuras ni punteros: cada función actúa directamente sobre el hardware.

---

## Referencia de la API

### Inicialización y pantalla

| Función | Descripción |
|---|---|
| `lcd_init()` | Inicializa el hardware. Llamar antes de todo lo demás. |
| `lcd_clear()` | Borra el display y mueve el cursor a (0, 0). |
| `lcd_home()` | Mueve el cursor a (0, 0) sin borrar. |
| `lcd_set_cursor(col, row)` | Posiciona el cursor (col y row empiezan en 0). |

### Escritura

| Función | Descripción |
|---|---|
| `lcd_write_char(c)` | Escribe un carácter ASCII o un índice CGRAM (0–7). |
| `lcd_write_string(str)` | Escribe una cadena terminada en `'\0'`. |
| `lcd_write_int(value)` | Escribe un `int32_t` en base decimal. |

### Control del display

| Función | Descripción |
|---|---|
| `lcd_display_on/off()` | Enciende o apaga el display (sin borrar). |
| `lcd_cursor_on/off()` | Muestra u oculta el cursor (subrayado). |
| `lcd_blink_on/off()` | Activa o desactiva el parpadeo del cursor. |

### Desplazamiento

| Función | Descripción |
|---|---|
| `lcd_scroll_left/right()` | Desplaza todo el contenido visible. |
| `lcd_cursor_left/right()` | Mueve el cursor sin cambiar el contenido. |
| `lcd_autoscroll_on/off()` | El display se desplaza al escribir cada carácter. |

### Dirección de escritura

| Función | Descripción |
|---|---|
| `lcd_left_to_right()` | Escritura normal (izquierda a derecha). |
| `lcd_right_to_left()` | Escritura invertida. |

### Caracteres personalizados

| Función | Descripción |
|---|---|
| `lcd_create_char(loc, charmap)` | Guarda un carácter en CGRAM (posición 0–7). |

---

## Caracteres personalizados (CGRAM)

El HD44780 permite definir hasta **8 caracteres** propios de 5×8 píxeles.
Cada carácter se describe con un array de 8 bytes; los **5 bits menos significativos**
de cada byte representan una fila de píxeles, de arriba hacia abajo.

```c
uint8_t corazon[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
};

uint8_t grado[8] = {
    0b01100,
    0b10010,
    0b10010,
    0b01100,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
};

// Cargar antes de usar
lcd_create_char(0, corazon);
lcd_create_char(1, grado);

// Reposicionar el cursor (lcd_create_char lo deja en CGRAM)
lcd_home();

// Mostrar usando el índice como código de carácter
lcd_write_char(0);   // ♥
lcd_write_char(1);   // °
```

---

## `LCD_DATA_OFFSET` — elegir los pines del bus

`LCD_DATA_OFFSET` indica el bit inicial del bus dentro del puerto.
Permite usar cualquier grupo de pines consecutivos sin desperdiciar el resto del puerto.

| Modo | D4–D7 del LCD en... | `LCD_DATA_PORT` | `LCD_DATA_OFFSET` |
|:---:|---|:---:|:---:|
| 4-bit | PD4–PD7 | `D` | `4` |
| 4-bit | PB0–PB3 | `B` | `0` |
| 4-bit | PC2–PC5 | `C` | `2` |
| 8-bit | PB0–PB7 | `B` | `0` |
| 8-bit | PD0–PD7 | `D` | `0` |

---

## Ejemplo completo

```c
#include <avr/io.h>
#include "lcd.h"

// Símbolo de grado: °
uint8_t grado[8] = {
    0b01100,
    0b10010,
    0b10010,
    0b01100,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
};

int main(void)
{
    lcd_init();

    lcd_create_char(0, grado);
    lcd_home();

    lcd_write_string("Temp: ");
    lcd_write_int(25);
    lcd_write_char(0);   // °
    lcd_write_char('C');

    lcd_set_cursor(0, 1);
    lcd_write_string("Humedad: 60%");

    while (1) {}
}
```

`lcd_config.h` para este ejemplo:

```c
#define LCD_MODE    4
#define LCD_COLS   16
#define LCD_ROWS    2

#define LCD_RS_PORT   D
#define LCD_RS_PIN    2
#define LCD_EN_PORT   D
#define LCD_EN_PIN    4
#define LCD_USE_RW    0

#define LCD_DATA_PORT    B
#define LCD_DATA_OFFSET  0
```

---

## Estructura del proyecto

```
lib/lcd/
├── lcd_config.h   ← EDITAR ESTE ARCHIVO con los pines del circuito
├── lcd.h          Declaraciones de la API
├── lcd.c          Implementación del driver
└── README.md      Esta documentación
```

---

## Detalles de implementación

### Cómo funciona la selección de pines en tiempo de compilación

El driver usa concatenación de tokens del preprocesador de C para construir
los nombres de los registros AVR a partir de la letra del puerto:

```c
#define _REG2(prefix, port)  prefix##port
#define _REG(prefix, port)   _REG2(prefix, port)

// Con LCD_RS_PORT = D:
_REG(DDR,  LCD_RS_PORT)  →  DDRD
_REG(PORT, LCD_RS_PORT)  →  PORTD
```

El compilador resuelve todo esto en tiempo de compilación y genera
exactamente el mismo código que si se hubiera escrito `DDRD` y `PORTD`
directamente — sin costo en velocidad ni en memoria RAM.

### Secuencia de inicialización

```
Encendido
    │
    ▼  espera > 40 ms
    ▼  Function Set  (×3, con tiempos: 5 ms / 150 µs / 150 µs)
    ▼  [solo 4-bit] nibble 0x2 → cambio a interfaz 4-bit
    ▼  Function Set definitivo (N líneas, fuente 5×8)
    ▼  Display OFF
    ▼  Clear Display
    ▼  Entry Mode Set
    ▼  Display ON   ← listo para usar
```

### Temporización

| Operación | Espera |
|---|:---:|
| Pulso EN alto | 1 µs |
| Tras pulso EN (ejecución) | 50 µs |
| Comandos Clear / Home | 2 ms |
| Arranque (VCC estable) | 50 ms |
