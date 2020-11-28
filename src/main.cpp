#include <Arduino.h>
#include <cstdint>

#include "timer_counter.h"
#include "uart.h"
#include "ahb_dma_mem.h"

#define b(r) (1u << (r))

// Arduino Due
// Do random stuff on it to learn it better.
/*
- Input
	- Digital
	- Analog
- Output
	- Digital
- PWM
- Timers
- Sending UART messages to COM
- Receiving UART messages to COM
	- polling
	- interrupts
- Interrupts on PWM
- Interrupts on input pin
- Interrupts on analog value is read
- DMA
	- sending a string through UART to COM port
	- Updating PWM
*/

// AHB DMA memory to peripheral (USART0).
// it actually works! makes me wonder why my AHB DMA memory to PWM didn't work.


char myString[] = "send me over\n ooh look its dma working like it should!\n\n";
char myStringTracking[] = "^123456789abcdefghijklmn*pqrstuvwxyz[]-+ABCDEFGHIJKLMN@PQRSTUVWXYZ";
char myString2[] = "life is good oh yeah is it still?!!!\n\n";
char myString3[sizeof(myString2)];	// will try to copy myString into this string through AHB DMA.
char myString4_fail[] = "failed\n";


void setup() {
	
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
	
	setup_USART0(38400);
	setup_PIOA11_as_USART0_TX();
	setup_USART0_TX_String_AHB_DMA();
	
}

void loop() {
	/*
	write_USART0_TX(0x38);
	delay_ms(100);
	write_USART0_TX(0x42);
	delay_ms(1);
	write_USART0_TX(0x17);
	delay_ms(1);
	write_USART0_TX(0x75);
	delay_ms(1);
	write_USART0_TX(0xAA);
	delay_ms(1);
	*/
	//start_USART0_TX_String_AHB_DMA(myString, sizeof(myString));
	start_USART0_TX_String_AHB_DMA(myStringTracking, sizeof(myStringTracking));
	delay_ms(50);
}

