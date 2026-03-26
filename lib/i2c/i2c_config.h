/**
 * @file  i2c_config.h
 * @brief Configuración del driver I2C (TWI) para ATmega328P.
 *
 * ESTE ES EL ÚNICO ARCHIVO QUE HAY QUE EDITAR.
 *
 * A diferencia del bus SPI o el paralelo, los pines SDA y SCL del
 * módulo TWI del ATmega328P están fijos en hardware:
 *
 *   SDA → PC4  (pin A4 en Arduino Nano/Uno)
 *   SCL → PC5  (pin A5 en Arduino Nano/Uno)
 *
 * No hay nada que configurar en cuanto a pines. Solo ajustar
 * la velocidad del bus y el timeout de seguridad.
 */

#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

/* ============================================================
 * VELOCIDAD DEL BUS I2C
 *
 *  Valores típicos:
 *    100000UL  →  100 kHz  (modo estándar, máxima compatibilidad)
 *    400000UL  →  400 kHz  (modo rápido)
 *
 *  El valor real depende de F_CPU. Para F_CPU = 16 MHz:
 *    100 kHz → TWBR = 72
 *    400 kHz → TWBR = 12
 * ============================================================ */
#define I2C_FREQ   100000UL

/* ============================================================
 * TIMEOUT DE SEGURIDAD
 *
 *  Número máximo de iteraciones esperando al flag TWINT antes
 *  de abortar y devolver I2C_ERR_TIMEOUT.
 *
 *  Evita que el programa quede colgado si SDA o SCL se quedan
 *  bloqueados (por ejemplo, si un dispositivo no responde o hay
 *  un problema en el bus).
 *
 *    0  →  sin timeout (espera infinita, no recomendado)
 * ============================================================ */
#define I2C_TIMEOUT   10000

/* ============================================================
 * VERIFICACIONES EN TIEMPO DE COMPILACIÓN
 * ============================================================ */
#ifndef F_CPU
#  error "F_CPU no está definido. Definirlo antes de incluir i2c_config.h"
#endif

#if I2C_FREQ == 0
#  error "i2c_config.h: I2C_FREQ no puede ser 0"
#endif

/* TWBR con prescaler = 1 (TWPS = 0) */
#define _I2C_TWBR  ((F_CPU / I2C_FREQ - 16) / 2)

#if _I2C_TWBR < 10
#  error "i2c_config.h: I2C_FREQ demasiado alta para este F_CPU (TWBR mínimo: 10)"
#endif

#if _I2C_TWBR > 255
#  error "i2c_config.h: I2C_FREQ demasiado baja para este F_CPU (TWBR máximo: 255)"
#endif

#endif /* I2C_CONFIG_H */
