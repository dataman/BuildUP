// BuildUP.c
// Flashing LED that decreases in delay.
// Push ON, LED blinks, faster after every X flashes til solid
// Push OFF, CPU Sleeps
// By Dataman, Charley Jones, 8/2012
//
// Based on TinyCylon by Dale Wheat
//
// tinyCylon2.c
// revised firmware for tinyCylon LED scanner
// written by dale wheat - 18 november 2008
// based on behavior of original tinyCylon firmware

// notes:

// device = ATtiny13A
// clock = 128 KHz internal RC oscillator
// max ISP frequency ~20 KHz
// brown-out detect = 1.8 V

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// These registers are available on the ATtiny13A but not the original ATtiny13

// Brown out detector control register

#define BODCR _SFR_IO8(0x30)
#define BODSE 0
#define BODS 1

// Power reduction register

#define PRR _SFR_IO8(0x3C)
#define PRADC 0
#define PRTIM0 1

typedef enum {
	MODE_0,  // Blink
	MODE_MAX // off
} MODE;

volatile MODE mode __attribute__ ((section (".noinit")));


///////////////////////////////////////////////////////////////////////////////
// init() - initialize everything
// note:  this "function" is in section .init3 so it is executed before main()
///////////////////////////////////////////////////////////////////////////////

void init(void) __attribute__ ((naked, section(".init3")));
void init(void) {

	// turn off unused peripherals to save power

	ACSR = 1<<ACD; // disable analog comparator
	DIDR0 = 1<<ADC3D | 1<<ADC2D | 1<<ADC1D | 1<<ADC0D | 1<<AIN1D | 1<<AIN0D; // disable all digital inputs

	// determine cause of device reset;  act accordingly

	if(bit_is_set(MCUSR, PORF)) {
		mode = MODE_0; // power on!
	} else if(bit_is_set(MCUSR, EXTRF)) {
		mode++; // advance mode
		if(mode > MODE_MAX) {
			mode = MODE_0; // reset mode
		}
	}

	MCUSR = 0; // reset bits

	// initialize ATtiny13 input & output port

	// PORTB

	//	PB0		5		MOSI/AIN0/OC0A/PCINT0		D1 output, active low
	//	PB1		6		MISO/AIN1/OC0B/INT0/PCINT1	D2 N/A
	//	PB2		7		SCK/ADC1/T0/PCINT2		D3 N/A
	//	PB3		2		PCINT3/CLKI/ADC3		D4 N/A
	//	PB4		3		PCINT4/ADC2			D5 N/A
	//	PB5		1		PCINT5/-RESET/ADC0/dW		MODE advance pushbutton

	PORTB = 0<<PORTB5 | 0<<PORTB4 | 0<<PORTB3 | 0<<PORTB2 | 0<<PORTB1 | 1<<PORTB0;
	DDRB = 0<<DDB5 | 0<<DDB4 | 0<<DDB3 | 0<<DDB2 | 0<<DDB1 | 1<<DDB0;

	// initialize ATtiny13 timer/counter

	TCCR0B = 0<<FOC0A | 0<<FOC0B | 0<<WGM02 | 0<<CS02 | 0<<CS01 | 1<<CS00;
	TIMSK0 = 0<<OCIE0B | 0<<OCIE0A | 1<<TOIE0; // interrupts

	sei(); // enable global interrupts
}

///////////////////////////////////////////////////////////////////////////////
// timing & delay functions
///////////////////////////////////////////////////////////////////////////////

volatile uint16_t downcounter;
volatile uint16_t upcounter;

void delay(uint16_t n) {

	downcounter = n;

	while(downcounter) {
		MCUCR = 1<<PUD | 1<<SE | 0<<SM1 | 0<<SM0 | 0<<ISC01 | 0<<ISC00; // idle mode
		asm("sleep"); // go to sleep to save power
	}
}


#define DELAYON         30     // How long is LED on, not variable
#define DELAYOFFSTART   180    // How long is LED off, variable
#define DELAYSTEP       30     // Step to decrease delay
#define DELAYSTOP       30     // Don't step lower than this
#define UPCOUNTERMAX    1200   // Max Upcounter for next step down

void blink();
void blink() {
	uint16_t idelay = DELAYOFFSTART;
	uint8_t icounter = DELAYCOUNTRESET;
	upcounter = 0;
	while (1) {
		PORTB = 1;
		delay(DELAYON);
		PORTB = 0;
		delay(idelay);
		if (upcounter >= UPCOUNTERMAX)
 		  upcounter = 0;
		  if (idelay>DELAYSTOP) idelay-=DELAYSTEP;
		}
	}	
}

///////////////////////////////////////////////////////////////////////////////
// main() - main program function
///////////////////////////////////////////////////////////////////////////////

void main(void) {

	switch(mode) {

		case MODE_0: // blink with decreasing delay
			blink();
			break;


		case MODE_MAX: // off
			PORTB = 0b00011111; // all LEDs off
			while(1) {
				// deepest sleep mode
				cli(); // disable interrupts
				PRR = 1<<PRTIM0 | 1<<PRADC; // power down timer/counter0 & ADC
				BODCR = 1<<BODS | 1<<BODSE; // enable BOD disable during sleep, step 1
				BODCR = 1<<BODS | 0<<BODSE; // step 2
				MCUCR = 1<<PUD | 1<<SE | 1<<SM1 | 0<<SM0 | 0<<ISC01 | 0<<ISC00; // select "power down" mode
				asm("sleep"); // go to sleep to save power
			}
	}
}

///////////////////////////////////////////////////////////////////////////////
// timer/counter0 overflow interrupt handler
///////////////////////////////////////////////////////////////////////////////

ISR(TIM0_OVF_vect) {

	downcounter--; // decrement downcounter for delay functions
	upcounter++;
}

///////////////////////////////////////////////////////////////////////////////

// [end-of-file]
