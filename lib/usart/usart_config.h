/**
 * @file  usart_config.h
 * @brief Configuración del driver USART para ATmega328P.
 *
 * ESTE ES EL ÚNICO ARCHIVO QUE HAY QUE EDITAR.
 *
 * Los pines TXD y RXD están fijos en hardware:
 *
 *   TXD → PD1  (pin D1 en Arduino Nano/Uno)
 *   RXD → PD0  (pin D0 en Arduino Nano/Uno)
 */

#ifndef USART_CONFIG_H
#define USART_CONFIG_H

/* ============================================================
 * VELOCIDAD (BAUD RATE)
 *
 *  Valores comunes: 9600, 19200, 38400, 57600, 115200
 *
 *  El modo U2X (doble velocidad) se selecciona automáticamente
 *  cuando mejora la precisión del baud rate.
 * ============================================================ */
#define USART_BAUD      9600UL

/* ============================================================
 * FORMATO DE TRAMA
 *
 *  La configuración más habitual y compatible es 8N1:
 *    8 bits de datos, sin paridad, 1 bit de stop.
 * ============================================================ */
#define USART_DATA_BITS  8   /* bits de datos: 5, 6, 7 u 8          */
#define USART_PARITY     0   /* paridad: 0=ninguna, 1=par, 2=impar  */
#define USART_STOP_BITS  1   /* bits de stop: 1 o 2                 */

/* ============================================================
 * VERIFICACIONES EN TIEMPO DE COMPILACIÓN
 * ============================================================ */
#ifndef F_CPU
#  error "F_CPU no está definido."
#endif

#if USART_DATA_BITS < 5 || USART_DATA_BITS > 8
#  error "usart_config.h: USART_DATA_BITS debe ser 5, 6, 7 u 8"
#endif

#if USART_PARITY < 0 || USART_PARITY > 2
#  error "usart_config.h: USART_PARITY debe ser 0 (ninguna), 1 (par) o 2 (impar)"
#endif

#if USART_STOP_BITS != 1 && USART_STOP_BITS != 2
#  error "usart_config.h: USART_STOP_BITS debe ser 1 o 2"
#endif

/* Calcula UBRR para modo normal y doble velocidad,
   y selecciona automáticamente el que da menos error. */
#define _UBRR_N    (F_CPU / (16UL * USART_BAUD) - 1)
#define _UBRR_D    (F_CPU / (8UL  * USART_BAUD) - 1)
#define _REAL_N    (F_CPU / (16UL * (_UBRR_N + 1)))
#define _REAL_D    (F_CPU / (8UL  * (_UBRR_D + 1)))
#define _ERR_N     ((_REAL_N > USART_BAUD) ? (_REAL_N - USART_BAUD) : (USART_BAUD - _REAL_N))
#define _ERR_D     ((_REAL_D > USART_BAUD) ? (_REAL_D - USART_BAUD) : (USART_BAUD - _REAL_D))

#define _USART_U2X   ((_ERR_D < _ERR_N) ? 1 : 0)
#define _USART_UBRR  ((_ERR_D < _ERR_N) ? _UBRR_D : _UBRR_N)

#if _USART_UBRR > 4095
#  error "usart_config.h: USART_BAUD demasiado bajo para este F_CPU"
#endif

#endif /* USART_CONFIG_H */
