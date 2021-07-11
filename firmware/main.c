/* CONFIG */
#pragma config IOSCFS = 4MHZ    /* Internal Oscillator Frequency Select bit (4 MHz) */
#pragma config MCPU = OFF       /* Master Clear Pull-up Enable bit (Pull-up disabled) */
#pragma config WDTE = ON        /* Watchdog Timer Enable bit (WDT enabled) */
#pragma config CP = OFF         /* Code protection bit (Code protection off) */
#pragma config MCLRE = OFF      /* GP3/MCLR Pin Function Select bit (GP3/MCLR pin function is digital I/O, MCLR internally tied to VDD) */

#include <xc.h>
#include <stdint.h>

/*
 * Pin descriptions:
 *
 * GP0 - pin connected to CHRG pin on TP4054 to determine if the charger
 * is charging correctly or the battery itself is charged. Because TP4045 has 3
 * possible states, by using ADC and resistor those states can be detected.
 * When 3.3V is on it (logic high) that means the charger is not connected or
 * the battery is not charging (may be absent from circuit for example)
 * When around 0.5V is on the pin (still logic low because input is TTL) that
 * means the charging is complete
 * When 0V on the pin (logic low) that means TP4054 is charging the battery
 *
 * GP1 - pin connected to resistor divider network for the battery measurement
 * divider ratio is 76,75% (10k and 33k resistor) and it should always stay
 * above 2.3V if the battery is connected
 *
 * GP2 - LED output pin
 *
 * GP3 - button pin (active low) - when pressed the device should give it's
 * current battery/charging status. If pressed for longer it should change
 * current LED effect (3 available at the moment) or shutdown the device
 */

typedef union {
    uint8_t temp_8[2];
    uint16_t temp_16;
} scratch_ram_t;

typedef enum {
    BR_POWER_ON = 0x00,
    BR_PIN_CHANGE = 0x01,
    BR_WDT_WAKE_UP = 0x02,
    BR_RESET = 0x03
} bootreason_t;

/*
 * TODO: Simplify this logic maybe
 * States with even number are used when charger wasn't connected
 * After connecting charger, the device should show that it's charging on
 * connection moment - only once per charger connection. odd states will go
 * back to even when the charger is disconnected
 */
typedef enum {
    LEDMODE_RNGBLINK = 0x00,
    LEDMODE_RNGBLINK_BS = 0x01,
    LEDMODE_QUADWAVE = 0x02,
    LEDMODE_QUADWAVE_BS = 0x03,
    LEDMODE_TRIWAVE = 0x04,
    LEDMODE_TRIWAVE_BS = 0x05,
    LEDMODE_SHUTDOWN = 0x06,
    LEDMODE_SHUTDOWN_BS = 0x07,
} ledmode_t;

/* Prescaler selection for more random sine/triwave options */
const uint8_t prescaler_lut[4] = {0x03, 0x04, 0x05, 0x03};

 /* Used solely for keeping random numbers */
uint16_t random_16 = 0x9532;
/* Used for iterators in functions */
uint8_t i = 0;
/* GPIO status pins to write to when going to sleep  */
/* and read when wake-up on pin change occurred or ADC result */
__persistent uint8_t gpio_reg = 0;
/* Used as temporary value for things like RNG and blink routines */
scratch_ram_t scratch = {.temp_16 = 0};
/* Mode of the LED */
__persistent  ledmode_t mode = 0;

/* Function prototypes */
void get_next_random_16(void);
bootreason_t get_bootreason(void);
uint8_t triwave8(uint8_t in);
uint8_t quadwave8(uint8_t in);
void initIO(void);
void delay_32_ms(void);
void delay_tmr(uint8_t tmr_cycles);
void blink(uint8_t time_multiplier);
void show_charging_status(void);
void show_battery_status(void);
void change_mode_state(void);
void set_random_prescaler(void);

/* Functions */

