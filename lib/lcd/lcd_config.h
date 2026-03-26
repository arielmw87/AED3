/**
 * @file  lcd_config.h
 * @brief Configuración del driver LCD HD44780.
 *
 * ESTE ES EL ÚNICO ARCHIVO QUE HAY QUE EDITAR.
 *
 * Indicar la letra del puerto (A, B, C o D) y el número de bit (0–7)
 * para cada señal del LCD.
 */

#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

/* ============================================================
 * MODO DE INTERFAZ
 *   4  → usa 4 pines de datos (D4–D7 del LCD)
 *   8  → usa 8 pines de datos (D0–D7 del LCD)
 * ============================================================ */
#define LCD_MODE    4

/* ============================================================
 * DIMENSIONES DEL DISPLAY
 * ============================================================ */
#define LCD_COLS   16
#define LCD_ROWS    2

/* ============================================================
 * PINES DE CONTROL
 *
 *  Escribir la LETRA del puerto (A, B, C o D) y el NÚMERO de
 *  bit (0–7) para cada señal.
 *
 *  Ejemplo: RS conectado a PD2  →  LCD_RS_PORT D  /  LCD_RS_PIN 2
 * ============================================================ */

/* RS — Register Select */
#define LCD_RS_PORT   D
#define LCD_RS_PIN    2

/* EN — Enable */
#define LCD_EN_PORT   D
#define LCD_EN_PIN    4

/* RW — Read/Write (opcional)
 *   LCD_USE_RW 0  →  RW conectado a GND (recomendado, ahorra un pin)
 *   LCD_USE_RW 1  →  RW controlado por el microcontrolador           */
#define LCD_USE_RW    0
#define LCD_RW_PORT   D   /* ignorado si LCD_USE_RW = 0 */
#define LCD_RW_PIN    3   /* ignorado si LCD_USE_RW = 0 */

/* ============================================================
 * BUS DE DATOS
 *
 *  Los pines deben ser CONSECUTIVOS dentro del MISMO puerto.
 *
 *  LCD_DATA_PORT   → letra del puerto (A, B, C o D)
 *  LCD_DATA_OFFSET → número del bit inicial en ese puerto
 *
 *  Modo 4-bit: se usan 4 pines (bit inicial hasta inicial+3)
 *              representan D4–D7 del LCD.
 *  Modo 8-bit: se usan 8 pines (bit inicial hasta inicial+7)
 *              representan D0–D7 del LCD.
 *
 *  Ejemplos:
 *    D4–D7 del LCD en PD4–PD7 → PORT D, OFFSET 4
 *    D4–D7 del LCD en PB0–PB3 → PORT B, OFFSET 0
 *    D0–D7 del LCD en PB0–PB7 → PORT B, OFFSET 0  (modo 8-bit)
 * ============================================================ */
#define LCD_DATA_PORT    B
#define LCD_DATA_OFFSET  0

/* ============================================================
 * VERIFICACIONES EN TIEMPO DE COMPILACIÓN
 * ============================================================ */
#if LCD_MODE != 4 && LCD_MODE != 8
#  error "lcd_config.h: LCD_MODE debe ser 4 u 8"
#endif

#if LCD_MODE == 4 && LCD_DATA_OFFSET > 4
#  error "lcd_config.h: en modo 4-bit, LCD_DATA_OFFSET no puede ser mayor que 4"
#endif

#if LCD_MODE == 8 && LCD_DATA_OFFSET != 0
#  error "lcd_config.h: en modo 8-bit, LCD_DATA_OFFSET debe ser 0"
#endif

#if LCD_ROWS < 1 || LCD_ROWS > 4
#  error "lcd_config.h: LCD_ROWS debe estar entre 1 y 4"
#endif

#endif /* LCD_CONFIG_H */
