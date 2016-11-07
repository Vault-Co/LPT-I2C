/*
 * File:    iic.h
 *          i2c bitbanging interface
 */

#ifndef _IIC_H
#define _IIC_H
 
#ifdef _IIC_C
static char rcsid_h[] = "$Id: iic.h 1.1 95/03/03 13:53:11 mi Exp $";
#endif

/*
 * Compiler/machine dependencies
 */

#ifndef __TYPES_DEFINED
#define __TYPES_DEFINED
typedef unsigned char byte;
typedef unsigned int word;
#endif

#if defined( __MSDOS__ ) || defined ( _MSDOS )
typedef unsigned char bit;      /* Bit type is built in in IAR C */
#define I2C_BUFSIZE     128
#else
#define ITSYBITSYBOX
#define I2C_BUFSIZE     8       /* Conserve memory on itsy-bitsy boxes */
#endif

/*
 * Prototypes for functions in iic.c (and iic51.s03)
 */

bit i2c_reset(void);
bit i2c_start(void);
bit i2c_stop(void);
bit i2c_send_ex(byte addr, byte count, bit send_stop);
bit i2c_recv(byte addr, byte count);
bit i2c_send_recv(byte addr, byte tx_count, byte rx_count);
bit i2c_byte_out(byte);

/*
 * Macro for normal send transfer ending with a stop condition
 */

#define i2c_send(addr, count)   i2c_send_ex(addr, count, 1)

/*
 * Global variables in iic.c
 */

extern byte i2c_xmit_buf[];     /* Transmit buffer */
extern byte i2c_recv_buf[];     /* Receive buffer */
extern byte i2c_error;          /* Set to last error value, see below */
extern volatile bit i2c_timeout;
 
/*
 * I2C error values
 */

#define I2CERR_OK       0       /* No error */
#define I2CERR_NAK      1       /* No ACK from slave */
#define I2CERR_LOST     2       /* Arbitration lost */
#define I2CERR_BUS      3       /* Bus is stuck (not used yet) */
#define I2CERR_TIMEOUT  4       /* Timeout on bus */

#endif
