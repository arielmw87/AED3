# Plantilla AED3 — ATmega328P con PlatformIO

Proyecto base para la materia **Arquitectura y Estructura de Datos 3**
de la EESTN°1. Pensado para usarse como punto de partida en trabajos
prácticos con microcontroladores AVR.

## Qué incluye

| Componente | Ubicación | Descripción |
|---|---|---|
| Esqueleto FSM | `src/main.c` | Estructura base de máquina de estados |
| Guía FSM | `src/FSM.md` | Referencia rápida sobre cómo usar FSMs en C |
| Driver LCD HD44780 | `lib/lcd/` | Display alfanumérico, interfaz 4-bit u 8-bit |
| Driver I2C (TWI) | `lib/i2c/` | Comunicación I2C en modo master |
| Driver USART | `lib/usart/` | Comunicación serie (TX/RX) |
| Utilidades | `lib/util/` | Macros de bits, timer de 1 ms |

## Hardware objetivo

- **MCU:** ATmega328P (Arduino Nano)
- **IDE:** [PlatformIO](https://platformio.org/)
- **Framework:** Arduino

## Cómo usar esta plantilla

1. Clonar o descargar el repositorio
2. Abrir la carpeta en VS Code con PlatformIO instalado
3. Editar `src/main.c` — el esqueleto de FSM ya está listo
4. Si se usa el LCD: configurar los pines en `lib/lcd/lcd_config.h`
5. Si se usa I2C: ajustar la velocidad en `lib/i2c/i2c_config.h`
6. Si se usa USART: ajustar la velocidad en `lib/usart/usart_config.h`
7. Compilar y cargar con el botón de PlatformIO (`→`)

## Estructura del proyecto

```
├── src/
│   ├── main.c          Punto de entrada — esqueleto FSM
│   └── FSM.md          Guía de máquinas de estado
├── lib/
│   ├── lcd/            Driver LCD HD44780
│   │   ├── lcd_config.h  ← configurar pines acá
│   │   ├── lcd.h
│   │   ├── lcd.c
│   │   └── README.md
│   ├── i2c/            Driver I2C (TWI)
│   │   ├── i2c_config.h  ← configurar velocidad acá
│   │   ├── i2c.h
│   │   ├── i2c.c
│   │   └── README.md
│   ├── usart/          Driver USART
│   │   ├── usart_config.h  ← configurar velocidad acá
│   │   ├── usart.h
│   │   ├── usart.c
│   │   └── README.md
│   └── util/           Utilidades generales
│       ├── util.h
│       └── util.c
└── platformio.ini      Configuración del proyecto
```

## Bibliotecas incluidas

### LCD HD44780 — `lib/lcd/`

Driver para displays alfanuméricos con controlador HD44780.
Configuración de pines en tiempo de compilación.

```c
#include "lcd.h"

lcd_init();
lcd_write_string("Hola mundo!");
lcd_set_cursor(0, 1);
lcd_write_int(42);
```

Ver [`lib/lcd/README.md`](lib/lcd/README.md) para la referencia completa.

### I2C (TWI) — `lib/i2c/`

Driver I2C en modo master usando el módulo TWI por hardware.
SDA en PC4, SCL en PC5 (pines fijos del ATmega328P).

```c
#include "i2c.h"

i2c_init();
i2c_write_reg(0x48, 0x01, 0xFF);   // escribir registro
i2c_read_reg(0x48, 0x00, &valor);  // leer registro
```

Ver [`lib/i2c/README.md`](lib/i2c/README.md) para la referencia completa.

### USART — `lib/usart/`

Driver para comunicación serial asíncrona. TXD en PD1, RXD en PD0 (fijos).
Selección automática de U2X para mejor precisión del baud rate.

```c
#include "usart.h"

usart_init();
usart_send_line("Hola!");
usart_send_int(42);

char c = usart_recv_wait();   // espera un carácter
```

Ver [`lib/usart/README.md`](lib/usart/README.md) para la referencia completa.
