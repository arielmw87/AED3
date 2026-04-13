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

/* =========================================================================
 * Recepción de tramas
 *
 * Disponible cuando se activa USART_FRAME_USE_DELIMITERS o
 * USART_FRAME_USE_TIMEOUT en usart_config.h.
 *
 * Internamente usa la interrupción de recepción (RXCIE0) para
 * procesar cada byte apenas llega, sin perder datos a cualquier
 * velocidad de baud rate.
 *
 * Una "trama" es una secuencia de bytes con inicio y fin bien definidos:
 *   - Modo DELIMITADORES: el inicio lo marca USART_FRAME_START_CHAR
 *                         y el fin, USART_FRAME_STOP_CHAR.
 *   - Modo TIMEOUT:       el inicio lo marca el primer byte recibido
 *                         y el fin, el silencio de USART_FRAME_TIMEOUT_MS.
 *
 * Flujo de uso:
 *
 *   while (1) {
 *       uint8_t buf[USART_FRAME_BUF_SIZE];
 *
 *       if (usart_frame_ready()) {
 *           uint8_t len = usart_frame_get(buf, sizeof(buf));
 *           // procesar buf[0..len-1]
 *       }
 *   }
 * ========================================================================= */

/**
 * Actualiza el detector de tramas.
 * Llamar desde la ISR del timer de 1 kHz (ver util.c).
 *
 * En modo TIMEOUT decrementa el contador de silencio y marca
 * la trama completa cuando se agota. En modo DELIMITADORES no
 * hace nada, pero puede llamarse igual sin problema.
 */
void usart_tick(void);

/**
 * Devuelve 1 si hay una trama completa lista para leer.
 * Devuelve 0 si todavía no llegó ninguna trama.
 */
uint8_t usart_frame_ready(void);

/**
 * Devuelve la cantidad de bytes de la trama disponible.
 * Solo válido si usart_frame_ready() devuelve 1.
 */
uint8_t usart_frame_len(void);

/**
 * Copia la trama al buffer del usuario y libera el receptor
 * para recibir la próxima trama.
 *
 * Si max_len es menor que la trama, se copian solo los primeros
 * max_len bytes (el resto se descarta de todas formas).
 *
 * @param buf      Buffer de destino.
 * @param max_len  Tamaño máximo del buffer.
 * @return         Bytes copiados, o 0 si no hay trama lista.
 */
uint8_t usart_frame_get(uint8_t *buf, uint8_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* USART_H */
