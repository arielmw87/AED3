/**
 * @file  lcd.c
 * @brief Implementación del driver LCD HD44780 para AVR ATmega.
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include "lcd.h"
#include <avr/io.h>
#include <util/delay.h>

/* =========================================================================
 * Construcción de nombres de registros AVR a partir de la letra del puerto
 *
 * Se necesitan dos niveles de macros para que el preprocesador expanda
 * el argumento antes de concatenarlo.
 *
 *   Ejemplo: _REG(DDR, LCD_RS_PORT)
 *            → _REG2(DDR, D)
 *            → DDRD
 * ========================================================================= */
#define _REG2(prefix, port)  prefix##port
#define _REG(prefix, port)   _REG2(prefix, port)

/* Registros de los pines de control */
#define _RS_DDR   _REG(DDR,  LCD_RS_PORT)
#define _RS_PORT  _REG(PORT, LCD_RS_PORT)
#define _EN_DDR   _REG(DDR,  LCD_EN_PORT)
#define _EN_PORT  _REG(PORT, LCD_EN_PORT)

/* Registros del bus de datos */
#define _DATA_DDR   _REG(DDR,  LCD_DATA_PORT)
#define _DATA_PORT  _REG(PORT, LCD_DATA_PORT)

/* Registro RW (solo si se usa) */
#if LCD_USE_RW
#  define _RW_DDR   _REG(DDR,  LCD_RW_PORT)
#  define _RW_PORT  _REG(PORT, LCD_RW_PORT)
#endif

/* =========================================================================
 * Macros de manejo de pines
 * ========================================================================= */
#define _PIN_OUT(ddr,  bit)  ((ddr)  |=  (1 << (bit)))
#define _PIN_HIGH(port, bit) ((port) |=  (1 << (bit)))
#define _PIN_LOW(port,  bit) ((port) &= ~(1 << (bit)))

/* =========================================================================
 * Estado interno del driver
 * ========================================================================= */
static uint8_t _display_ctrl;
static uint8_t _entry_mode;

/* =========================================================================
 * Comandos HD44780 (uso interno)
 * ========================================================================= */
#define _CMD_CLEAR        0x01
#define _CMD_HOME         0x02
#define _CMD_ENTRY_MODE   0x04
#define _CMD_DISPLAY      0x08
#define _CMD_SHIFT        0x10
#define _CMD_FUNCTION     0x20
#define _CMD_CGRAM        0x40
#define _CMD_DDRAM        0x80

#define _ENTRY_LEFT       0x02
#define _ENTRY_SHIFT      0x01

#define _DISPLAY_ON       0x04
#define _CURSOR_ON        0x02
#define _BLINK_ON         0x01

#define _DISPLAY_MOVE     0x08
#define _MOVE_RIGHT       0x04

#define _BUS_8BIT         0x10
#define _2LINE            0x08

/* =========================================================================
 * Funciones internas
 * ========================================================================= */

static void _pulse_enable(void)
{
    _PIN_LOW(_EN_PORT,  LCD_EN_PIN);
    _delay_us(1);
    _PIN_HIGH(_EN_PORT, LCD_EN_PIN);
    _delay_us(1);     /* pulso EN alto: mínimo 230 ns */
    _PIN_LOW(_EN_PORT,  LCD_EN_PIN);
    _delay_us(50);    /* espera ejecución: máximo 37 µs */
}

/* Escribe 4 bits en el bus y genera el pulso de Enable.
   Solo modifica los 4 bits del bus; el resto del puerto queda intacto. */
static void _write_nibble(uint8_t nibble)
{
    uint8_t mask = (uint8_t)(0x0F << LCD_DATA_OFFSET);
    _DATA_PORT = (_DATA_PORT & ~mask) | ((nibble & 0x0F) << LCD_DATA_OFFSET);
    _pulse_enable();
}

/* Escribe 8 bits en el bus y genera el pulso de Enable. */
static void _write_byte(uint8_t data)
{
    uint8_t mask = (uint8_t)(0xFF << LCD_DATA_OFFSET);
    _DATA_PORT = (_DATA_PORT & ~mask) | ((data & 0xFF) << LCD_DATA_OFFSET);
    _pulse_enable();
}

/* Envía un byte completo al controlador.
   rs = 0 → comando,  rs = 1 → dato. */
static void _send(uint8_t value, uint8_t rs)
{
#if LCD_USE_RW
    _PIN_LOW(_RW_PORT, LCD_RW_PIN);
#endif

    if (rs) _PIN_HIGH(_RS_PORT, LCD_RS_PIN);
    else    _PIN_LOW(_RS_PORT,  LCD_RS_PIN);

#if LCD_MODE == 8
    _write_byte(value);
#else
    _write_nibble(value >> 4);   /* nibble alto primero */
    _write_nibble(value & 0x0F); /* nibble bajo         */
#endif
}

/* =========================================================================
 * API pública
 * ========================================================================= */

void lcd_command(uint8_t cmd)
{
    _send(cmd, 0);
    if (cmd == _CMD_CLEAR || cmd == _CMD_HOME) {
        _delay_ms(2); /* Clear y Home tardan hasta 1.52 ms */
    }
}

void lcd_write_char(char c)
{
    _send((uint8_t)c, 1);
}

