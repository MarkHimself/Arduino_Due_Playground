#ifndef UART_H
#define UART_H
#include <stdint.h>
#include <Arduino.h>

/*
send UART messages to COM port
38400 baud rate
UART is Ch. 2
D.P. 1 = TX0 = UTXD on pin PIOA9 periopheral A
D.P. 2 = RX0 = URXD on pin PIOA8 peripheral A

UART is instance I.D. 8
UART supports only 8 bit (with parity)
	-Arduino COM port default: 8 data bits, 1 stop bit, no parity.
	- Add DMA to transmit the string.
		-Peripheral DMA Controller = PDC
		- PDC Ch. 26
		- info on how to trigger the DMA on page. 756
			- The TXRDY bit triggers the PDC channel data transfer of the transmitter
*/

void setup_UART();
bool transmit_UART(uint8_t val);
void setup_UART_empty_tx_int();
void setup_UART_TX_String_DMA();
void start_UART_TX_String_DMA_Transfer(char *str, uint8_t str_length);
// add code to change baud rate.

void setup_PIOA8_as_UART_RX();
void setup_PIOA9_as_UART_TX();


#endif