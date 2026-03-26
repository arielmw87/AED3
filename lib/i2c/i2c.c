/**
 * @file  i2c.c
 * @brief Implementación del driver I2C (TWI) en modo master para ATmega328P.
 *
 * Usa el módulo TWI por hardware del ATmega328P.
 * SDA → PC4  |  SCL → PC5  (pines fijos, no configurables por software).
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include "i2c.h"
#include <avr/io.h>
#include <util/twi.h>   /* Constantes de estado TWI: TW_START, TW_MT_SLA_ACK, etc. */

/* =========================================================================
 * Espera a que el hardware TWI termine la operación actual (flag TWINT).
 *
 * Devuelve 1 si TWINT se activó correctamente.
 * Devuelve 0 si se agotó el timeout (solo si I2C_TIMEOUT > 0).
 * ========================================================================= */
static uint8_t _twi_wait(void)
{
#if I2C_TIMEOUT > 0
    uint16_t t = (uint16_t)I2C_TIMEOUT;
    while (!(TWCR & (1 << TWINT))) {
        if (--t == 0) return 0;
    }
#else
    while (!(TWCR & (1 << TWINT)));
#endif
    return 1;
}

/* Lee el código de estado del bus (los 5 bits altos de TWSR). */
static inline uint8_t _twi_status(void)
{
    return (TWSR & 0xF8);
}

/* =========================================================================
 * API de bajo nivel
 * ========================================================================= */

void i2c_init(void)
{
    /* Prescaler = 1 (TWPS1=0, TWPS0=0) */
    TWSR = 0x00;

    /* Bit rate: SCL = F_CPU / (16 + 2 * TWBR * prescaler)
       Con prescaler=1: TWBR = (F_CPU/SCL - 16) / 2
       El valor se calcula en tiempo de compilación en i2c_config.h */
    TWBR = (uint8_t)_I2C_TWBR;

    /* Habilitar el módulo TWI */
    TWCR = (1 << TWEN);
}

uint8_t i2c_start(uint8_t addr, uint8_t rw)
{
    /* Enviar condición START */
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    if (!_twi_wait()) return I2C_ERR_TIMEOUT;

    /* Verificar que START o REPEATED START se enviaron correctamente */
    uint8_t st = _twi_status();
    if (st != TW_START && st != TW_REP_START) return I2C_ERR_START;

    /* Enviar dirección + bit R/W */
    TWDR = (uint8_t)((addr << 1) | (rw & 0x01));
    TWCR = (1 << TWINT) | (1 << TWEN);

    if (!_twi_wait()) return I2C_ERR_TIMEOUT;

    /* Verificar ACK en la dirección */
    st = _twi_status();
    if (rw == I2C_WRITE && st != TW_MT_SLA_ACK) return I2C_ERR_ADDR;
    if (rw == I2C_READ  && st != TW_MR_SLA_ACK) return I2C_ERR_ADDR;

    return I2C_OK;
}

uint8_t i2c_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    if (!_twi_wait())              return I2C_ERR_TIMEOUT;
    if (_twi_status() != TW_MT_DATA_ACK) return I2C_ERR_DATA;

    return I2C_OK;
}

uint8_t i2c_read(uint8_t ack)
{
    /* TWEA=1 → responder con ACK (hay más bytes).
       TWEA=0 → responder con NACK (último byte). */
    if (ack == I2C_ACK)
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    else
        TWCR = (1 << TWINT) | (1 << TWEN);

    /* Si el timeout se agota devolvemos 0xFF como valor inválido.
       El llamador debe verificar el código de retorno de i2c_read_from
       o de las funciones de alto nivel, no el dato en sí. */
    if (!_twi_wait()) return 0xFF;

    return TWDR;
}

void i2c_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    /* No es necesario esperar TWINT: TWSTO se limpia automáticamente
       cuando la condición STOP se completa en el bus. */
}

/* =========================================================================
 * API de alto nivel
 * ========================================================================= */

uint8_t i2c_write_to(uint8_t addr, const uint8_t *data, uint8_t len)
{
    uint8_t err;

    err = i2c_start(addr, I2C_WRITE);
    if (err) { i2c_stop(); return err; }

    for (uint8_t i = 0; i < len; i++) {
        err = i2c_write(data[i]);
        if (err) { i2c_stop(); return err; }
    }

    i2c_stop();
    return I2C_OK;
}

uint8_t i2c_read_from(uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint8_t err;

    err = i2c_start(addr, I2C_READ);
    if (err) { i2c_stop(); return err; }

    for (uint8_t i = 0; i < len; i++) {
        /* ACK en todos los bytes excepto el último */
        buf[i] = i2c_read(i < len - 1 ? I2C_ACK : I2C_NACK);
    }

    i2c_stop();
    return I2C_OK;
}

uint8_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t err;

    err = i2c_start(addr, I2C_WRITE);
    if (err) { i2c_stop(); return err; }

    err = i2c_write(reg);
    if (err) { i2c_stop(); return err; }

    err = i2c_write(value);
    if (err) { i2c_stop(); return err; }

    i2c_stop();
    return I2C_OK;
}

uint8_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value)
{
    uint8_t err;

    /* Escribir el número de registro */
    err = i2c_start(addr, I2C_WRITE);
    if (err) { i2c_stop(); return err; }

    err = i2c_write(reg);
    if (err) { i2c_stop(); return err; }

    /* REPEATED START y cambiar a modo lectura */
    err = i2c_start(addr, I2C_READ);
    if (err) { i2c_stop(); return err; }

    *value = i2c_read(I2C_NACK);   /* único byte → NACK */

    i2c_stop();
    return I2C_OK;
}

uint8_t i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t err;

    err = i2c_start(addr, I2C_WRITE);
    if (err) { i2c_stop(); return err; }

    err = i2c_write(reg);
    if (err) { i2c_stop(); return err; }

    /* REPEATED START y cambiar a modo lectura */
    err = i2c_start(addr, I2C_READ);
    if (err) { i2c_stop(); return err; }

    for (uint8_t i = 0; i < len; i++) {
        buf[i] = i2c_read(i < len - 1 ? I2C_ACK : I2C_NACK);
    }

    i2c_stop();
    return I2C_OK;
}
