/*
 * iic_pc.c
 * Low level bit banging for parallel/I2C interface
 *
 */
 
#include <dos.h>
#include "iic.h"

#ifndef __TURBOC__
#include <conio.h>
#define MK_FP(seg, ofs)     ( (void __far *) ((((unsigned long)(seg)) << 16) + (ofs)))
#else
#define _outp(port, dat)    outportb((port), (dat))
#define _inp(port)          inportb((port))
#define _dos_setvect(vec,h) setvect(vec,h)
#define _dos_getvect(vec)   getvect(vec)
#define __interrupt         interrupt
#define __far               far
#endif
 
static int lpt;                     /* Port to use */
static int baseaddr[3];             /* LPT base addresses */
static unsigned char byt = 0x04;    /* Current data in control port */
static int timercount;

static char rcsid[] = "$Id: iic_pc.c 1.2 95/03/03 13:55:15 mi Exp $";

void __interrupt __far new1c(void);
void (__interrupt __far *old1c)();

#define I2C_TIMEOUT     6           /* Clock ticks before timeout on bus wait states */

/*
 * Macros to make actual banging easier to write
 */

#define STATUS_IN()     (_inp(baseaddr[lpt]+1))
#define CTRL_IN()       (_inp(baseaddr[lpt]+1))
#define STATUS_OUT(b)   (_outp(baseaddr[lpt]+2, (b)))
#define CTRL_OUT(b)     (_outp(baseaddr[lpt]+2, (b)))

/*
 * Prototypes for public functions
 */
 
void SCL_H(void);       /* These will be called by iic.c */
void SCL_L(void);
void SDA_H(void);
void SDA_L(void);
void SDA_OUT(bit b);
bit SCL_IN(void);
bit SDA_IN(void);
void i2c_init(int);     /* ..and these by the main program */
void i2c_uninit(void);


/*
 * Interrupt handler for timeouts. This uses the DOS 0x1c timer, which is
 * normally called at approximately 18.2 Hz rate. Decrements timercount
 * until it reaches zero, and then sets the timeout flag from iic.c.
 */

void __interrupt __far new1c(void)
{
    (*old1c)();         /* Call old handler to keep DOS happy */

    if (timercount) timercount--;
    else i2c_timeout = 1;
}


/*
 * Resets and restarts the bus timeout timer.
 */

void i2c_reset_timeout(void)
{
    timercount = I2C_TIMEOUT;
    i2c_timeout = 0;
}


/*
 * General i2c delay between most transitions.
 */

void i2c_delay(void)
{
#if 1
    int n;

    n = 4000;           /* This delay constant seems to work on a 486DX-50. */
    while(n--);
#else
    timercount = 2;     /* Use this if you're willing to spend 1 sec */
    while(timercount);  /*   for transmitting a single damn byte. */
#endif
}


/*
 * Queries the BIOS data table for LPT base addresses. LPT base I/O addresses
 * are stored in consecutive words starting at 0040:0008 (LPT1).
 */

void get_addresses(void)
{
    baseaddr[0] = *((int far *)MK_FP(0x40, 0x08));
    baseaddr[1] = *((int far *)MK_FP(0x40, 0x0a));
    baseaddr[2] = *((int far *)MK_FP(0x40, 0x0c));
}


/*
 * Initialises the port and timer. In this implementation, param holds the
 * zero-based number of the LPT port to use.
 */

void i2c_init(int param)
{
    get_addresses();
    lpt = param;
    
    old1c = _dos_getvect(0x1c);         /* Chain to our timer handler */
    _dos_setvect(0x1c, new1c);
    
    _outp(baseaddr[lpt], 0xff);         /* Supply power to adapter */
    i2c_reset();                        /* Put SCL & SDA to idle state */
}


/*
 * Uninitialises the port and timer.
 */
 
void i2c_uninit(void)
{
    _dos_setvect(0x1c, old1c);          /* Restore original interrupt handler */
    _outp(baseaddr[lpt], 0);            /* Power off */
}


/*
 * Dump an error message.
 */

void i2c_dump_error(int error)
{
    switch(error)
    {
      case 0:
        puts("OK.");
        break;
      case I2CERR_NAK:
        puts("Slave didn't acknowledge");
        break;
      case I2CERR_LOST:
        puts("Lost arbitration with another master");
        break;
      case I2CERR_TIMEOUT:
        puts("Timeout on bus");
        break;
      case I2CERR_BUS:
        puts("The bus is stuck");
        break;
    }
}


/*
 * Bit banging functions. Setting a parallel port output pin high pulls
 * the respective wire low on the I2C bus, while the inputs give
 * the I2C bus state as is. Note that SCL output is connected to the INIT
 * pin, which is inverting.
 */

void SCL_H(void)
{
    byt |= 0x04;
    CTRL_OUT(byt);
}

void SCL_L(void)
{
    byt &= ~0x04;
    CTRL_OUT(byt);
}

void SDA_H(void)
{
    byt &= ~0x01;
    CTRL_OUT(byt);
}

void SDA_L(void)
{
    byt |= 0x01;
    CTRL_OUT(byt);
}

void SDA_OUT(bit b)
{
    if (b) SDA_H();
    else SDA_L();
}

bit SCL_IN(void)
{
    if (STATUS_IN() & 0x80) return 1;
    else return 0;
}

bit SDA_IN(void)
{
    if (STATUS_IN() & 0x20) return 0;
    else return 1;
}
