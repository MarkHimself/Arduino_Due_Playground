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

/* Set up reading digital input on pin 40
D.P. 40 = PC8
Ch. 31 is PIO Controller
*/

void PC8_Input_Mode();
void PC8_Pullup_Enable();
void PC8_Pullup_Disable();
bool PC8_Input_State();

uint32_t b[32];


void setup() {
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(9600);
	PC8_Input_Mode();
	PC8_Pullup_Enable();
}

void loop() {
	
	if (PC8_Input_State() == true){
		Serial.println("ON");
	}
	else {
		Serial.println("OFF");
	}
	delay(100);
}

void PC8_Input_Mode(){
	// pin multiplexing
	PIOC->PIO_PER = b[8];				// pin controlled by PIO controller
	PIOC->PIO_ODR = b[8];				// output disable register
	PIOC->PIO_IFDR = b[8];				// disable input filter
	
	// power management
	PMC->PMC_PCER0 = b[13];					// enable clock for PIOC.			pg. 542, 38
}

void PC8_Pullup_Enable(){
	PIOC->PIO_PUER = b[8];
}

void PC8_Pullup_Disable(){
	PIOC->PIO_PUDR = b[8];
}

bool PC8_Input_State(){
	return PIOC->PIO_PDSR & b[8];		// return status of pin
}