void lcd_init(void)
{
    /* Configurar pines de control como salidas */
    _PIN_OUT(_RS_DDR, LCD_RS_PIN);
    _PIN_OUT(_EN_DDR, LCD_EN_PIN);
#if LCD_USE_RW
    _PIN_OUT(_RW_DDR, LCD_RW_PIN);
    _PIN_LOW(_RW_PORT, LCD_RW_PIN);
#endif
    _PIN_LOW(_RS_PORT, LCD_RS_PIN);
    _PIN_LOW(_EN_PORT, LCD_EN_PIN);

    /* Configurar bus de datos como salida */
#if LCD_MODE == 8
    _DATA_DDR |= (uint8_t)(0xFF << LCD_DATA_OFFSET);
#else
    _DATA_DDR |= (uint8_t)(0x0F << LCD_DATA_OFFSET);
#endif

    /* Estado inicial */
    _display_ctrl = _DISPLAY_ON;
    _entry_mode   = _ENTRY_LEFT;

    /* ---------------------------------------------------------------
     * Secuencia de inicialización HD44780 (datasheet pág. 45–46)
     *
     * El controlador siempre arranca en modo 8-bit. Para pasar a 4-bit
     * se necesita una secuencia especial de nibbles individuales antes
     * de poder enviar comandos completos.
     * --------------------------------------------------------------- */
    _delay_ms(50); /* esperar que VCC suba a 4.5 V */

#if LCD_MODE == 8
    _write_byte(0x30); _delay_ms(5);   /* intento 1: Function Set 8-bit */
    _write_byte(0x30); _delay_us(150); /* intento 2 */
    _write_byte(0x30); _delay_us(150); /* intento 3 */
    lcd_command(_CMD_FUNCTION | _BUS_8BIT | (LCD_ROWS > 1 ? _2LINE : 0));
#else
    _write_nibble(0x03); _delay_ms(5);   /* intento 1 */
    _write_nibble(0x03); _delay_us(150); /* intento 2 */
    _write_nibble(0x03); _delay_us(150); /* intento 3 */
    _write_nibble(0x02); _delay_us(150); /* cambio a 4-bit */
    lcd_command(_CMD_FUNCTION | (LCD_ROWS > 1 ? _2LINE : 0));
#endif

    lcd_command(_CMD_DISPLAY);              /* display OFF */
    lcd_clear();                            /* borrar      */
    lcd_command(_CMD_ENTRY_MODE | _entry_mode);
    lcd_command(_CMD_DISPLAY | _display_ctrl); /* display ON  */
}

void lcd_clear(void)      { lcd_command(_CMD_CLEAR); }
void lcd_home(void)       { lcd_command(_CMD_HOME);  }

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if (row >= LCD_ROWS) row = LCD_ROWS - 1;
    if (col >= LCD_COLS) col = LCD_COLS - 1;
    lcd_command(_CMD_DDRAM | (col + row_offsets[row]));
}

void lcd_write_string(const char *str)
{
    while (*str) lcd_write_char(*str++);
}

void lcd_write_int(int32_t value)
{
    char    buf[11];
    uint8_t i = 0;

    if (value == 0) { lcd_write_char('0'); return; }

    if (value < 0) {
        lcd_write_char('-');
        if (value == (int32_t)0x80000000) { lcd_write_string("2147483648"); return; }
        value = -value;
    }

    while (value > 0) { buf[i++] = '0' + (char)(value % 10); value /= 10; }
    while (i > 0)     { lcd_write_char(buf[--i]); }
}

/* --- Control del display ------------------------------------------------- */

void lcd_display_on(void)  { _display_ctrl |=  _DISPLAY_ON; lcd_command(_CMD_DISPLAY | _display_ctrl); }
void lcd_display_off(void) { _display_ctrl &= ~_DISPLAY_ON; lcd_command(_CMD_DISPLAY | _display_ctrl); }
void lcd_cursor_on(void)   { _display_ctrl |=  _CURSOR_ON;  lcd_command(_CMD_DISPLAY | _display_ctrl); }
void lcd_cursor_off(void)  { _display_ctrl &= ~_CURSOR_ON;  lcd_command(_CMD_DISPLAY | _display_ctrl); }
void lcd_blink_on(void)    { _display_ctrl |=  _BLINK_ON;   lcd_command(_CMD_DISPLAY | _display_ctrl); }
void lcd_blink_off(void)   { _display_ctrl &= ~_BLINK_ON;   lcd_command(_CMD_DISPLAY | _display_ctrl); }

/* --- Desplazamiento ------------------------------------------------------ */

void lcd_scroll_left(void)  { lcd_command(_CMD_SHIFT | _DISPLAY_MOVE);                  }
void lcd_scroll_right(void) { lcd_command(_CMD_SHIFT | _DISPLAY_MOVE | _MOVE_RIGHT);    }
void lcd_cursor_left(void)  { lcd_command(_CMD_SHIFT);                                  }
void lcd_cursor_right(void) { lcd_command(_CMD_SHIFT | _MOVE_RIGHT);                    }

/* --- Dirección de escritura ---------------------------------------------- */

void lcd_left_to_right(void)  { _entry_mode |=  _ENTRY_LEFT;  lcd_command(_CMD_ENTRY_MODE | _entry_mode); }
void lcd_right_to_left(void)  { _entry_mode &= ~_ENTRY_LEFT;  lcd_command(_CMD_ENTRY_MODE | _entry_mode); }
void lcd_autoscroll_on(void)  { _entry_mode |=  _ENTRY_SHIFT; lcd_command(_CMD_ENTRY_MODE | _entry_mode); }
void lcd_autoscroll_off(void) { _entry_mode &= ~_ENTRY_SHIFT; lcd_command(_CMD_ENTRY_MODE | _entry_mode); }

/* --- Caracteres personalizados ------------------------------------------- */

void lcd_create_char(uint8_t location, uint8_t charmap[8])
{
    location &= 0x07;
    lcd_command(_CMD_CGRAM | (location << 3));
    for (uint8_t i = 0; i < 8; i++) _send(charmap[i], 1);
    /* El puntero queda en CGRAM; llamar lcd_home() o lcd_set_cursor() a continuación. */
}
