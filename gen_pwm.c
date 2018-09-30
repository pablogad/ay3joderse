#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bcm2835.h>


// 19200000 / 2 / 9 = 1000000 Hz aprox
#define CLOCK_DIVISOR 2  // Valor minimo es 2
#define PWM_PIN       RPI_V2_GPIO_P1_12

#define PWM_CHANNEL 0
// ALT FUN 5 para PWM en GPIO 18 (pin 12)
#define PWM_ALT BCM2835_GPIO_FSEL_ALT5


void end_pwm()
{
    bcm2835_pwm_set_mode( PWM_CHANNEL, 0, 0 );
    bcm2835_gpio_clr( PWM_PIN );
    bcm2835_gpio_fsel( PWM_PIN, BCM2835_GPIO_FSEL_INPT );
}

void init_pwm( const uint32_t freq ) {
    uint32_t range = 19200000 / ( CLOCK_DIVISOR * freq );
    printf( "-- Frecuencia real: %d Hz\n", 19200000 / (CLOCK_DIVISOR * range) );
 
    bcm2835_pwm_set_clock( CLOCK_DIVISOR);            // Cualquier valor hasta 1024 (?)
    bcm2835_pwm_set_mode( PWM_CHANNEL, 1, 1 );        // MARK-SPACE MODE, ENABLE
    bcm2835_pwm_set_range( PWM_CHANNEL, range );  // DATA varia hasta este rango
    bcm2835_gpio_fsel( PWM_PIN, PWM_ALT );
    bcm2835_pwm_set_data( PWM_CHANNEL, range / 2 );  // Ratio: 50% duty cycle
}

