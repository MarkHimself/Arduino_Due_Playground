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
	//PWM->PWM_CH_NUM[0].PWM_CDTY = 100;	// duty cycle				pg. 1046
	
	// pwm comparisons.
	PWM->PWM_CMP[0].PWM_CMPM = 0;
	
	// pwm channel period will be 1000
	PWM->PWM_CH_NUM[0].PWM_CPRD = 1000;		//						pg. 1048
	
	PWM->PWM_SCM = 0			// synchronous channels register	pg. 1014
		| PWM_SCM_SYNC0			// ch. 0 is a synchronous channel
		| PWM_SCM_UPDM_MODE2	// use PDC to update values.
		//| (0x03 << 16)			// reserved value. lets just see what happens...
	;
	
	//PWM->PWM_SCUP = PWM_SCUP_UPR(0);	// update PWM duty cycle every time.	pg. 1017
	PWM->PWM_SCUP = PWM_SCUP_UPR(3);	// update PWM duty cycle every 10 cycles.	pg. 1017
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

// *** AHB DMA for PWM *** //

uint8_t ahb_pwm_ch_num = 0;
bool setup_AHB_DMA_for_pwm(){
	uint32_t temp_var = 0;
	
	// setup clock for AHB DMA.
	PMC->PMC_PCER1 = PMC_PCER1_PID39;
	DMAC->DMAC_WPMR = 0x444D41 << 8;	// unlock write protect					pg. 380
	//DMAC->DMAC_EN = 0;					// disable DMA Controller				pg. 360
	DMAC->DMAC_EN = DMAC_EN_ENABLE;			// enable DMA Controller			pg. 360
	temp_var = DMAC->DMAC_EBCISR;		// read status of dma to clear int's.	pg. 367
	
	temp_var = DMAC->DMAC_CHSR;			// Channel handler status register		pg. 370
	// find a disabled channel.
	for (uint8_t i = 0, en_mask = 1; i < 6; i++, en_mask = en_mask << 1){
		if (temp_var & en_mask){
			if (i == 5) return false;		// couldn't find a disabled channel
		}
		else{
			ahb_pwm_ch_num = i;
			break;
		}
	}
	//DMAC->DMAC_GCFG = DMAC_GCFG_ARB_CFG_ROUND_ROBIN;	// round robit priority	pg. 359
	DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_DSCR = 0;	//						pg. 373
	return true;
}

