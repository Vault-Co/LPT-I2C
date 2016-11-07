/*
 * iicw.c
 * Writes data on the i2c bus
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iic.h"

#define MAXBYTES I2C_BUFSIZE

static char rcsid[] = "$Id: iicw.c 1.1 95/03/03 13:53:12 mi Exp $";

extern void i2c_dump_error(int error);


void usage(char *progname)
{
    printf("Usage:\t%s aa bb ...\n\n", progname);
    printf("Writes hex digits bb ... to slave aa (in hex) on the I2C bus.\n");
    printf("Maximum number of bytes to write is %d.\n", MAXBYTES);
}


int main(int argc, char **argv)
{
    int n, sladd, byt;
    char *env;

    if (argc < 3 || argc > MAXBYTES+2)
    {
        usage(argv[0]);
        exit(10);
    }

    if (sscanf(argv[1], "%x", &sladd) != 1)
    {
        usage(argv[0]);
        exit(10);
    }

    for(n = 2; n < argc; n++)
    {
        if (sscanf(argv[n], "%x", &byt) != 1)
        {
            usage(argv[0]);
            exit(10);
        }
        i2c_xmit_buf[n-2] = byt;
    }

    env = getenv("I2C");
    if (env != NULL)
    {
        sscanf(env, "%d", &n);
        if (n) n--;
    }
    else
    {
        n = 0;
    }
    
    i2c_init(n);
    i2c_reset();

    i2c_send(sladd, argc-2);

    i2c_uninit();

    i2c_dump_error(i2c_error);

    return i2c_error;
}
