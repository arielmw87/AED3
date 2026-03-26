/**
 * @file  lcd.h
 * @brief Driver para display LCD alfanumérico con controlador HD44780.
 *
 * Configurar los pines en lcd_config.h antes de usar esta biblioteca.
 */

#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "lcd_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * API pública
 * ========================================================================= */

/** Inicializa el LCD. Debe llamarse antes que cualquier otra función. */
void lcd_init(void);

/** Borra el display y mueve el cursor a la posición (0, 0). */
void lcd_clear(void);

/** Mueve el cursor a (0, 0) sin borrar el contenido. */
void lcd_home(void);

/**
 * Posiciona el cursor.
 * @param col  Columna (0 = izquierda).
 * @param row  Fila    (0 = primera fila).
 */
void lcd_set_cursor(uint8_t col, uint8_t row);

/**
 * Escribe un carácter en la posición actual del cursor.
 * Para mostrar un carácter personalizado usar su índice (0–7):
 *   lcd_write_char(0);
 */
void lcd_write_char(char c);

/** Escribe una cadena terminada en '\0'. */
void lcd_write_string(const char *str);

/** Escribe un número entero con signo en base decimal. */
void lcd_write_int(int32_t value);

/* --- Control del display ------------------------------------------------- */

void lcd_display_on(void);   /**< Enciende el display.            */
void lcd_display_off(void);  /**< Apaga el display.               */
void lcd_cursor_on(void);    /**< Muestra el cursor (subrayado).  */
void lcd_cursor_off(void);   /**< Oculta el cursor.               */
void lcd_blink_on(void);     /**< Activa el parpadeo del cursor.  */
void lcd_blink_off(void);    /**< Desactiva el parpadeo.          */

/* --- Desplazamiento ------------------------------------------------------ */

void lcd_scroll_left(void);   /**< Desplaza el contenido a la izquierda.  */
void lcd_scroll_right(void);  /**< Desplaza el contenido a la derecha.    */
void lcd_cursor_left(void);   /**< Mueve el cursor un lugar a la izquierda.  */
void lcd_cursor_right(void);  /**< Mueve el cursor un lugar a la derecha.    */

/* --- Dirección de escritura ---------------------------------------------- */

void lcd_left_to_right(void);   /**< Escritura de izquierda a derecha (por defecto). */
void lcd_right_to_left(void);   /**< Escritura de derecha a izquierda.               */
void lcd_autoscroll_on(void);   /**< El display se desplaza al escribir.             */
void lcd_autoscroll_off(void);  /**< Desactiva el desplazamiento automático.         */

/* --- Caracteres personalizados ------------------------------------------- */

/**
 * Crea un carácter personalizado y lo almacena en la CGRAM.
 *
 * El HD44780 admite hasta 8 caracteres propios (posiciones 0–7).
 * Cada carácter se define con 8 bytes; los 5 bits menos significativos
 * de cada byte representan una fila de píxeles (de arriba a abajo).
 *
 * @param location  Posición en CGRAM (0–7).
 * @param charmap   Array de 8 bytes con el bitmap del carácter.
 *
 * @note Llamar a lcd_home() o lcd_set_cursor() después de esta función
 *       antes de continuar escribiendo texto.
 */
void lcd_create_char(uint8_t location, uint8_t charmap[8]);

/* --- Acceso de bajo nivel ------------------------------------------------ */

/**
 * Envía un byte de comando al controlador HD44780.
 * Para uso avanzado; normalmente no es necesario.
 *
 * Comandos útiles:
 *   0x01  Clear display
 *   0x02  Return home
 *   0x0C  Display ON, cursor OFF
 *   0x0E  Display ON, cursor ON
 *   0x0F  Display ON, cursor ON, blink ON
 */
void lcd_command(uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* LCD_H */
