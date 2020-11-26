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
1000 ticks per period. 1 tick = 95.238095 ns = .095238095 us
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

/*
PWMH0 on PIOC3 Peripheral B - Arduino D.P. 35

*/
void setup_pwmSyncPDC_ch_0_PIOC3(){
	// pioc instance 13
	PMC->PMC_PCER0 = PMC_PCER0_PID13;		// turn on clock for PIOC			pg. 542
	// pwm instance 36
	PMC->PMC_PCER1 = PMC_PCER1_PID36;		// turn on clock for PWM			pg. 563
	
	// disable pwm on channel 0
	PWM->PWM_DIS = PWM_DIS_CHID0;	// 										pg. 1008
	
	// *** Setup PIOC3 Pin to use PWM ***
	// disable pull up resistor. through PIO_PUDR					pg. 622
	PIOC->PIO_PUDR = PIO_PUDR_P3;	// 										pg. 653
	// disable PIO control of PIOC
	PIOC->PIO_PDR = PIO_PDR_P3;	// 										pg. 634
	// select peripheral B
	PIOC->PIO_ABSR |= PIO_ABSR_P3;
	
	// *** setup pwm registers ***
	
	// Not using CLKA, CLKB
	PWM->PWM_CLK = 0;	// 											pg. 1006
	
	PWM->PWM_CH_NUM[0].PWM_CMR = 0 		// channel mode				pg. 1044
		| PWM_CMR_CPRE_MCK_DIV_8		// use: master_clock / 8
		| PWM_CMR_CPOL		// Start HIGH at beginning of pulse
	;
	
	PWM->PWM_CH_NUM[0].PWM_CDTY = 0;	// duty cycle				pg. 1046
	
	// pwm comparisons.
	PWM->PWM_CMP[0].PWM_CMPM = 0;
	
	// pwm channel period will be 1000
	PWM->PWM_CH_NUM[0].PWM_CPRD = 1000;		//						pg. 1048
	
	PWM->PWM_SCM = 0			// synchronous channels register	pg. 1014
		| PWM_SCM_SYNC0			// ch. 0 is a synchronous channel
		| PWM_SCM_UPDM_MODE2	// use PDC to update values.
	;
	
	PWM->PWM_SCUP = PWM_SCUP_UPR(0);	// update PWM duty cycle every time.	pg. 1017
	//PWM->PWM_IER2 = PWM_IER2_ENDTX;		// pdc transfer done int enable.		pg. 1019
	
	// enable PWM on channel 0
	PWM->PWM_ENA = PWM_ENA_CHID0;	// 								pg. 1007
}

/*
read = transmit
write = receive
*/
void setup_PDC_for_pwm_ch_0(){
	
	PWM->PWM_TNPR = 0;		// transmit next pointer register.					pg. 515
	PWM->PWM_TNCR = 0;		// transmit next counter register.					pg. 516
	PWM->PWM_PTCR = 0		// transfer control register						pg. 517
		| PWM_PTCR_RXTDIS	// receive transfer disable
		| PWM_PTCR_TXTDIS	// transmit transfer disable
	;
}

void start_pwmSyncPDC_ch_0_PIOC3(uint16_t *buf, uint8_t length){
	PWM->PWM_TPR = (uint32_t)buf;	// transmit pointer register is buffer with values pg. 511
	PWM->PWM_TCR = length;			// buffer size to transmit.					pg. 512
	PWM->PWM_SCUC = PWM_SCUC_UPDULOCK;	// update unlock						pg. 1016
	PWM->PWM_PTCR = PWM_PTCR_TXTEN;	// Transmitter Transfer Enable				pg. 517
}





