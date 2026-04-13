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
#include <avr/interrupt.h>

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
    UCSR0B = (1 << TXEN0) | (1 << RXEN0)
#if defined(USART_FRAME_USE_DELIMITERS) || defined(USART_FRAME_USE_TIMEOUT)
           | (1 << RXCIE0)   /* habilitar interrupción de RX para recepción de tramas */
#endif
    ;

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

/* =========================================================================
 * Recepción de tramas
 *
 * Todo este bloque se compila solo si hay un modo de trama activo.
 * ========================================================================= */
#if defined(USART_FRAME_USE_DELIMITERS) || defined(USART_FRAME_USE_TIMEOUT)

/* -------------------------------------------------------------------------
 * Estados internos de la máquina de estados de recepción
 *
 * Solo accesibles dentro de este archivo (privados).
 * ------------------------------------------------------------------------- */
#define _FRAME_IDLE       0   /* esperando el inicio de una trama            */
#define _FRAME_RECEIVING  1   /* acumulando bytes de la trama                */
#define _FRAME_READY      2   /* trama completa, esperando que la lean       */

static uint8_t  _frame_state  = _FRAME_IDLE;
static uint8_t  _frame_buf[USART_FRAME_BUF_SIZE];
static uint8_t  _frame_len    = 0;

#ifdef USART_FRAME_USE_TIMEOUT
static uint16_t _frame_timer  = 0;   /* cuenta regresiva en ms hasta fin de trama */
#endif

/* -------------------------------------------------------------------------
 * Procesa un byte recibido y actualiza la máquina de estados.
 * Se llama exclusivamente desde la ISR de recepción (USART_RX_vect).
 *
 * La máquina tiene tres estados:
 *
 *   IDLE ──── inicio detectado ────► RECEIVING ──── fin detectado ────► READY
 *    ▲                                                                      │
 *    └──────────────────── usart_frame_get() libera ──────────────────────┘
 *
 * Qué es "inicio" y "fin" depende del modo configurado:
 *   DELIMITADORES → carácter START_CHAR  /  carácter STOP_CHAR
 *   TIMEOUT       → cualquier byte       /  silencio de N ms
 * ------------------------------------------------------------------------- */
static void _frame_procesar_byte(uint8_t byte)
{
    /* Si ya hay una trama lista sin leer, ignorar bytes nuevos.
     * Esto evita pisar el buffer antes de que el usuario lo consuma. */
    if (_frame_state == _FRAME_READY)
        return;

#ifdef USART_FRAME_USE_DELIMITERS

    if (_frame_state == _FRAME_IDLE) {

        /* Esperar el carácter de inicio (a menos que NO_START esté definido) */
#ifndef USART_FRAME_NO_START
        if (byte != USART_FRAME_START_CHAR)
            return;     /* todavía no llega el inicio: ignorar */
        /* El carácter de inicio no se guarda en el buffer */
#else
        /* Sin carácter de inicio: cualquier byte comienza la trama.
         * Este primer byte sí se incluye en el buffer. */
        _frame_buf[0] = byte;
        _frame_len    = 1;
#endif
        _frame_state = _FRAME_RECEIVING;
        return;
    }

    if (_frame_state == _FRAME_RECEIVING) {

        if (byte == USART_FRAME_STOP_CHAR) {
            /* Llegó el carácter de fin: trama completa */
            _frame_state = _FRAME_READY;
            return;
        }

        /* Acumular el byte si hay lugar en el buffer */
        if (_frame_len < USART_FRAME_BUF_SIZE)
            _frame_buf[_frame_len++] = byte;
        else
            _frame_state = _FRAME_READY;   /* buffer lleno: cerrar la trama */
    }

#endif /* USART_FRAME_USE_DELIMITERS */

#ifdef USART_FRAME_USE_TIMEOUT

    /* En modo timeout cualquier byte (re)inicia el contador de silencio */
    _frame_timer = USART_FRAME_TIMEOUT_MS;

    if (_frame_state == _FRAME_IDLE) {
        _frame_len   = 0;
        _frame_state = _FRAME_RECEIVING;
    }

    /* Acumular el byte si hay lugar en el buffer */
    if (_frame_len < USART_FRAME_BUF_SIZE)
        _frame_buf[_frame_len++] = byte;
    else
        _frame_state = _FRAME_READY;   /* buffer lleno: cerrar la trama */

#endif /* USART_FRAME_USE_TIMEOUT */
}

/* -------------------------------------------------------------------------
 * ISR de recepción USART
 *
 * Se dispara automáticamente cada vez que llega un byte completo.
 * Lee UDR0 inmediatamente (si no se lee, el hardware lo descarta)
 * y lo pasa a la máquina de estados.
 * ------------------------------------------------------------------------- */
ISR(USART_RX_vect)
{
    uint8_t byte = UDR0;   /* leer SIEMPRE para limpiar el flag de interrupción */
    _frame_procesar_byte(byte);
}

/* -------------------------------------------------------------------------
 * API pública de tramas
 * ------------------------------------------------------------------------- */

void usart_tick(void)
{
#ifdef USART_FRAME_USE_TIMEOUT
    /* Este código corre desde la ISR del timer (1 kHz = cada 1 ms).
     * Si hay una trama en curso, decrementar el contador de silencio.
     * Cuando llega a 0, la trama se da por terminada. */
    if (_frame_state == _FRAME_RECEIVING && _frame_timer > 0) {
        _frame_timer--;
        if (_frame_timer == 0)
            _frame_state = _FRAME_READY;
    }
#endif
}

uint8_t usart_frame_ready(void)
{
    return (_frame_state == _FRAME_READY) ? 1 : 0;
}

uint8_t usart_frame_len(void)
{
    return _frame_len;
}

uint8_t usart_frame_get(uint8_t *buf, uint8_t max_len)
{
    uint8_t i;
    uint8_t n;

    if (_frame_state != _FRAME_READY)
        return 0;

    /* Copiar hasta max_len bytes al buffer del usuario */
    n = (_frame_len < max_len) ? _frame_len : max_len;
    for (i = 0; i < n; i++)
        buf[i] = _frame_buf[i];

    /* Liberar el receptor para la próxima trama */
    _frame_len   = 0;
    _frame_state = _FRAME_IDLE;

    return n;
}

/* =========================================================================
 * Versión vacía para cuando el modo de tramas está desactivado.
 * De esta forma util.c puede llamar usart_tick() siempre, sin condiciones.
 * ========================================================================= */
#else

void usart_tick(void)    { /* modo de tramas desactivado */ }
uint8_t usart_frame_ready(void)                   { return 0; }
uint8_t usart_frame_len(void)                     { return 0; }
uint8_t usart_frame_get(uint8_t *buf, uint8_t max_len) { (void)buf; (void)max_len; return 0; }

#endif /* USART_FRAME_USE_DELIMITERS || USART_FRAME_USE_TIMEOUT */
