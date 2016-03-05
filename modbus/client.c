// gcc -Wall -g -I/usr/local/include/modbus client.c -lmodbus -o client

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
 
#include <modbus.h>
 
#define MODBUS_SERVER_IP            "127.0.0.1"
#define MODBUS_SERVER_PORT          502
#define MODBUS_TIMEOUT_SEC          3
#define MODBUS_TIMEOUT_USEC         0

 
int main(int argc, char *argv[])
{
    modbus_t    *ctx;
    int         ret, ii;
    uint8_t     bits[MODBUS_MAX_READ_BITS] = {0};
    uint16_t    regs[MODBUS_MAX_READ_REGISTERS] = {0};

    ctx = modbus_new_tcp(MODBUS_SERVER_IP, MODBUS_SERVER_PORT);

    modbus_set_slave(ctx, 1);
 
    //modbus_set_debug(ctx, ON);
 
    modbus_set_byte_timeout(ctx, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC);
 
    modbus_set_response_timeout(ctx, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC);
 
    if (modbus_connect(ctx) == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        exit(-1);
    }
 
    ret = modbus_read_registers(ctx, 3, 1, regs);
    if (ret < 0) 
    {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
    } 
    else 
    {
        for (ii=0; ii < ret; ii++) 
        {
            printf("[%d]=%d\n", ii, regs[ii]);
        }
    }

    ret = modbus_read_bits(ctx, 16, 2, bits);
    if (ret < 0) 
    {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
    } 
    else 
    {
        for (ii = 0; ii < ret; ii++) 
        {
            printf("[%d]=%d\n", ii, bits[ii]);
        }
    }
 
    ret = modbus_write_bit(ctx, 16, TRUE);
    if (ret < 0) 
    {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
    }

    ret = modbus_write_bit(ctx, 17, TRUE);
    if (ret < 0) 
    {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
    }
 
    ret = modbus_read_bits(ctx, 16, 2, bits);
    if (ret < 0) 
    {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
    } 
    else 
    {
        for (ii = 0; ii < ret; ii++) 
        {
            printf("[%d]=%d\n", ii, bits[ii]);
        }
    }

 
    // Close the connection
    modbus_close(ctx);
    modbus_free(ctx);
 
    exit(0);
}