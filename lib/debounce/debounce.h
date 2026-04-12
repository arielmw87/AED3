/**
 * @file  debounce.h
 * @brief Debounce de pulsadores por software — "Ultimate Debouncer".
 *
 * Basado en el algoritmo de Jack Ganssle, adaptado por Elliot Williams
 * (Hackaday, diciembre 2015).
 *
 * Referencias:
 *   https://hackaday.com/2015/12/09/embed-with-elliot-debounce-your-noisy-buttons-part-i/
 *   https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
 *   https://www.ganssle.com/debouncing-pt2.htm
 *
 * Configurar los pines y la velocidad de muestreo en debounce_config.h
 * antes de usar esta biblioteca.
 *
 * Integración con el timer del proyecto
 * --------------------------------------
 * La biblioteca necesita que se la llame a intervalos regulares.
 * El proyecto ya tiene un timer a 1 kHz configurado en util.c.
 * Alcanza con agregar una línea en la ISR de ese timer:
 *
 *   ISR(TIMER0_COMPA_vect) {
 *       debounce_tick();
 *   }
 *
 * Cómo funciona el algoritmo
 * ---------------------------
 * Cada pulsador mantiene un registro de desplazamiento de 8 bits (historial).
 * En cada llamado a debounce_tick() se descarta la muestra más vieja y se
 * inserta la nueva al final:
 *
 *   historial antes:  [ b7  b6  b5  b4  b3  b2  b1  b0 ]
 *   historial después:[ b6  b5  b4  b3  b2  b1  b0  nueva_muestra ]
 *
 * Para detectar si el botón fue pulsado se aplica una máscara que ignora
 * la zona media (donde ocurre el rebote) y evalúa solo los extremos:
 *
 *   historial: [  S   S   x   x   x   N   N   N  ]
 *                  ↑                   ↑
 *              estable previo       nuevo estable
 *              (estaba suelto)      (ya está pulsado)
 *
 *   máscara = 1 1 0 0 0 1 1 1 = 0xC7
 *
 *   Pulsado  cuando  (historial & máscara) == 0b00000111
 *   Soltado  cuando  (historial & máscara) == 0b11000000
 *
 * Al detectar el evento, el historial se rellena con 1s o 0s para que la
 * máscara no vuelva a disparar hasta que haya una nueva transición real.
 * Esto actúa como histéresis y evita falsos re-disparos.
 *
 * Temporización
 * -------------
 *   DEBOUNCE_TICKS_POR_MUESTRA = 5  →  1 muestra cada 5 ms
 *   3 bits de don't care × 5 ms    → 15 ms de rebote tolerado
 *   8 muestras × 5 ms              → 40 ms de ventana de historial total
 */

#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>
#include <arduino.h>
#include "debounce_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Máscara del algoritmo
 *
 * Selecciona los 2 bits previos (bits 7-6) y los 3 bits nuevos (bits 2-0).
 * Los bits 5-3 (zona de rebote) quedan en don't care (cero en la máscara).
 *
 *   bit:   7  6  5  4  3  2  1  0
 *          1  1  0  0  0  1  1  1   =  0xC7
 * ========================================================================= */
#define DEBOUNCE_MASK  ((uint8_t)0b11000111)

/* =========================================================================
 * API pública
 * ========================================================================= */

/**
 * Inicializa todos los pulsadores.
 * Configura los pines como entradas según la tabla de debounce.c
 * Llamar una vez al inicio, antes del loop principal.
 */
void debounce_init(void);

/**
 * Actualiza el historial de todos los pulsadores.
 * Llamar desde la ISR del timer de 1 kHz (TIMER0_COMPA_vect en util.c).
 * No llamar desde el loop principal.
 */
void debounce_tick(void);

/**
 * Devuelve 1 si el pulsador fue presionado desde la última consulta.
 * El evento se consume al leer: devuelve 1 solo una vez por pulsación.
 *
 * @param id  Índice del pulsador (0 a NUM_BOTONES - 1).
 * @return    1 si se acaba de pulsar, 0 si no.
 */
uint8_t debounce_pulsado(uint8_t id);

/**
 * Devuelve 1 si el pulsador fue soltado desde la última consulta.
 * El evento se consume al leer: devuelve 1 solo una vez por liberación.
 *
 * @param id  Índice del pulsador (0 a NUM_BOTONES - 1).
 * @return    1 si se acaba de soltar, 0 si no.
 */
uint8_t debounce_soltado(uint8_t id);

/**
 * Devuelve el estado actual del pulsador sin consumir ningún evento.
 *
 * @param id  Índice del pulsador (0 a NUM_BOTONES - 1).
 * @return    1 si está presionado, 0 si está suelto.
 */
uint8_t debounce_esta_abajo(uint8_t id);

/**
 * Devuelve cuántas muestras consecutivas lleva el pulsador presionado.
 * Se resetea a 0 al soltar. Útil para detectar pulsaciones largas.
 *
 * Para convertir a milisegundos:
 *   ms = debounce_ticks_sostenido(id) * DEBOUNCE_TICKS_POR_MUESTRA
 *
 * Ejemplo: un resultado mayor a 200 con TICKS_POR_MUESTRA = 5
 * significa que el botón lleva más de 1 segundo pulsado.
 *
 * @param id  Índice del pulsador (0 a NUM_BOTONES - 1).
 * @return    Muestras consecutivas en estado pulsado.
 */
uint16_t debounce_ticks_sostenido(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif /* DEBOUNCE_H */
