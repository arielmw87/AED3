# Driver I2C (TWI) para AVR ATmega328P

Driver en C para comunicación I2C en **modo master** usando el módulo TWI
por hardware del ATmega328P. Toda la configuración se resuelve en tiempo
de compilación.

## Características

- Configuración en un único archivo (`i2c_config.h`) — solo la velocidad del bus
- Pines **fijos por hardware** — SDA en PC4, SCL en PC5 (no hay nada que configurar)
- API de **bajo nivel** para control total del bus
- API de **alto nivel** para las operaciones más comunes (write, read, registros)
- Manejo de **REPEATED START** para el patrón write-then-read de sensores
- **Timeout de seguridad** configurable — evita bloqueos si el bus falla
- Códigos de error descriptivos

---

## Pines (fijos en el ATmega328P)

```
ATmega328P         Bus I2C
──────────         ───────
  PC4  (A4)  ───   SDA  (datos)
  PC5  (A5)  ───   SCL  (reloj)
```

> Los pines son fijos en el hardware del ATmega328P. No hay configuración
> de pines — a diferencia de otras interfaces como SPI o el LCD paralelo.

Conexión típica con pull-up (obligatoria en I2C):

```
VCC ──┬── 4.7 kΩ ──┬── SDA (PC4)
      │             │
      └── 4.7 kΩ ──┴── SCL (PC5)
```

> Sin resistencias de pull-up el bus no funciona.
> Valores habituales: 4.7 kΩ a 100 kHz, 2.2 kΩ a 400 kHz.

---

## Configuración rápida

Abrir `i2c_config.h` y ajustar la velocidad del bus:

```c
#define I2C_FREQ     100000UL   // 100 kHz (estándar) o 400000UL (rápido)
#define I2C_TIMEOUT  10000      // iteraciones de espera; 0 = sin límite
```

Eso es todo. La frecuencia real de SCL se calcula y verifica en tiempo
de compilación a partir de `F_CPU`.

---

## Uso rápido

```c
#include "i2c.h"

int main(void) {
    i2c_init();

    // Escribir un byte en el registro 0x10 del dispositivo con dirección 0x48
    i2c_write_reg(0x48, 0x10, 0xAB);

    // Leer un byte del registro 0x00
    uint8_t valor;
    i2c_read_reg(0x48, 0x00, &valor);
}
```

---

## Referencia de la API

### Inicialización

| Función | Descripción |
|---|---|
| `i2c_init()` | Configura el módulo TWI. Llamar antes de todo lo demás. |

### API de bajo nivel

Permiten construir cualquier secuencia I2C manualmente.

| Función | Descripción |
|---|---|
| `i2c_start(addr, rw)` | START + dirección + R/W. `rw`: `I2C_WRITE` o `I2C_READ`. |
| `i2c_write(data)` | Envía un byte (modo escritura). |
| `i2c_read(ack)` | Recibe un byte. `ack`: `I2C_ACK` o `I2C_NACK`. |
| `i2c_stop()` | Envía la condición STOP y libera el bus. |

### API de alto nivel

Encapsulan START, datos y STOP en una sola llamada.
En caso de error envían STOP automáticamente.

| Función | Descripción |
|---|---|
| `i2c_write_to(addr, data, len)` | Envía `len` bytes al dispositivo `addr`. |
| `i2c_read_from(addr, buf, len)` | Lee `len` bytes del dispositivo `addr`. |
| `i2c_write_reg(addr, reg, value)` | Escribe `value` en el registro `reg`. |
| `i2c_read_reg(addr, reg, &value)` | Lee un byte del registro `reg`. |
| `i2c_read_regs(addr, reg, buf, len)` | Lee `len` bytes a partir del registro `reg`. |

### Códigos de retorno

Todas las funciones (excepto `i2c_init`, `i2c_read` e `i2c_stop`) devuelven:

| Código | Valor | Significado |
|---|:---:|---|
| `I2C_OK` | 0 | Operación completada con éxito. |
| `I2C_ERR_START` | 1 | No se pudo enviar la condición START. |
| `I2C_ERR_ADDR` | 2 | El dispositivo no respondió (NACK en dirección). |
| `I2C_ERR_DATA` | 3 | El dispositivo envió NACK durante datos. |
| `I2C_ERR_TIMEOUT` | 4 | Se agotó el timeout esperando al bus. |

---

## Ejemplos

### Escritura simple

```c
// Enviar 3 bytes al dispositivo 0x27
uint8_t datos[] = { 0x01, 0x02, 0x03 };
uint8_t err = i2c_write_to(0x27, datos, 3);

if (err != I2C_OK) {
    // manejar el error
}
```

### Lectura de registro (sensores)

```c
// Leer el registro 0x00 de un sensor en dirección 0x68 (MPU-6050, etc.)
uint8_t quien_soy;
i2c_read_reg(0x68, 0x00, &quien_soy);
```

### Lectura de múltiples registros

```c
// Leer 6 bytes de acelerómetro a partir del registro 0x3B
uint8_t raw[6];
i2c_read_regs(0x68, 0x3B, raw, 6);

int16_t accel_x = (raw[0] << 8) | raw[1];
int16_t accel_y = (raw[2] << 8) | raw[3];
int16_t accel_z = (raw[4] << 8) | raw[5];
```

