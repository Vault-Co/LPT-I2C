/*
 * System:  <???>
 * File:    iic.c
 *          Software implementation for I2C bus master
 *
 */

#include "iic.h"

/*
 * Machine/compiler dependencies. Low level bit banging can be implemented
 * as macros or functions depending on the hardware. This file will
 * use the following macro/function names everywhere.
 */

#ifdef ITSYBITSYBOX

#include <io51.h> 

#define SCL_PIN         P1.0        /* For 8051 / IAR C */
#define SDA_PIN         P1.1

#define I2C_TIMEOUTCONST    5           /* 1/100ths of a sec to wait on bus */
#define SCL_H()         {SCL_PIN = 1;}  /* SCL high */
#define SCL_L()         {SCL_PIN = 0;}  /* Pull SCL down */
#define SDA_H()         {SDA_PIN = 1;}  /* SDA high */
#define SDA_L()         {SDA_PIN = 0;}  /* Pull SDA down */
#define SDA_OUT(b)      {SDA_PIN = (b);}/* Put bit b on SDA */
#define SCL_IN()        (SCL_PIN)       /* Returns current status of SCL */
#define SDA_IN()        (SDA_PIN)       /* Ditto for SDA */

volatile data byte i2c_timer;
volatile bit i2c_timeout;

#define i2c_reset_timeout() {i2c_timer = I2C_TIMEOUTCONST;}

static code char rcsid[] = "$Id: iic.c 1.1 95/03/03 13:53:06 mi Exp $";

#else

#include <dos.h>                    /* PC compilers - low level functions */
                                    /*   for i2c are in iic_pc.c */
extern void SCL_H(void);
extern void SCL_L(void);
extern void SDA_H(void);
extern void SDA_L(void);
extern void SDA_OUT(bit b);
extern bit SCL_IN(void);
extern bit SDA_IN(void);

volatile bit i2c_timeout = 0;       /* Set by i2c timer routine */

extern void i2c_delay(void);            /* Basic i2c timing atom */
extern void i2c_reset_timeout(void);    /* Resets bus timeout counter - */
                                        /*   i2c_timeout will be set on timeout */
static char rcsid[] = "$Id: iic.c 1.1 95/03/03 13:53:06 mi Exp $";

#endif

byte i2c_xmit_buf[I2C_BUFSIZE];     /* Global transfer buffers */
byte i2c_recv_buf[I2C_BUFSIZE];
byte i2c_error = 0;                 /* Last error */


/*
 * Makes sure that the bus is in a known condition. Returns 1 on success,
 * 0 if some other device is pulling on the bus.
 */

bit i2c_reset(void)
{
    SCL_H();
    SDA_H();
    i2c_error = 0;
    return (SCL_IN() && SDA_IN());
}


/*
 * Generates a start condition on the bus. Returns 0 on success, 1 if some
 * other device is holding the bus.
 */

bit i2c_start(void)
{
    i2c_delay();
    SDA_L();        /* Pull SDA down... */
    i2c_delay();
    SCL_L();        /* ...and then SCL -> start condition. */
    i2c_delay();
    return 0;
}


/*
 * Generates a stop condition on the bus. Returns 0 on success, 1 if some
 * other device is holding the bus.
 */

bit i2c_stop(void)
{
    SCL_H();        /* Let SCL go up */
    i2c_delay();
    SDA_H();        /* ...and then SDA up -> stop condition. */
    i2c_delay();
    
    return (SCL_IN() && SDA_IN());  /* Both will be up, if everything is fine */
}


/*
 * Clock out one bit.
 * Returns 0 on success, 1 if we lose arbitration.
 */

bit i2c_bit_out(bit bout)
{
    SDA_OUT(bout);              /* Put data out on SDA */
    i2c_delay();
    SCL_H();                    /* Let SCL go up */
    i2c_reset_timeout();
    while(!SCL_IN())            /* Wait until all other devices are ready */
    {
        if (i2c_timeout)        /* Timeout, return and set error flag */
        {
            i2c_error = I2CERR_TIMEOUT;
            return 1;
        }
    }
        
    if (SDA_IN() != bout)       /* Arbitration lost, release bus and return */
    {
        SDA_H();                /* Should be up anyway, but make sure */
        i2c_error = I2CERR_LOST;
        return 1;
    }
    i2c_delay();
    SCL_L();                    /* Pull SCL back down */
    i2c_delay();                
    return 0;                   /* OK */
}


/*
 * Clock in one bit.
 */