/*
 * 16-bit Implementations of Galois LFSR (Linear Feedback Shift Register)
 * original source: http://www.piclist.com/techref/microchip/rand8bit.htm
 * original author: Mark Jeronimus
 * C equivalent function:
 * uint16_t random_16 = 1;
 * void next_rand_16() {
 *   uint8_t result = 1 & (random_16 >> 1);
 *   random_16 = random_16 >> 1;
 *   if (result == 0) {
 *     random_16 = random_16 ^ 0xA1A1;
 *   }
 * }
 */
void get_next_random_16() {
    __asm("clrw");
    __asm("bcf STATUS, 0"); /* Clear carry bit */
    __asm("rrf _random_16+1, 1"); /* Shift high part right through carry */
    __asm("rrf _random_16, 1"); /* Shift lwo part right through carry */
    __asm("btfss STATUS, 0"); /* Check carry bit */
    __asm("goto end_random"); /* if carry is 0 then end function */
    __asm("movlw 0xA1"); /* set W to A1 (0xA1A1 is polynomial value) */
    __asm("xorwf _random_16+1"); /* XOR 0xA1 with high part */
    __asm("xorwf _random_16"); /* XOR 0xA1 with low part */
    __asm("end_random:"); /* end of function */
}

bootreason_t get_bootreason() {
    if (STATUSbits.GPWUF == 1) {
        return BR_PIN_CHANGE; /* Pin change when in Sleep */
    } else if (__timeout == 0) {
        return BR_WDT_WAKE_UP; /* Reset Caused By WDT wake-up from Sleep */
    } else if (__powerdown == 1) {
        return BR_POWER_ON; /* Reset Caused By Power-up */
    } else return BR_RESET; /* Reset caused by MCLR reset */
}
/*
 * Slightly modified and inlined functions from lib8tion (FastLED Project)
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 FastLED
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

uint8_t triwave8(uint8_t in) {
    if( in & 0x80) {
        in = 255 - in;
    }
    uint8_t out = (uint8_t) (in << 1);
    return out;
}

uint8_t quadwave8(uint8_t in) {
    if( in & 0x80) {
        in = 255 - in;
    }
    uint8_t j = (uint8_t)(in << 1);
    if( j & 0x80 ) {
        j = 255 - j;
    }
    uint8_t jj  =  (((uint16_t) j) * (1+(uint16_t)(j))) >> 8;
    jj = (uint8_t)(jj << 1);
    if( in << 1 & 0x80 ) {
        jj = 255 - jj;
    }
    return jj;
}

/* Setup the IO to avoid strange LED glitches on power-up */
void initIO() {
    /* Enable wake-up on pin change, disable pull-ups, set prescaler */
    /* to 1:256 for Timer0 (1:128 for WDT) */
    OPTION = nGPPU | PS0 | PS1 | PS2;
    /* Select only GP2 as output and set it to low */
    GP2 = 0;
    TRISGPIO = 0x0B;
    /* Preconfigure ADC to use GP0 and GP1 but set them as digital pins */
    ADCON0 = 0x00;
}

/* Use timer to get ~32 ms delay on 4MHz oscillator and 1:256 prescaler */
void delay_32_ms() {
    TMR0 = 0;
    while(!(TMR0&0x80)) __asm("clrwdt");
}
/* Use timer to get a variable delay - useful for soft PWM */
void delay_tmr(uint8_t tmr_cycles) {
    TMR0 = 0;
    while (TMR0 != tmr_cycles);
}

void blink(uint8_t time) {
    GPIObits.GP2 = 1;
    blink_wait:
    for(scratch.temp_8[0] = time; scratch.temp_8[0] > 0; scratch.temp_8[0]--) {
        TMR0 = 0;
        while(!(TMR0&0x80)) __asm("clrwdt");
    }
    if (!GPIObits.GP2)
        return;
    GPIObits.GP2 = 0;
    goto blink_wait;
}