### Uso de bajo nivel (control total)

```c
// Patrón write-then-read con REPEATED START
i2c_start(0x50, I2C_WRITE);   // START + dirección de escritura
i2c_write(0x00);              // dirección alta de memoria
i2c_write(0x10);              // dirección baja
i2c_start(0x50, I2C_READ);   // REPEATED START + dirección de lectura
uint8_t b0 = i2c_read(I2C_ACK);
uint8_t b1 = i2c_read(I2C_ACK);
uint8_t b2 = i2c_read(I2C_NACK);  // último byte
i2c_stop();
```

---

## Uso futuro: LCD con placa de expansión I2C (PCF8574)

Las placas de expansión I2C para LCD usan el chip **PCF8574**, un
expansor de 8 bits de E/S que se comunica por I2C.

```
              PCF8574
             ┌───────┐
  I2C ───────┤ SDA   │  bit 7 ── D7 (LCD pin 14)
  I2C ───────┤ SCL   │  bit 6 ── D6 (LCD pin 13)
  A0  ───────┤ A0    │  bit 5 ── D5 (LCD pin 12)
  A1  ───────┤ A1    │  bit 4 ── D4 (LCD pin 11)
  A2  ───────┤ A2    │  bit 3 ── BL (backlight)
             │       │  bit 2 ── EN (LCD pin 6)
             │       │  bit 1 ── RW (LCD pin 5)
             └───────┘  bit 0 ── RS (LCD pin 4)
```

La dirección I2C depende de los pines A0/A1/A2 de la placa:

| A2 | A1 | A0 | Dirección |
|:--:|:--:|:--:|:---------:|
|  0 |  0 |  0 |  `0x27`   |
|  0 |  0 |  1 |  `0x26`   |
|  1 |  1 |  1 |  `0x20`   |

La operación fundamental es enviar un solo byte para controlar los 8 bits
del PCF8574 a la vez — directamente con esta biblioteca:

```c
// Encender D7, D4 y RS simultáneamente
uint8_t estado = (1<<7) | (1<<4) | (1<<0);
i2c_write_to(0x27, &estado, 1);
```

La extensión del driver LCD (`lcd_i2c`) usará esta biblioteca internamente
para reemplazar el acceso directo a los registros de puerto por escrituras
I2C al PCF8574, manteniendo la misma API de `lcd.h`.

---

## Direcciones I2C de dispositivos comunes

| Dispositivo | Dirección |
|---|:---:|
| PCF8574 (LCD backpack) | `0x20`–`0x27` |
| PCF8574A (variante) | `0x38`–`0x3F` |
| OLED SSD1306 | `0x3C` o `0x3D` |
| MPU-6050 (acelerómetro) | `0x68` o `0x69` |
| BMP280 (temperatura/presión) | `0x76` o `0x77` |
| DS3231 (RTC) | `0x68` |
| AT24Cxx (EEPROM) | `0x50`–`0x57` |

> Para descubrir qué dispositivos hay en el bus se puede hacer un
> **I2C scan**: intentar `i2c_start(addr, I2C_WRITE)` para cada dirección
> de 0x00 a 0x7F y ver cuáles responden con `I2C_OK`.

---

## Detalles de implementación

### El módulo TWI del ATmega328P

El ATmega328P incluye un módulo TWI por hardware que implementa I2C.
Funciona mediante registros:

| Registro | Función |
|---|---|
| `TWBR` | Bit Rate — controla la velocidad de SCL |
| `TWSR` | Status — indica el estado actual del bus |
| `TWDR` | Data — byte a enviar o recibido |
| `TWCR` | Control — inicia operaciones (START, STOP, ACK...) |

El driver espera el flag `TWINT` (TWI Interrupt Flag) después de cada
operación. Cuando `TWINT = 1` el hardware terminó y el código de estado
en `TWSR` indica el resultado.

### Cálculo de velocidad en tiempo de compilación

```
TWBR = (F_CPU / I2C_FREQ − 16) / 2     (con prescaler = 1)
```

Para F_CPU = 16 MHz:

| `I2C_FREQ` | `TWBR` | SCL real |
|:---:|:---:|:---:|
| 100 000 Hz | 72 | 100 kHz |
| 400 000 Hz | 12 | 400 kHz |

El valor se calcula y valida como `_I2C_TWBR` en `i2c_config.h`.

### Secuencia de una transacción

```
Master                         Dispositivo
  │                                │
  ├─ START ──────────────────────► │
  ├─ SLA+W ──────────────────────► │
  │                       ACK ◄───┤
  ├─ REG ────────────────────────► │
  │                       ACK ◄───┤
  ├─ REPEATED START ─────────────► │
  ├─ SLA+R ──────────────────────► │
  │                       ACK ◄───┤
  │           DATA[0] ◄───────────┤
  ├─ ACK ────────────────────────► │
  │           DATA[1] ◄───────────┤
  ├─ NACK ───────────────────────► │  (último byte)
  ├─ STOP ───────────────────────► │
```

---

## Estructura del proyecto

```
lib/i2c/
├── i2c_config.h   ← EDITAR ESTE ARCHIVO (velocidad y timeout)
├── i2c.h          Declaraciones de la API y códigos de error
├── i2c.c          Implementación del driver
└── README.md      Esta documentación
```
