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







#endif