void show_charging_status() {
    /* Connect GP0 to ADC and communicate charging status to the user */
    /* by blinking: 2 short means charged and 2 long means charging */
    ADCON0bits.ANS0 = 1;
    ADCON0bits.CHS0 = 0;
    ADCON0bits.ADON = 1;
    /* Start conversion and wait for the result */
    ADCON0bits.GO = 1;
    while (ADCON0bits.nDONE);
    if (ADRES > 20) {
            blink(25);
            blink(25);
    } else {
            blink(15);
            blink(15);
    }
    /* Turn off ADC and disconnect the pin */
    ADCON0bits.ADON = 0;
    ADCON0bits.ANS0 = 0;
}

void show_battery_status() {
    /* Connect GP1 to ADC and communicate battery level */
    /* to the user by blinking the more the higher battery level */
    ADCON0bits.CHS0 = 1;
    ADCON0bits.ANS1 = 1;
    ADCON0bits.ADON = 1;
    /* Start conversion and wait for the result */
    ADCON0bits.GO = 1;
    while (ADCON0bits.nDONE);
    /* Sanity check - if battery is not present or */
    /* bellow 2.65V  make one quick blink */
    if (ADRES < 155) {
        blink(5);
    } else {
        /* Convert ADC result to amount of blinks */
        i = 1;
        if (ADRES > 200) i = 2;
        if (ADRES > 211) i = 3;
        if (ADRES > 219) i = 4;
        if (ADRES > 222) i = 5;
        if (ADRES > 229) i = 6;
        for (; i > 0; i--) {
            blink(10);
        }
    }

    /* Turn off ADC */
    ADCON0bits.ADON = 0;
    ADCON0bits.ANS1 = 0;
}

/* Changing of the mode will be happening as long as the button is pressed */
void change_mode_state() {
    while (!GPIObits.GP3) {
        /* Show demo of the function and if still pressed go to the next one */
        blink(60);
        if (mode < LEDMODE_SHUTDOWN) mode = mode + 2;
        else if (mode == LEDMODE_QUADWAVE_BS) mode = LEDMODE_SHUTDOWN_BS;
        else mode = LEDMODE_RNGBLINK;
    }
}

void set_random_prescaler() {
    OPTION = nGPPU | prescaler_lut[(uint8_t) (random_16 & 0x03)];
}

