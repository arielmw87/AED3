/**
 * @file  usart.h
 * @brief Driver USART para ATmega328P.
 *
 * Configurar velocidad y formato en usart_config.h antes de usar
 * esta biblioteca.
 *
 * Pines (fijos en hardware, no configurables):
 *   TXD → PD1  (D1 en Arduino Nano/Uno)
 *   RXD → PD0  (D0 en Arduino Nano/Uno)
 */

#ifndef USART_H
#define USART_H

#include <stdint.h>
#include <arduino.h>
#include "usart_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * API pública
 * ========================================================================= */

/** Inicializa la USART con la velocidad y formato configurados. */
void usart_init(void);

/* --- Transmisión --------------------------------------------------------- */

/**
 * Envía un carácter. Bloquea hasta que el registro de transmisión
 * esté libre.
 */
void usart_send_char(char c);

/** Envía una cadena terminada en '\0'. */
void usart_send_string(const char *str);

/** Envía una cadena seguida de salto de línea ("\r\n"). */
void usart_send_line(const char *str);

/** Envía un número entero con signo en base decimal. */
void usart_send_int(int32_t value);

/**
 * Envía un número entero sin signo en base hexadecimal.
 * Siempre muestra exactamente `digits` dígitos (con ceros a la izquierda).
 *
 * @param value   Valor a mostrar.
 * @param digits  Cantidad de dígitos hex a mostrar (1–8).
 *
 * Ejemplo: usart_send_hex(0xAB, 4)  →  "00AB"
 */
void usart_send_hex(uint32_t value, uint8_t digits);

/* --- Recepción ----------------------------------------------------------- */

/**
 * Indica si hay un byte disponible en el buffer de recepción.
 * @return  1 si hay datos, 0 si no.
 */
uint8_t usart_available(void);

/**
 * Lee un byte del buffer de recepción sin bloquear.
 * Verificar con usart_available() antes de llamar, o usar usart_recv_wait().
 *
 * @param c  Puntero donde se guardará el carácter recibido.
 * @return   1 si se leyó un byte, 0 si el buffer estaba vacío.
 */
uint8_t usart_recv(char *c);

/**
 * Lee un byte bloqueando hasta que llegue uno.
 * @return  Carácter recibido.
 */
char usart_recv_wait(void);

#ifdef __cplusplus
}
#endif

#endif /* USART_H */
