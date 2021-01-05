#include "timer_counter.h"


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
	
	// TC2 channel 1. instance number 34 and also instance name TC7
}

void setup_timer_for_ms_delay(){
	
	PMC->PMC_PCER0 = PMC_PCER0_PID30;			// enable clock to TC 1 channel 0			pg. 542
	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;	// enable the clock						pg. 880
	TC1->TC_WPMR = 0x54494D << 8;		// unlock write protect registers		pg. 908
	
	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;	// disable the clock			pg. 880
	TC1->TC_CHANNEL[0].TC_CMR = 0		// clear out capture/waveform mode		pg. 881
		| TC_CMR_TCCLKS_TIMER_CLOCK1			// 42 MHz
		| TC_CMR_WAVE							// select waveform mode.
		| TC_CMR_CPCSTOP						// stop the clock when an RC compare occurs.
		| TC_CMR_WAVSEL_UP						// count just up (no external trigger on rc compary)
												// so it doesn't restart the clock.
	;
	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;	// enable the clock						pg. 880
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
	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG | TC_CCR_CLKEN;	// start the counter			// pg. 880
	
	
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

bool TC2_1_int_flag = false;

void setup_TC2_1_for_interrupts(){
	//TC2_1_int_flag = false;
	
	// NVIC Controller.
	// functions for configuring it												pg. 164
	//__disable_irq();					// disable all irq's
	//NVIC_SetPriorityGrouping(??);
	NVIC_SetPriority(TC7_IRQn, 0);		// set priority of interrupt to be pretty important.
	NVIC_EnableIRQ(TC7_IRQn);			// found in sam3x8e.h
	//__enable_irq();						// enable irq's
	
	PMC->PMC_PCER1 = PMC_PCER1_PID34;	// enable clock to TC2 channel 1		pg. 563
	TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKDIS;		// disable clock			pg. 880
	TC2->TC_CHANNEL[1].TC_CMR = 0					// waveform mode			pg. 883
		| TC_CMR_TCCLKS_TIMER_CLOCK2	// MCK/8 = 10.5 MHz
		| TC_CMR_WAVSEL_UP_RC			// restart on RC compare
		| TC_CMR_WAVE					// wave mode
	;
	// Actual interrupt period will be updated by user.
	TC2->TC_CHANNEL[1].TC_RC = 10500 * 100;			// interrupt period. 100ms	pg. 891
}

void TC2_1_interrupt_period(uint16_t fire_ms){
	// get clock status to determine if interrupts were disabled or not.
	bool TC2_1_ClockStatus = TC2->TC_CHANNEL[1].TC_SR & TC_SR_CLKSTA;	//		pg. 892
	
	TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKDIS;		// disable clock			pg. 880
	TC2->TC_CHANNEL[1].TC_RC = 10500 * fire_ms;			// interrupt period. 100ms	pg. 891
	if (TC2_1_ClockStatus){
		TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG | TC_CCR_CLKEN;	//			pg. 880
	}
}

// enables/disables interrupts and the clock.
void TC2_1_enable_interrupts(){
	// interrupt enabling.
	TC2->TC_CHANNEL[1].TC_IER = TC_IER_CPCS;		// enable interrupt on RC Compare. pg. 894
	TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG | TC_CCR_CLKEN;	//			pg. 880
}

void TC2_1_disable_interrupts(){
	TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKDIS;		// disable clock			pg. 880
	TC2->TC_CHANNEL[1].TC_IDR = TC_IDR_CPCS;		// disable interrupt on RC Compare. pg. 896
}

/*
void TC7_Handler(){
	uint32_t TC2_1_int_status = TC2->TC_CHANNEL[1].TC_SR;		// read status to allow for more interrupts.
	//NVIC_ClearPendingIRQ(TC7_IRQn);
	TC2_1_int_flag = true;
}
*/

bool get_TC2_1_int_flag(){
	bool pre_flag = TC2_1_int_flag;
	TC2_1_int_flag = false;
	return pre_flag;
}









