#include <Arduino.h>
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
- DMA
*/

/* Set up reading analog input on pin A7
Arduino Analog Pin A7 = PIOA2 (PIOA2 if/when used as a digital pin)
Ch. 31 is PIO Controller
Ch. 43 is ADC Controller
Peripheral: X1 (extra function)
Signal: AD0
Channel: 0 		pg. 1326
ADC is 12 bits (from 0 = ground to 4095 = 3.3V)
*/

void reset_ADC();
void PIOA2_Analog_Input_Mode();
uint16_t PIOA2_Input_State();

uint32_t b[32];


void setup() {
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(9600);
	reset_ADC();
	PIOA2_Analog_Input_Mode();
}

void loop() {
	
	uint16_t adc_val = 0;
	adc_val = PIOA2_Input_State();
	
	Serial.println(adc_val);
	delay(100);
}

void reset_ADC(){
	ADC->ADC_CR = b[0];						// reset ADC						pg. 1332
	ADC->ADC_WPSR;							// clear any flags					pg. 1354
	ADC->ADC_WPMR = (0x414443 << 8);		// disable write protect			pg. 1353
	ADC->ADC_MR = 0
		| (4 << 8)							// ADC clock prescale
		//| (2 << 16)							// startup time is 16 periods of ADC clock
		| (6 << 16)							// startup time is 96 periods of ADC clock
		//| (2 << 20)							// settling time is 9 periods of ADC clock
		| (3 << 20)							// settling time is 17 periods of ADC clock
	;
}

void PIOA2_Analog_Input_Mode(){
	// pin multiplexing
	ADC->ADC_CHER = b[0];			// ADC_CHER - CHannel Enable Register pg. 1338
	// remove control from PIO (done with ADC_CHER? see pg. 1319)
	
	// single ended mode pg. 1326
	
	
	// power management
	PMC->PMC_PCER0 = b[11];					// enable clock for PIOA			pg. 
	PMC->PMC_PCER1 = b[5];					// enable clock for PIOC.			pg. 39, 1319, 563
}


uint16_t PIOA2_Input_State(){
	ADC->ADC_CR = b[1];						// start conversion					pg. 1332
	
	while (!(ADC->ADC_ISR & b[0])){};		// wait for conversion to finish	pg. 1345
	// this is ok since only have 1 adc channel.
	// better to have a class that checks if any finished (through the DRDY bit)
	// and then update whatever what finished.
	
	return ADC->ADC_CDR[0];					// return the data					pg. 
}




