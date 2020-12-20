/* 
 * File:   main.c
 * Author: sol
 *
 * Created in December 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 1000000UL

//TODO: figure out the clock?? PLL? Other? ALso, what to do w/synchronous mode?

/*Pinout:
  - PB1 = LED Group0
  - PB2 = LED Group1
  - PB0 = TOPPER LED (OC0A)
 
 Functionality:
 * Alternate/blink group0 and group1 w/ timer0 ISR
 * Fade topper w/ PWM.
 */


//timer prescale reg values.
#define TIMER0_PRSCL 0x00
#define TIMER1_PRSCL 0b0010
#define TOPPER_BASE_VAL 5
#define PWM_LOOP_DELAY 2000 //the number of clock cycles to delay between ocr updates
#define PWM_MAX_OCR 254 //value at which to toggle up/down flag

int downcount_flag = 0;

//OC0A is used to define the blink period for the LED groups.
#define TIMER1_OUTCOMP 0xFF


void timer_init(void) {
    TCNT1 = 0;
    TCCR1 = (1<<CTC1)|(TIMER1_PRSCL<<CS10); //set prescaler & wave gen mode to clear on match.
    TIMSK |= (1<<OCIE1A);
    OCR1A = TIMER1_OUTCOMP;
}


//ISR to blink branch lights
ISR(TIMER1_COMPA_vect) {
    PORTB ^= (0b11 << PB1); //Toggle PB1 and PB2 on compare match.
}

void pwm_init(void) {
    TCNT0 = 0;
    TCCR0A = (1<<COM0A1)|(1<<WGM00)|(1<<WGM01); //non-inverting mode
    TCCR0B = (1<<WGM02)|(1<<CS01); //Fast-PWM mode (also includes wgm1&2 above)
    OCR0A = TOPPER_BASE_VAL; //start topper dim
    //may need to enable output compare 0A in TIMSK?
}

void _setup(void) {
    cli(); //disable interrupts
    
    DDRB = (1<<PB0)|(1<<PB1)|(1<<PB2); //pb0=topper, pb1&2=branches
    //PORTB = (1<<PB0);
    TIMSK = 0;
    
    timer_init();
    pwm_init();
    
    sei(); //enable interrupts
}

/*
 * 
 */
int main() {
    _setup();
    int outer_loop_counter = 0;
    
    while(1) {
       if(outer_loop_counter == PWM_LOOP_DELAY) {
           
           //set up/down count at max/min
           if(OCR0A == PWM_MAX_OCR) {
               downcount_flag = 1;
           }
           if(OCR0A==TOPPER_BASE_VAL) {
               downcount_flag = 0;
           }
           
           //up/downcount the output compare value.
           if(downcount_flag) {
               cli();
               OCR0A -= 1;
               sei();
           }
           if(!downcount_flag) {
               cli();
               OCR0A += 1;
               sei();
           }
           
           outer_loop_counter = 0;
       }
       outer_loop_counter += 1;
    }

    return (EXIT_SUCCESS);
}

