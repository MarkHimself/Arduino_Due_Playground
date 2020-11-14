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
- Interrupts on analog value is read
- DMA
	- sending a string through UART to COM port
	- Updating PWM
*/

/* Set up digital output on digital pin 26.
d.p. 26 = PIOD1
*/

void reset_PIOD_Controlller();
void set_PIOD1_Output();
void set_PIOD1_State(bool ishigh);


uint32_t b[32];


void setup() {
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(9600);
	reset_PIOD_Controlller();
	set_PIOD1_Output();
	
}

void loop() {
	set_PIOD1_State(true);
	//Serial.println("on");
	delay(100);
	set_PIOD1_State(false);
	//Serial.println("off");
	delay(100);
}

void reset_PIOD_Controlller(){
	// reset it
	// HOW?
	
	// unlock it													pg. 630
	PIOD->PIO_WPMR = (0x50494F << 8);	//							pg. 674
}

void set_PIOD1_Output(){
	
	// disable pull up resistor. through PIO_PUDR					pg. 622
	PIOD->PIO_PUDR = b[1];	// 										pg. 653
	
	// pio enable/disable register									pg. 622
	PIOD->PIO_PER = b[1];	// 										pg. 633
	
	// output control												pg. 623
	PIOD->PIO_OER = b[1];	//										pg. 636
	
	// set pin to LOW
	PIOD->PIO_CODR = b[1];	// 										pg. 643
	
	
	// power management id is 14 									pg. 38
	PMC->PMC_PCER0 = b[14];			//								pg. 542
}

void set_PIOD1_State(bool ishigh){
	if (ishigh) PIOD->PIO_SODR = b[1];	// 							pg. 642
	else PIOD->PIO_CODR = b[1];			// 							pg. 643
}