bit i2c_bit_in(void)
{
    bit bin;
    
    SCL_H();                    /* Let SCL go up */
    i2c_reset_timeout();
    while(!SCL_IN())            /* Wait for other devices */
    {
        if (i2c_timeout)        /* Timeout, return and set error flag */
        {
            i2c_error = I2CERR_TIMEOUT;
            return 1;
        }
    }
    bin = SDA_IN();             /* Read in data */
    i2c_delay();
    SCL_L();                    /* Pull SCL back up */
    i2c_delay();
    return bin;                 /* Return the sampled bit */
}


/*
 * Send one byte on the bus. No start or stop conditions are generated here,
 * but i2c_error will be set according to the result.
 * Returns 0 on success, 1 if we lose arbitration or if the slave doesn't
 * acknowledge the byte. Check i2c_error for the actual result on error.
 */

bit i2c_byte_out(byte dat)
{
    byte bit_count;
    
    bit_count = 8;
    while(bit_count)
    {
        if (dat & 0x80)
        {
            if (i2c_bit_out(1)) return 1;
        }
        else
        {
            if (i2c_bit_out(0)) return 1;
        }
        dat <<= 1;
        bit_count--;
    }
    
    if (i2c_bit_in())
    {
        i2c_error = I2CERR_NAK;
        return 1;
    }
    return 0;
}


/*
 * Reads one byte in from the slave. Ack must be 1 if this is the last byte
 * to be read during this transfer, 0 otherwise (as per I2C bus specification,
 * the receiving master must acknowledge all but the last byte during a
 * transfer).
 */
 
byte i2c_byte_in(bit ack)
{
    byte bit_count, byte_in;

    bit_count = 8;
    byte_in = 0;
    
    while(bit_count)
    {
        byte_in <<= 1;
        if (i2c_bit_in()) byte_in |= 0x01;
        bit_count--;
    }

    i2c_bit_out(ack);
    SDA_H();             /* Added 18-Jul-95 - thanks to Ray Bellis */
    return byte_in;
}


/*
 * Send 'count' bytes to slave 'addr'.
 * Returns 0 on success. Stop condition is sent only when send_stop is true.
 */

bit i2c_send_ex(byte addr, byte count, bit send_stop)
{
    byte byteptr, byte_out;
    
    if (i2c_start()) return 1;
    i2c_error = 0;

    byte_out = addr & 0xfe;     /* Ensure that it's a write address */
    count++;                    /* Include slave address to byte count */
    byteptr = 0;
    while(count)
    {
        if (i2c_byte_out(byte_out))
        {
#ifdef DEBUG
            printf("i2c_send byte %d: ", byteptr);
            i2c_dump_error(i2c_error);
#endif
            if (i2c_error == I2CERR_NAK && send_stop) i2c_stop();
            return i2c_error;
        }
        byte_out = i2c_xmit_buf[byteptr];
        byteptr++;
        count--;
    }

    if (send_stop) i2c_stop();
    return 0;
}


/*
 * Read in 'count' bytes from slave 'addr'.
 * Returns 0 on success.
 */

bit i2c_recv(byte addr, byte count)
{
    byte byteptr, byte_in;

    if (i2c_start()) return 1;
    i2c_error = 0;
    byteptr = 0;

    byte_in = addr | 0x01;

    if (i2c_byte_out(byte_in))
    {
        if (i2c_error == I2CERR_NAK) i2c_stop();
        return i2c_error;
    }

    while(count)
    {
        if (--count)
        {
            byte_in = i2c_byte_in(0);
        }
        else
        {
            byte_in = i2c_byte_in(1);   /* No ACK during last byte */
        }
        i2c_recv_buf[byteptr] = byte_in;
        byteptr++;
    }

    i2c_stop();
    
    return (i2c_error ? 1 : 0);
}


/*
 * Write 'tx_count' bytes to slave 'addr', then use a repeated start condition
 * to read 'rx_count' bytes from the same slave during the same transfer.
 * Returns 0 on success, 1 otherwise. On error, check i2c_error for the actual
 * error value.
 */

bit i2c_send_recv(byte addr, byte tx_count, byte rx_count)
{
    if (i2c_send_ex(addr, tx_count, 0))
    {
        /* If send fails, abort but don't send a stop condition if we lost
           arbitration */

        if (i2c_error != I2CERR_LOST) i2c_stop();
        return 1;
    }

    SDA_H();        /* One of these may be low now, in which case the next */
    SCL_H();        /*   start condition wouldn't be detected so make */
    i2c_delay();    /*   sure that they're up and wait for one delay slot */
    
    if (i2c_recv(addr | 0x01, rx_count)) return 1;

    return (i2c_error ? 1 : 0);
}
