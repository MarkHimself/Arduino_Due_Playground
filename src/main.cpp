#include <Arduino.h>
#include <cstdint>
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

/* Set up a timer to create a delay function.
//	-UPDATE: The Datasheet refers to TC0 - TC8 as the timer + channel type of thing.
	- in other words. to turn on the clock for TC2 Channel 1, you need TC7 (in PCER1) register.
	- This is called BAD Documentation.

PLAN
Use waveform mode. disable clock (or just stop it) with an RC compare.
When RC compare occurs, the delay time has passed by.
Get the status from the status register.

OUTCOME:
Looks like an accurate delay function.
*/


void reset_Timer_Controller();
void setup_timer_for_ms_delay();
void delay_ms(uint16_t val);


uint32_t b[32];


void setup() {
	
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(38400);
	Serial.println("hi");
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
}

void loop() {
	//int delay_time = 50;
	
	Serial.println("2000 in 1ms increments");
	for (int i = 0; i < 2000; i++) delay_ms(1);
	
	Serial.println("2000");
	delay_ms(2000);
	
	Serial.println("3000");
	delay_ms(3000);
	
	Serial.println("8000");
	delay_ms(8000);
	
	Serial.println("750");
	delay_ms(750);
	
	Serial.println("100");
	delay_ms(100);
	
}

void reset_Timer_Controller(){
	
	// TC1 channel 0 is instance number 30										pg. 38
	
	// disable quad decoder
	TC1->TC_BMR = 0;					// block mode register					pg. 901
	TC1->TC_QIDR = 7;					// disabl quad decoder Interrupts		pg. 904
	TC1->TC_FMR = 0;					// disable faults						pg. 907
}

void setup_timer_for_ms_delay(){
	
	// TC1 is instance number 28												pg. 38
	PMC->PMC_PCER0 = b[30];			// enable clock to TC 1 channel 0			pg. 542
	TC1->TC_WPMR = 0x54494D << 8;		// unlock write protect registers		pg. 908
	
	TC1->TC_CHANNEL[0].TC_CCR = b[1];	// disable the clock					pg. 880
	TC1->TC_CHANNEL[0].TC_CMR = 0		// clear out capture/waveform mode		pg. 881
		| TC_CMR_TCCLKS_TIMER_CLOCK1			// 42 MHz
		| TC_CMR_WAVE							// select waveform mode.
		| TC_CMR_CPCSTOP						// stop the clock when an RC comare occurs.
		| TC_CMR_WAVSEL_UP						// count just up (no external trigger on rc compary)
												// so it doesn't restart the clock.
	;
	TC1->TC_CHANNEL[0].TC_CCR = b[0];	// enable the clock						pg. 880
	// consider checking the status register to ensure the clock is actually enabled.
	// return true/false to know if it was set up successfully.
}

void delay_ms(uint16_t val){
	uint32_t status_reg = 0;
	
	TC1->TC_CHANNEL[0].TC_RC = 42000 * val;			// count to RC = val.		pg. 891
	/*
	f_clk = 42 MHz
	delay 1 [ms] * (1 second / 1000 ms) * (42*10^6 / 1 second) = 42,000 ticks.
	delay val [ms] = 42,000 * val ticks.
	*/
	
	// software trigger reset the counter and starts counting (SWTRG in TC_CCR)	// pg. 862-863
	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG | b[0];	// start the counter			// pg. 880
	
	
	// when timer overflows, COVFS is set in TC_SR register.					// pg. 860
	// The CPCS bit is set when the timer reaches RC. that is what i'm using.
	
	while (	!((status_reg = TC1->TC_CHANNEL[0].TC_SR) & TC_SR_CPCS) ){				// pg. 892
		// wait until the timer/counter value equals TC_RC.
		// when compare occurs, the clock will be stopped.
	}
}



