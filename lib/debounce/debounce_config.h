/**
 * @file  debounce_config.h
 * @brief Configuración del debounce de pulsadores.
 *
 * ESTE ES EL ÚNICO ARCHIVO QUE HAY QUE EDITAR para adaptar la biblioteca
 * a un proyecto nuevo.
 *
 * Cómo agregar o quitar pulsadores
 * ---------------------------------
 *  1. Cambiar NUM_BOTONES al número de pulsadores que se van a usar.
 *  2. Agregar o quitar filas BOTON(...) en la tabla config[] de debounce.c.
 *
 * Formato de cada fila en debounce.c:
 *
 *   BOTON( puerto , bit , activo_bajo )
 *            │       │        └── 1 = el pin lee 0 cuando el botón está pulsado
 *            │       │               (conexión a GND con pullup, la más habitual)
 *            │       │            0 = el pin lee 1 cuando el botón está pulsado
 *            │       └────────── nombre del bit: PD2, PB0, PC3, etc.
 *            └────────────────── letra del puerto AVR: B, C o D
 *
 * Ejemplos:
 *   BOTON(D, PD2, 1)   →  Arduino pin 2,  pulsador conectado a GND
 *   BOTON(D, PD3, 1)   →  Arduino pin 3,  pulsador conectado a GND
 *   BOTON(B, PB0, 1)   →  Arduino pin 8,  pulsador conectado a GND
 */

#ifndef DEBOUNCE_CONFIG_H
#define DEBOUNCE_CONFIG_H

/* ============================================================
 * CANTIDAD DE PULSADORES
 *
 * Debe coincidir con la cantidad de filas BOTON(...) en debounce.c
 * ============================================================ */
#define NUM_BOTONES  2

/* ============================================================
 * VELOCIDAD DE MUESTREO
 *
 * Cada cuántos ticks del timer de 1 kHz se toma una muestra.
 * Determina cuánto rebote tolera la biblioteca:
 *
 *   TICKS_POR_MUESTRA = 5  →  1 muestra cada  5 ms
 *   TICKS_POR_MUESTRA = 8  →  1 muestra cada  8 ms
 *
 * Con 3 bits de "don't care" en la máscara:
 *   zona de rebote tolerada = 3 × TICKS_POR_MUESTRA (en ms)
 *
 *   Con 5 → tolera 15 ms de rebote   (pulsadores de buena calidad)
 *   Con 8 → tolera 24 ms de rebote   (pulsadores más ruidosos)
 *
 * No bajar de 3 ni subir de 20.
 * ============================================================ */
#define DEBOUNCE_TICKS_POR_MUESTRA  5

/* ============================================================
 * VERIFICACIONES EN TIEMPO DE COMPILACIÓN
 * ============================================================ */
#if NUM_BOTONES < 1
#  error "debounce_config.h: NUM_BOTONES debe ser al menos 1"
#endif

#if DEBOUNCE_TICKS_POR_MUESTRA < 3
#  error "debounce_config.h: DEBOUNCE_TICKS_POR_MUESTRA demasiado bajo (mínimo 3)"
#endif

#if DEBOUNCE_TICKS_POR_MUESTRA > 20
#  error "debounce_config.h: DEBOUNCE_TICKS_POR_MUESTRA demasiado alto (máximo 20)"
#endif

#endif /* DEBOUNCE_CONFIG_H */
