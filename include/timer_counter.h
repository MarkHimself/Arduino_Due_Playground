#ifndef TIMER_COUNTER_H
#define TIMER_COUNTER_H

#include "Arduino.h"


/* Set up a timer to create a delay function.
//	-UPDATE: The Datasheet refers to TC0 - TC8 as the timer + channel type of thing.
	- in other words. to turn on the clock for TC2 Channel 1, you need TC7 (in PCER1) register.
	- This is called BAD Documentation.
	
	OUTCOME:
	Looks like an accurate delay function.
	
*** Addition #2: Add a oneshot signal on another channel. ***
	- TC2 Channel 0
	- PIOC 25 Peripheral B (digital pin 5)
	- TIOA6

PLAN
Use waveform mode. disable clock (or just stop it) with an RC compare.
When RC compare occurs, the delay time has passed by.
Get the status from the status register.

OUTCOME:
The oneshot signal is not as accurate as i want it to be. kinda off by like 1us
It can be fine tuned but it should just work without tuning though.

two ways to do this oneshot code..
enable the clock in setup and only stop it in the oneshot RC compare.
have to be careful to not send another oneshot before current one ends.
OR
disable the clock in setup. upon completion of oneshot, clock gets disabled.
check that clock is disabled before entering oneshot (if enabled, then oneshot in progress)

*** Addition #3: Make TC2 Channel 1 create an interrupt every 1s. print value after interrupt.
	- TC2 Channel 1
	- This is Instance I.D. TC7 on page 38-39
	- header definitions are found in:
		- sam3x8e.h
		- cortex_handler.c
		- component_tc.h
	- Don't forget to read the status register inside the interrupt or else system freezes.
	- Most logic is untested. the interrupt does work though!
*/



void reset_Timer_Controller();
void setup_timer_for_ms_delay();
void delay_ms(uint16_t val);

void setup_timer_for_oneshot();
void setup_PIOC25_as_TIOA6();
void oneshot_TIOA6_us(uint16_t pulse_us);
void oneshot_TIOA6_42MHz_Clock_Ticks(uint16_t pulse_ticks);

void setup_TC2_1_for_interrupts();
void TC2_1_interrupt_period(uint16_t fire_ms);
void TC2_1_enable_interrupts();
void TC2_1_disable_interrupts();
bool get_TC2_1_int_flag();


#endif