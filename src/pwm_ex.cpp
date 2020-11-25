#include "pwm_ex.h"



void reset_PWM_Controller(){
	// reset PWM
	// how?
	
	// clear write protect register
	PIOC->PIO_WPMR = (0x50494F << 8);	// 							pg. 674
	PWM->PWM_WPCR = 0x50574D << 8;		// pwm						pg. 1037
	
	// pioc instance 13
	PMC->PMC_PCER0 = PMC_PCER0_PID13;		// turn on clock for PIOC			pg. 542
	
	// pwm instance 36
	PMC->PMC_PCER1 = PMC_PCER1_PID36;		// turn on clock for PWM			pg. 563
	
}

/*
pwm clock frequency: 	10.5 MHz
Period:					95.238095 us
1000 ticks per period. 1 tick = 95.238095 ns
*/
void setup_PIOC6_PWM(){
	
	// disable pwm on channel 2
	PWM->PWM_DIS = PWM_DIS_CHID2;	// 										pg. 1008
	
	// *** Setup PIOC Pin to use PWM ***
	// disable pull up resistor. through PIO_PUDR					pg. 622
	PIOC->PIO_PUDR = PIO_PUDR_P6;	// 										pg. 653
	// disable PIO control of PIOC
	PIOC->PIO_PDR = PIO_PDR_P6;	// 										pg. 634
	// select peripheral B
	PIOC->PIO_ABSR |= PIO_ABSR_P6;
	
	// *** setup pwm registers ***
	
	// Not using CLKA, CLKB
	PWM->PWM_CLK = 0;	// 											pg. 1006
	
	PWM->PWM_CH_NUM[2].PWM_CMR = 0 		// channel mode				pg. 1044
		| 0b0011			// use: master_clock / 8
		//| PWM_CMR_CPOL		// Start LOW at beginning of pulse
		// i want it to start high, so not using CPOL.
	;
	
	PWM->PWM_CH_NUM[2].PWM_CDTY = 0;	// duty cycle				pg. 1046
	
	// pwm comparisons.
	PWM->PWM_CMP[2].PWM_CMPM = 0;
	
	
	// pwm channel period will be 1000
	PWM->PWM_CH_NUM[2].PWM_CPRD = 1000;		//						pg. 1048
	
	// enable PWM on channel 2
	PWM->PWM_ENA = PWM_ENA_CHID2;	// 										pg. 1007
}

void write_PIOC6_PWM_Value(uint16_t val){
	if (val > 1000) val = 1000;
	PWM->PWM_CH_NUM[2].PWM_CDTYUPD = val;
}

void setup_pwm_ch_2_int(){
	PWM->PWM_IDR1 = PWM_IDR1_CHID2;		// disable ch. 2 interrupts			pg. 1011
	NVIC_SetPriority(PWM_IRQn, 0);
	NVIC_EnableIRQ(PWM_IRQn);
}

void enable_pwm_ch_2_int(){
	PWM->PWM_IER1 = PWM_IER1_CHID2;		// enable ch. 2 interrupts			pg. 1010
}

void disable_pwm_ch_2_int(){
	PWM->PWM_IDR1 = PWM_IDR1_CHID2;		// disable ch. 2 interrupts			pg. 1011
}








