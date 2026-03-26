/**
 * @file  usart.c
 * @brief Implementación del driver USART para ATmega328P.
 *
 * TXD → PD1  |  RXD → PD0  (pines fijos, no configurables por software).
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include "usart.h"
#include <avr/io.h>

/* =========================================================================
 * Valor de UCSRC según la configuración de formato
 * ========================================================================= */

/* Bits de datos (UCSZ1:UCSZ0 en UCSRC, UCSZ2 en UCSRB — siempre 0 para ≤8 bits) */
#if   USART_DATA_BITS == 5
#  define _UCSZ  (0)
#elif USART_DATA_BITS == 6
#  define _UCSZ  (1 << UCSZ00)
#elif USART_DATA_BITS == 7
#  define _UCSZ  (1 << UCSZ01)
#else  /* 8 bits */
#  define _UCSZ  ((1 << UCSZ01) | (1 << UCSZ00))
#endif

/* Paridad */
#if   USART_PARITY == 1  /* par */
#  define _UPM  (1 << UPM01)
#elif USART_PARITY == 2  /* impar */
#  define _UPM  ((1 << UPM01) | (1 << UPM00))
#else                    /* ninguna */
#  define _UPM  (0)
#endif

/* Bits de stop */
#if USART_STOP_BITS == 2
#  define _USBS  (1 << USBS0)
#else
#  define _USBS  (0)
#endif

/* =========================================================================
 * API pública
 * ========================================================================= */

void usart_init(void)
{
    /* Baud rate — valor y modo (normal / doble velocidad) calculados
       en tiempo de compilación en usart_config.h */
    UBRR0H = (uint8_t)(_USART_UBRR >> 8);
    UBRR0L = (uint8_t)(_USART_UBRR);

#if _USART_U2X
    UCSR0A = (1 << U2X0);
#else
    UCSR0A = 0;
#endif

    /* Habilitar transmisor y receptor */
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    /* Formato de trama: modo asíncrono + bits de datos + paridad + stop */
    UCSR0C = (0 << UMSEL01) | (0 << UMSEL00)   /* asíncrono */
           | _UPM
           | _USBS
           | _UCSZ;
}

/* --- Transmisión --------------------------------------------------------- */

void usart_send_char(char c)
{
    /* Esperar a que el registro de transmisión esté vacío */
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = (uint8_t)c;
}

void usart_send_string(const char *str)
{
    while (*str) usart_send_char(*str++);
}

void usart_send_line(const char *str)
{
    usart_send_string(str);
    usart_send_char('\r');
    usart_send_char('\n');
}

void usart_send_int(int32_t value)
{
    char    buf[11];
    uint8_t i = 0;

    if (value == 0) { usart_send_char('0'); return; }

    if (value < 0) {
        usart_send_char('-');
        if (value == (int32_t)0x80000000) { usart_send_string("2147483648"); return; }
        value = -value;
    }

    while (value > 0) { buf[i++] = '0' + (char)(value % 10); value /= 10; }
    while (i > 0)     { usart_send_char(buf[--i]); }
}

void usart_send_hex(uint32_t value, uint8_t digits)
{
    if (digits == 0 || digits > 8) digits = 8;

    /* Construir dígitos de más significativo a menos */
    for (int8_t i = (int8_t)(digits - 1); i >= 0; i--) {
        uint8_t nibble = (uint8_t)((value >> (i * 4)) & 0x0F);
        usart_send_char(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    }
}

/* --- Recepción ----------------------------------------------------------- */

uint8_t usart_available(void)
{
    return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

uint8_t usart_recv(char *c)
{
    if (!usart_available()) return 0;
    *c = (char)UDR0;
    return 1;
}

char usart_recv_wait(void)
{
    while (!usart_available());
    return (char)UDR0;
}