void start_AHB_DMA_for_pwm(uint16_t *source, uint16_t length){
	
	DMAC->DMAC_CHDR = (1 << ahb_pwm_ch_num);		// Disable the channel		pg. 369
	while(DMAC->DMAC_CHSR & (1 << ahb_pwm_ch_num)){	// wait while its enabled 	pg. 370
		
	}
	uint32_t temp_var = 0;
	temp_var = DMAC->DMAC_EBCISR;		// read status of dma to clear int's.	pg. 367
	
	DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_SADDR = (uint32_t)source;		// buffer source address	pg. 371
	//DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_DADDR = (uint32_t)&REG_PWM_DMAR;	// destination is PWM_DMAR.	pg. 372, S70: pg. 1204, 1239
	DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_DADDR = (uint32_t)&(PWM->Reserved1[0]);	// destination is PWM_DMAR.	pg. 372, S70: pg. 1204, 1239
	//DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_DADDR = (uint32_t)&PWM->PWM_CH_NUM[0].PWM_CDTYUPD;	// 		pg. 1047
	DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_DSCR = 0;	//						pg. 373
	DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_CTRLA = 0	// Control A reg.		pg. 374
		| DMAC_CTRLA_BTSIZE(length)	// buffer transfer size
		| DMAC_CTRLA_SCSIZE_CHK_1	// ???
		| DMAC_CTRLA_DCSIZE_CHK_1	// ???
		//| DMAC_CTRLA_SCSIZE_CHK_8	// ???
		//| DMAC_CTRLA_DCSIZE_CHK_8	// ???
		//| DMAC_CTRLA_SCSIZE_CHK_256	// ???
		//| DMAC_CTRLA_DCSIZE_CHK_256	// ???
		| DMAC_CTRLA_SRC_WIDTH_HALF_WORD	// 16 bit source
		//| DMAC_CTRLA_DST_WIDTH_HALF_WORD	// 16 bit destination
		| DMAC_CTRLA_DST_WIDTH_WORD	// 32 bit destination
	;
	DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_CTRLB = 0	// Control B reg.		pg. 376
		//| DMAC_CTRLB_SRC_DSCR_FETCH_FROM_MEM			// fetch source descriptor - it's zero anyways.
		//| DMAC_CTRLB_DST_DSCR_FETCH_FROM_MEM			// fetch destinaiton descriptor - it's zero anyways.
		| DMAC_CTRLB_SRC_DSCR_FETCH_DISABLE				// fetch source descriptor disable
		| DMAC_CTRLB_DST_DSCR_FETCH_DISABLE				// fetch destination descriptor disable
		| DMAC_CTRLB_FC_MEM2PER_DMA_FC					// memory to peripheral
		| DMAC_CTRLB_SRC_INCR_INCREMENTING				// increment source address during transfer
		| DMAC_CTRLB_DST_INCR_FIXED						// destination address unchanged.
	;
	
	/*
	Notes for memory to peripheral transfer.
	Table 22-2 on pg. 339
	PWM Transmit 15: 	transmit to the PWM peripheral. interface number 15
	ex: SPI0 Receive 2: 	receive from SPI0. interface number 2
	DST_PER: Connect it to the transmit interface number.
	Use destination hardware handshaking.
	*/
	DMAC->DMAC_CH_NUM[ahb_pwm_ch_num].DMAC_CFG = 0		// configuration register	pg. 378
		| DMAC_CFG_DST_PER(15)							// destination peripheral is PWM
		//| DMAC_CFG_SRC_PER(15)							// source peripheral is PWM
		| DMAC_CFG_SRC_H2SEL_SW							// source software handshaking ???		// not much info on what to choose
		//| DMAC_CFG_SRC_H2SEL_HW							// source hardware handshaking ???		// not much info on what to choose
		//| DMAC_CFG_DST_H2SEL_SW							// destination software handshaking ???	// for memory to peripheral xfer
		| DMAC_CFG_DST_H2SEL_HW							// destination hardware handshaking ???	// for memory to peripheral xfer
		| DMAC_CFG_SOD									// disable dma when done.		???
		| DMAC_CFG_FIFOCFG_ASAP_CFG						// ???
		//| DMAC_CFG_AHB_PROT(2)							// ??? 
	;
	
	//PWM->PWM_SCUC = PWM_SCUC_UPDULOCK;	// update unlock						pg. 1016
	DMAC->DMAC_EN = DMAC_EN_ENABLE;						// enable DMAC							pg. 360
	delay(1);
	DMAC->DMAC_LAST = 0x2 << (ahb_pwm_ch_num * 2);		// last transfer.		pg. 363
	DMAC->DMAC_CHER = (1 << ahb_pwm_ch_num);	// enable the channel			pg. 368
	//DMAC->DMAC_SREQ = 0x03 << (ahb_pwm_ch_num * 2);		// destination/source transfer request	pg. 361
	//DMAC->DMAC_SREQ = 0x01 << (ahb_pwm_ch_num * 2);		// destination transfer request			pg. 361
	//DMAC->DMAC_SREQ = 0x02 << (ahb_pwm_ch_num * 2);		// source transfer request				pg. 361
	//DMAC->DMAC_CREQ = 0x03 << (ahb_pwm_ch_num * 2);		// destination/source chunk transfer request	pg. 362
	DMAC->DMAC_CREQ = 0x02 << (ahb_pwm_ch_num * 2);			// destination chunk transfer request	pg. 362
	//DMAC->DMAC_CREQ = 0x01 << (ahb_pwm_ch_num * 2);			// source chunk transfer request	pg. 362
	//PWM->PWM_ENA = PWM_ENA_CHID0;	// 								pg. 1007
	
	while (DMAC->DMAC_CHSR & (1 << ahb_pwm_ch_num)){			// while ch. is enabled	pg. 370
		// wait for DMA to finish.
		Serial.println("A");
		Serial.println(PWM->PWM_ISR2, HEX);
		Serial.println(DMAC->DMAC_CHSR, HEX);
		Serial.println(DMAC->DMAC_EBCIMR, HEX);
		Serial.println(DMAC->DMAC_EBCISR, HEX);
		Serial.println("Z");
		Serial.println(PWM->PWM_SCUP >> 4);
		//DMAC->DMAC_CHDR = (1 << ahb_pwm_ch_num);		// Disable the channel		pg. 369
		//break;
	}
	
}


