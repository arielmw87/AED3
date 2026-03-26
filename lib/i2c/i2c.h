/**
 * @file  i2c.h
 * @brief Driver I2C (TWI) en modo master para ATmega328P.
 *
 * Configurar la velocidad del bus en i2c_config.h antes de usar
 * esta biblioteca.
 *
 * Pines (fijos en hardware, no configurables):
 *   SDA → PC4  (A4 en Arduino Nano/Uno)
 *   SCL → PC5  (A5 en Arduino Nano/Uno)
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "i2c_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Códigos de retorno
 *
 * Todas las funciones que pueden fallar devuelven uno de estos valores.
 * Un retorno distinto de I2C_OK indica que la operación no se completó
 * y el bus puede haber quedado en un estado inconsistente.
 * ========================================================================= */
#define I2C_OK           0   /**< Operación exitosa.                        */
#define I2C_ERR_START    1   /**< No se pudo enviar la condición START.     */
#define I2C_ERR_ADDR     2   /**< El dispositivo no respondió (NACK en dir.)*/
#define I2C_ERR_DATA     3   /**< El dispositivo envió NACK durante datos.  */
#define I2C_ERR_TIMEOUT  4   /**< Se agotó el timeout esperando al bus.     */

/* Bit R/W dentro de la dirección I2C */
#define I2C_WRITE  0
#define I2C_READ   1

/* =========================================================================
 * API de bajo nivel
 *
 * Permiten construir cualquier secuencia I2C manualmente.
 * El flujo habitual es:
 *
 *   i2c_start(addr, I2C_WRITE)  ← START + dirección
 *   i2c_write(reg)              ← registro o dato
 *   i2c_write(value)            ← más datos si hace falta
 *   i2c_stop()                  ← STOP
 *
 * Para leer después de escribir (patrón "write-then-read"):
 *
 *   i2c_start(addr, I2C_WRITE)  ← START
 *   i2c_write(reg)              ← registro a leer
 *   i2c_start(addr, I2C_READ)   ← REPEATED START (sin STOP previo)
 *   i2c_read(I2C_ACK)           ← leer byte, pedir más
 *   i2c_read(I2C_NACK)          ← leer último byte
 *   i2c_stop()                  ← STOP
 * ========================================================================= */

#define I2C_ACK   1   /**< Enviar ACK tras recibir un byte (hay más bytes). */
#define I2C_NACK  0   /**< Enviar NACK tras recibir un byte (último byte).  */

/**
 * Inicializa el módulo TWI con la velocidad configurada en i2c_config.h.
 * Debe llamarse antes que cualquier otra función.
 */
void i2c_init(void);

/**
 * Envía una condición START (o REPEATED START) seguida de la dirección
 * del dispositivo y el bit R/W.
 *
 * Para REPEATED START llamar sin haber enviado STOP previamente.
 *
 * @param addr  Dirección I2C del dispositivo (7 bits, sin el bit R/W).
 * @param rw    I2C_WRITE (0) o I2C_READ (1).
 * @return      I2C_OK, I2C_ERR_START, I2C_ERR_ADDR o I2C_ERR_TIMEOUT.
 */
uint8_t i2c_start(uint8_t addr, uint8_t rw);

/**
 * Envía un byte de datos. Solo válido en modo escritura (tras i2c_start
 * con I2C_WRITE).
 *
 * @param data  Byte a enviar.
 * @return      I2C_OK, I2C_ERR_DATA o I2C_ERR_TIMEOUT.
 */
uint8_t i2c_write(uint8_t data);

/**
 * Recibe un byte de datos. Solo válido en modo lectura (tras i2c_start
 * con I2C_READ).
 *
 * @param ack   I2C_ACK  → enviar ACK  (hay más bytes que leer).
 *              I2C_NACK → enviar NACK (este es el último byte).
 * @return      Byte recibido.
 */
uint8_t i2c_read(uint8_t ack);

/**
 * Envía la condición STOP y libera el bus.
 * Llamar siempre al terminar una transacción.
 */
void i2c_stop(void);

/* =========================================================================
 * API de alto nivel
 *
 * Encapsulan las operaciones más comunes en una sola llamada.
 * Gestionan automáticamente START, dirección, datos y STOP.
 * En caso de error envían STOP y devuelven el código de error.
 * ========================================================================= */

/**
 * Envía uno o más bytes a un dispositivo I2C.
 *
 * Secuencia: START → SLA+W → data[0] → ... → data[len-1] → STOP
 *
 * @param addr  Dirección I2C del dispositivo (7 bits).
 * @param data  Puntero al buffer de datos a enviar.
 * @param len   Número de bytes a enviar.
 * @return      I2C_OK o código de error.
 */
uint8_t i2c_write_to(uint8_t addr, const uint8_t *data, uint8_t len);

/**
 * Lee uno o más bytes de un dispositivo I2C.
 *
 * Secuencia: START → SLA+R → buf[0] → ... → buf[len-1] → STOP
 *
 * @param addr  Dirección I2C del dispositivo (7 bits).
 * @param buf   Buffer donde se almacenarán los bytes recibidos.
 * @param len   Número de bytes a leer.
 * @return      I2C_OK o código de error.
 */
uint8_t i2c_read_from(uint8_t addr, uint8_t *buf, uint8_t len);

/**
 * Escribe un byte en un registro de un dispositivo I2C.
 * Patrón habitual de sensores y periféricos.
 *
 * Secuencia: START → SLA+W → reg → value → STOP
 *
 * @param addr   Dirección I2C del dispositivo (7 bits).
 * @param reg    Dirección del registro.
 * @param value  Valor a escribir.
 * @return       I2C_OK o código de error.
 */
uint8_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);

/**
 * Lee un byte de un registro de un dispositivo I2C.
 * Usa REPEATED START para escribir el registro y luego leer.
 *
 * Secuencia: START → SLA+W → reg → RSTART → SLA+R → *value → STOP
 *
 * @param addr   Dirección I2C del dispositivo (7 bits).
 * @param reg    Dirección del registro.
 * @param value  Puntero donde se guardará el byte leído.
 * @return       I2C_OK o código de error.
 */
uint8_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value);

/**
 * Lee múltiples bytes consecutivos a partir de un registro.
 * Útil para leer datos compuestos (ej: acelerómetro XYZ en 6 bytes).
 *
 * Secuencia: START → SLA+W → reg → RSTART → SLA+R → buf[0..len-1] → STOP
 *
 * @param addr  Dirección I2C del dispositivo (7 bits).
 * @param reg   Registro inicial.
 * @param buf   Buffer de destino.
 * @param len   Número de bytes a leer.
 * @return      I2C_OK o código de error.
 */
uint8_t i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H */
