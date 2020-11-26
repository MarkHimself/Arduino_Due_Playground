#ifndef PWM_EX_H
#define PWM_EX_H
#include <Arduino.h>

/* Set up PWM on digital pin 38
it is pin PIOC6
PWM: peripheral B, PWML2 - PWM Channel 2 Output Low
write 0 to 1000 for PWM control.
	0 = Low
	1000 = High
	CPOL looks like it's used backwards.
	Maybe because i'm using PWML and not PWMH
*/

void reset_PWM_Controller();
void setup_PIOC6_PWM();
void write_PIOC6_PWM_Value(uint16_t val);


void setup_pwm_ch_2_int();
void enable_pwm_ch_2_int();
void disable_pwm_ch_2_int();

void setup_pwmSyncPDC_ch_0_PIOC3();
void setup_PDC_for_pwm_ch_0();
void start_pwmSyncPDC_ch_0_PIOC3(uint16_t *buf, uint8_t length);




#endif