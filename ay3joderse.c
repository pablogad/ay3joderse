#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bcm2835.h>
#include "ay-3-8910.h"

// Latch de los HC595
#define HC_STCP RPI_V2_GPIO_P1_07  // GPIO 4 - pin 7

static void init_spi() {
   bcm2835_spi_begin();
   bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
   bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
   // ~2Mhz http://www.airspayce.com/mikem/bcm2835/group__constants.html#gaf2e0ca069b8caef24602a02e8a00884e
   bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);
   bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
   bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
}

static void init_gpio() {
   // GPIO para latch de 595
   bcm2835_gpio_fsel( HC_STCP, BCM2835_GPIO_FSEL_OUTP );
   bcm2835_gpio_write( HC_STCP, LOW );
}


// data: D0-D7 de AY3, pins: bit0: BC1, bit1: BDIR, resto libres
// devuelve los 16 bits leidos
static uint16_t enviar_spi( const uint8_t data, const uint8_t pins ) {
   uint8_t s[2];
   s[0]=pins; s[1]=data;
   //s[0]=data; s[1]=pins;
   bcm2835_gpio_write( HC_STCP, LOW );
   bcm2835_spi_transfern( s, 2 );
   bcm2835_gpio_write( HC_STCP, HIGH );  // Store latch HC595 : ^
   return s[0] + s[1]*256;
}

// Enviar datos a dirección de AY-3-8910
static void ay3_write_reg( const uint8_t reg_addr, const uint8_t data ) {
   enviar_spi( reg_addr, AY3_INACTIVE );  // 00
   enviar_spi( reg_addr, AY3_LATCH );     // 11
   // delay 350ns
   bcm2835_delayMicroseconds(1);
   enviar_spi( data, AY3_INACTIVE );      // 00
   // delay 100ns
   bcm2835_delayMicroseconds(1);
   enviar_spi( data, AY3_WRITE );         // 01
   // delay 1900ns
   bcm2835_delayMicroseconds(3);
   enviar_spi( data, AY3_INACTIVE );      // 00
   // delay 100ns
   bcm2835_delayMicroseconds(1);
}


int main (void)
{
    if (!bcm2835_init()) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }

    init_spi();
    init_gpio();

    // TEST
    //enviar_spi( 0xAA, 0x55 );  // 10100101 - 01010101

    ay3_write_reg( R7, 0 );    // Todo activado (tono, noise y ports)
    // -- ch A --
    ay3_write_reg( R0, 62 );   // Fine tune
    ay3_write_reg( R1, 0 );    // Coarse tune
    ay3_write_reg( R10, 15 );  // Chn A volumen al maximo
    // -- ch B --
    ay3_write_reg( R2, 0 );    // Coarse tune
    ay3_write_reg( R3, 0 );    // Coarse tune
    ay3_write_reg( R11, 0 );   // Chn B volumen
    // -- ch C --
    ay3_write_reg( R2, 0 );    // Coarse tune
    ay3_write_reg( R3, 0 );    // Coarse tune
    ay3_write_reg( R12, 0 );   // Chn C volumen


    /*while (1)*/ sleep (20);

    ay3_write_reg( R10, 0 );  // Chn A volumen a cero

    bcm2835_spi_end();

    return 0;
}

