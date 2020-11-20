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
	
*** Addition: Add a oneshot signal on another channel. ***
	- TC2 Channel 0
	- PIOC 25 Peripheral B (digital pin 5)
	- TIOA6

PLAN
Use waveform mode. disable clock (or just stop it) with an RC compare.
When RC compare occurs, the delay time has passed by.
Get the status from the status register.

OUTCOME:
Looks like an accurate delay function.

The oneshot signal is not as accurate as i want it to be. kinda off by like 1us
It can be fine tuned but it should just work without tuning though.

two ways to do this oneshot code..
enable the clock in setup and only stop it in the oneshot RC compare.
have to be careful to not send another oneshot before current one ends.
OR
disable the clock in setup. upon completion of oneshot, clock gets disabled.
check that clock is disabled before entering oneshot (if enabled, then oneshot in progress)
*/


void reset_Timer_Controller();
void setup_timer_for_ms_delay();
void delay_ms(uint16_t val);

void setup_timer_for_oneshot();
void setup_PIOC25_as_TIOA6();
void oneshot_TIOA6_us(uint16_t pulse_us);
void oneshot_TIOA6_42MHz_Clock_Ticks(uint16_t pulse_ticks);

uint32_t b[32];


void setup() {
	
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(38400);
	Serial.println("hi");
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
	setup_timer_for_oneshot();
	setup_PIOC25_as_TIOA6();
}

void loop() {
	
	oneshot_TIOA6_us(35);
	delay_ms(1);
	
	oneshot_TIOA6_us(200);
	delay_ms(1);
	
	oneshot_TIOA6_us(1);
	delay_ms(1);
	
	oneshot_TIOA6_42MHz_Clock_Ticks(1);
	//delay_ms(1);
	
	oneshot_TIOA6_us(4);
	delay_ms(1);
}

void reset_Timer_Controller(){
	
	// TC1 channel 0 is instance number 30										pg. 38
	// disable quad decoder
	TC1->TC_BMR = 0;					// block mode register					pg. 901
	TC1->TC_QIDR = 7;					// disabl quad decoder Interrupts		pg. 904
	TC1->TC_FMR = 0;					// disable faults						pg. 907
	
	
	// TC2 channel 0 is instance number 33
	TC2->TC_BMR = 0;					// block mode register					pg. 901
	TC2->TC_QIDR = 7;					// disabl quad decoder Interrupts		pg. 904
	TC2->TC_FMR = 0;					// disable faults						pg. 907
}

void setup_timer_for_ms_delay(){
	
	PMC->PMC_PCER0 = b[30];			// enable clock to TC 1 channel 0			pg. 542
	TC1->TC_WPMR = 0x54494D << 8;		// unlock write protect registers		pg. 908
	
	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;	// disable the clock			pg. 880
	TC1->TC_CHANNEL[0].TC_CMR = 0		// clear out capture/waveform mode		pg. 881
		| TC_CMR_TCCLKS_TIMER_CLOCK1			// 42 MHz
		| TC_CMR_WAVE							// select waveform mode.
		| TC_CMR_CPCSTOP						// stop the clock when an RC compare occurs.
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

void setup_timer_for_oneshot(){
	
	// TC2 channel 0 is instance number 33										pg. 39, 859
	PMC->PMC_PCER1 = PMC_PCER1_PID33;	// enable clock to TC 1 channel 0		pg. 542
	TC2->TC_WPMR = 0x54494D << 8;		// unlock write protect registers		pg. 908
	
	TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS;	// disable the clock			pg. 880
	TC2->TC_CHANNEL[0].TC_CMR = 0		// 	Channel Mode register				pg. 881
		| TC_CMR_TCCLKS_TIMER_CLOCK1			// 42 MHz
		| TC_CMR_WAVE							// select waveform mode.
		//| TC_CMR_CPCSTOP						// stop the clock when an RC compare occurs.
		| TC_CMR_CPCDIS							// disable the clock when an RC compare occurs.
		| TC_CMR_WAVSEL_UP						// count just up (no external trigger on rc compary)
												// so it doesn't restart the clock.
		| TC_CMR_ACPA_SET						// RA causes TIOA6 to go HIGH
		| TC_CMR_ACPC_CLEAR						// RC causes TIOA6 to go LOW
	;
	
	// set RA to zero to trigger the pulse at beginning of cycle.
	// note: setting RA = 0 does not work.
	TC2->TC_CHANNEL[0].TC_RA = 1;				//								pg. 889
	// set RC to the pulse length (in the actual pulse function)
	TC2->TC_CHANNEL[0].TC_RC = 0;				//								pg. 891
	
	//TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;	// enable the clock				pg. 880
	// consider checking the status register to ensure the clock is actually enabled.
	// return true/false to know if it was set up successfully.
}

void setup_PIOC25_as_TIOA6(){
	PMC->PMC_PCER0 = PMC_PCER0_PID13;			// clock for PIOC				pg. 542
	PIOC->PIO_WPMR = 0x50494F << 8;				// enable writing to registers	pg. 674
	PIOC->PIO_PUDR = PIO_PUDR_P25;				// disable pull up resistor		pg. 622, 653
	PIOC->PIO_PDR = PIO_PDR_P25;				// disable PIO controller		pg. 622, 634
	PIOC->PIO_ABSR = PIO_ABSR_P25;				// use peripheral B				pg. 622, 656
}

void oneshot_TIOA6_us(uint16_t pulse_us){
	
	if (TC2->TC_CHANNEL[0].TC_SR & TC_SR_CLKSTA) return;			// if clock is already enabled (oneshot in progress)	pg. 892
	
	// load the time pulse into RC.
	TC2->TC_CHANNEL[0].TC_RC = 42 * (pulse_us + 1);	//							pg. 891
	
	// start the pulse.
	TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG | TC_CCR_CLKEN;	// perform a software trigger.	pg. 880 
}

// each ticket is about 22 ns long.
void oneshot_TIOA6_42MHz_Clock_Ticks(uint16_t pulse_ticks){
	
	if (TC2->TC_CHANNEL[0].TC_SR & TC_SR_CLKSTA) return;			// if clock is already enabled (oneshot in progress)	pg. 892
	
	// load the time pulse into RC.
	TC2->TC_CHANNEL[0].TC_RC = (pulse_ticks + 1);	//							pg. 891
	
	// start the pulse.
	TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG | TC_CCR_CLKEN;	// perform a software trigger.	pg. 880 
}




