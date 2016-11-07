/*
 * simple unit test
 */

#include <conio.h>
#include "iic.h"

#define LIMIT       256
#define BLOCKSIZE   8

#ifndef __TURBOC__
#include <graph.h>
#define gotoxy(a, b)    _settextposition((b), (a))
#define clrscr()        _clearscreen(_GCLEARSCREEN)
#endif

void bang_test(void)
{
    bit old_sda, old_scl, scl, sda;
    int ch = 0;
    
    old_scl = SCL_IN();
    old_sda = SDA_IN();
    
    clrscr();
    cprintf("SCL = %d\n\r", SCL_IN());
    cprintf("SDA = %d", SDA_IN());
    
    for(;;)
    {
        scl = SCL_IN();
        sda = SDA_IN();
        
        if (old_scl != scl || old_sda != sda)
        {
            gotoxy(1, 1);
            cprintf("SCL = %d\n\r", scl);
            cprintf("SDA = %d", sda);
            old_scl = scl;
            old_sda = sda;
        }
        
        if (kbhit()) switch(ch = getch())
        {
          case 27:
            return;
            
          case 'z':
            SCL_L();
            break;
            
          case 'a':
            SCL_H();
            break;
            
          case 'x':
            SDA_L();
            break;
            
          case 's':
            SDA_H();
            break;

          default:
            i2c_start();
            if (!i2c_byte_out(0x40)) i2c_byte_out(ch);
            else
            {
                clrscr();
                cprintf("%d", i2c_error);
                break;
            }
            if (i2c_error != I2CERR_LOST) i2c_stop();
            break;
        }
    }
}

void write_test(void)
{
    int n;

    cprintf("Doing write test...\n\r");

    for(n = 0; n < LIMIT; n++)
    {
        i2c_delay();
        i2c_xmit_buf[0] = n;

        if (i2c_send(0x40, 1))
        {
            cprintf("write: n = %d, error = %d\n\r", n, i2c_error);
        }
    }
}

void write_block_test(void)
{
    int n, m;

    cprintf("Doing write test with %d byte blocks...\n\r", BLOCKSIZE);

    n = 0;
    while(n < LIMIT)
    {
        for(m = 0; m < BLOCKSIZE; m++)
        {
            i2c_xmit_buf[m] = n++;
        }
        i2c_delay();

        if (i2c_send(0x40, BLOCKSIZE))
        {
            cprintf("write: n = %d, error = %d\n\r", n, i2c_error);
        }
    }
}


void write_and_read_test(void)
{
    int n;

    cprintf("Doing write-followed-by-read test...\n\r");

    for(n = 0; n < LIMIT; n++)
    {
        i2c_delay();
        i2c_xmit_buf[0] = n;

        if (i2c_send(0x40, 1))
        {
            cprintf("write: n = %d, error = %d\n\r", n, i2c_error);
        }
        i2c_delay();

        if (i2c_recv(0x40, 1))
        {
            cprintf("read: n = %d, error = %d\n\r", n, i2c_error);
	}
	if (i2c_recv_buf[0] != n) cprintf("read: got %d\n\r", i2c_recv_buf[0]);
    }
}

void combined_read_write_test(void)
{
    int n;
    
    cprintf("Doing combined read-and-write test...\n\r");
    for(n = 0; n < LIMIT; n++)
    {
        i2c_delay();
        i2c_xmit_buf[0] = n;
        if (i2c_send_recv(0x40, 1, 1))
        {
            cprintf("read/write: n = %d, error = %d\n\r", n, i2c_error);
        }
        if (i2c_recv_buf[0] != n) cprintf("read/write: n = %d, got %d\n\r", n, i2c_recv_buf[0]);
    }
}

main()
{
    int n;

    i2c_init(1);
    i2c_reset();

#if 0
    write_test();
    write_block_test();
    write_and_read_test();
#endif
    combined_read_write_test();

    i2c_uninit();
}
