/*
* SPI testing utility (using spidev driver)
*
* Copyright (c) 2007  MontaVista Software, Inc.
* Copyright (c) 2007  Anton Vorontsov
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License.
*
* Cross-compile with cross-gcc -I/path/to/cross-kernel/include
*
*/
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#include "pluto_common_rpspi.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
    perror(s);
    abort();
}
volatile unsigned *spihold;
static const char *device = "/dev/spidev0.0";
static uint8_t mode = 3;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay;

static void transfer(int fd)
{
    int ret;
    uint8_t tx[] = {
        0x01, 0x00,0x45,0x67,
		0x89, 0xab,0xcd,0xef,
        0x01, 0x23,0x45,0x67,
		0x89, 0xab,0xcd,0xef,
        0x01, 0x23,0x45,0x67
    };
    uint8_t rx[ARRAY_SIZE(tx)] = {0, };
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = delay,
        .speed_hz = 0,
        .bits_per_word = 0,
    };

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
//	write(fd, tx,5);
//	read(fd, rx,5);
    if (ret == 1)
        pabort("can't send spi message");

    for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
        if (!(ret % 4))
            puts("");
        printf("%.2X ", rx[ret]);
    }
    puts("");
}

void print_usage(const char *prog)
{
    printf("Usage: %s [-DsbdlHOLC3]\n", prog);
    puts("  -D --device   device to use (default /dev/spidev1.1)\n"
         "  -s --speed    max speed (Hz)\n"
         "  -d --delay    delay (usec)\n"
         "  -b --bpw      bits per word \n"
         "  -l --loop     loopback\n"
         "  -H --cpha     clock phase\n"
         "  -O --cpol     clock polarity\n"
         "  -L --lsb      least significant bit first\n"
         "  -C --cs-high  chip select active high\n"
         "  -3 --3wire    SI/SO signals shared\n");
    exit(1);
}

void parse_opts(int argc, char *argv[])
{
    while (1) {
        static const struct option lopts[] = {
            { "device",  1, 0, 'D' },
            { "speed",   1, 0, 's' },
            { "delay",   1, 0, 'd' },
            { "bpw",     1, 0, 'b' },
            { "loop",    0, 0, 'l' },
            { "cpha",    0, 0, 'H' },
            { "cpol",    0, 0, 'O' },
            { "lsb",     0, 0, 'L' },
            { "cs-high", 0, 0, 'C' },
            { "3wire",   0, 0, '3' },
            { NULL, 0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "D:s:d:b:lHOLC3", lopts, NULL);
        if (c == -1)
            break;
        switch (c) {
        case 'D':
            device = optarg;
            break;
        case 's':
            speed = atoi(optarg);
            break;
        case 'd':
            delay = atoi(optarg);
            break;
        case 'b':
            bits = atoi(optarg);
            break;
        case 'l':
            mode |= SPI_LOOP;
            break;
        case 'H':
            mode |= SPI_CPHA;
            break;
        case 'O':
            mode |= SPI_CPOL;
			printf("SPI_CPOL = 0x%x\n",SPI_CPOL);
            break;
        case 'L':
            mode |= SPI_LSB_FIRST;
            break;
        case 'C':
            mode |= SPI_CS_HIGH;
            break;
        case '3':
            mode |= SPI_3WIRE;
            break;
        default:
            print_usage(argv[0]);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int fd,fd2;
	static unsigned int  mem_spi_hold_base;
	mem_spi_hold_base = BCM2835_SPI_hold+ BCM2709_OFFSET;
	
    parse_opts(argc, argv);
    fd = open(device, O_RDWR);
	if (fd < 0)
        pabort("can't open device");
	
	fd2 = open("/dev/mem", O_RDWR);
	
	if (fd2 < 0)
        pabort("can't open device fd2\n");
	
	spihold = mmap(
	        NULL,
	        BLOCK_SIZE,
	        PROT_READ|PROT_WRITE,
	        MAP_SHARED,
	        fd2,
	        mem_spi_hold_base);
		if (spihold == MAP_FAILED) {
		printf("HAL_pluto_servo_rpspi: can't map spi\n");
		return -3;
	}
	
    printf("mem_spi_hold_base:0x%x BCM2835_spiholdtime:addr:0x%x value:0x%x\n",BCM2835_SPI_hold,&BCM2835_spiholdtime,BCM2835_spiholdtime);


    /* spi mode */
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
        pabort("can't set spi mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1)
        pabort("can't get spi mode");
//bits=8;
    /* bits per word */
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't set bits per word");
//bits =8;
    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't get bits per word");

    /* max speed hz */
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't get max speed hz");

    printf("spi mode: %d\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d Hz (%d MHz)\n", speed, speed/1000/1000);
//while(1)
    transfer(fd);

    close(fd);
	close(fd2);
    return ret;
}