void main(void) {
    /* Some sanity checks to make sure the device will operate properly */
    if (mode > LEDMODE_SHUTDOWN_BS) mode = LEDMODE_RNGBLINK;
    /* Init random generator from given seed or the last value before sleep */
    if (scratch.temp_16 != 0) random_16 = scratch.temp_16;

    /* Init IO pins to default state */
    initIO();
    /* Check why device started */
    switch (get_bootreason()) {
        case BR_RESET:
        case BR_POWER_ON: {
            /* Wait for around 2 seconds before beginning power-up sequence */
            /* XXX: not working in simulator - probably a bug in MPLAB X 5.40 */
            for (i = 60; i > 0; i--) delay_32_ms();

            /* Check if charger is connected */
            if (!GPIObits.GP0) {
                /* If it is show charging status */
                show_charging_status();
                /* Go to odd state as charging was already shown */
                mode++;
            } else {
                /* If charger not connected display battery charge */
                show_battery_status();
            }
            /* Check if button is still pressed and change the mode if it is */
            change_mode_state();
            break;
        }
        case BR_PIN_CHANGE: {
            /* XOR old and new bit status and then AND it with mask */
            /* to get if the button caused the wake-up or was it  */
            /* battery or charger pin */
            gpio_reg = (gpio_reg ^ GPIO);

            if (gpio_reg & 0x08) {
                /* Button pressed - show battery/charging status */
                if (!GPIObits.GP0) {
                    show_charging_status();
                } else {
                    show_battery_status();
                }
                /* if still pressed change mode */
                change_mode_state();
            /* Charger connected/disconnected */
            } else if (gpio_reg & 0x01) {
                show_charging_status();
            } else {
                show_battery_status();
            }
            break;
        }
        case BR_WDT_WAKE_UP: {
            /* Do nothing and proceed */
        }
    }
    /* Skip this iteration once in a while */
    /* this is added because maximum WDT period of 2.3 seconds is too short */
    if (random_16 & 0x01) {
        i = nGPPU | PSA | PS0 | PS1 | PS2;
        goto set_wdt_and_sleep;
    }
    /* Show LED effect  */
    /* Set timer prescaler for random effect duration (only rngblink) */
    /* will use the longest because shorter blinks don't look that nice */
    set_random_prescaler();
    
    for (i = 255; i > 0; i--) {
        __asm("clrwdt"); /* Remember about watchdog */
        get_next_random_16();

        switch (mode) {
            case LEDMODE_RNGBLINK:
            case LEDMODE_RNGBLINK_BS: {
                /* The longest prescaler has to be used for this effect */
                OPTION = nGPPU | PS0 | PS1 | PS2;
                delay_32_ms();
                delay_32_ms();
                if (random_16 & 0x07) GPIObits.GP2 = 0;
                else                  GPIObits.GP2 = 1;
                break;
            }
            case LEDMODE_QUADWAVE:
            case LEDMODE_QUADWAVE_BS: {
                scratch.temp_8[0] = quadwave8(i);
                GPIObits.GP2 = 1;
                delay_tmr(scratch.temp_8[0]);
                GPIObits.GP2 = 0;
                delay_tmr(255-scratch.temp_8[0]);
                break;
            }
            case LEDMODE_TRIWAVE:
            case LEDMODE_TRIWAVE_BS: {
                scratch.temp_8[0] = triwave8(i);
                GPIObits.GP2 = 1;
                delay_tmr(scratch.temp_8[0]);
                GPIObits.GP2 = 0;
                delay_tmr(255-scratch.temp_8[0]);
                break;
            }
            /* Device shutdown
             * (status LEDMODE_SHUTDOWN or LEDMODE_SHUTDOWN_BS) 
             */
            default:
                /* Set the prescaler to make WDT sleep as long as possible */
                /* because WDT cannot be disabled at runtime */
                i = nGPPU | PSA | PS0 | PS1 | PS2;
                goto set_wdt_and_sleep;
        }


        /* Check if button was pressed */
        
        if (!GPIObits.GP3) {
            /* Set the prescaler to default value before going into settings */
            OPTION = nGPPU | PS0 | PS1 | PS2;
            /* Assume that charging/battery status will always be shown */
            if (!GPIObits.GP0) {
                show_charging_status();
            } else {
                show_battery_status();
            }
            /* If the button is still pressed after that get to change mode */
            change_mode_state();
            /* Restore prescaler - won't be same as before but in the range */
            set_random_prescaler();
        }
        /* Check if charger was connected and charging was not shown yet */
        if (!GPIObits.GP0 && (mode & 0x01) == 0) {
            /* Set the prescaler to default value before showing status */
            OPTION = nGPPU | PS0 | PS1 | PS2;
            show_charging_status();
            mode++; /* Go to odd state - charging status already shown */
            /* Restart showing of the effect */
            i = 0;
            set_random_prescaler();
        }
        /* Check if charger was disconnected and the state is odd to be able */
        /* to show charging reaction after another connection */
        if (GPIObits.GP0 && (mode & 0x01) == 1) {
            mode--; /* Go back to even state */
        }
    }

    /* Save random variable for the next time */
    scratch.temp_16 = random_16;
    /* get the 2 random bits to set the prescaler */
    /* to be random from 1:16 to 1:128 - works fine */
    i = nGPPU | PSA | (0x03 & scratch.temp_8[0]) | PS2;
    /* turn off LED (it could still be on because of the rngblink mode) */
    GPIObits.GP2 = 0;
    set_wdt_and_sleep:
        /* Read the registers to make wake up on pin change work correctly */
        gpio_reg = GPIO;
        /* Give the prescaler to WDT, set it and go to sleep */
        __asm("clrwdt");
        TMR0 = 0;
        OPTION = i;
        __asm("sleep");                
}
