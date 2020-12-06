#ifndef SPI_EX_H
#define SPI_EX_H
#include "Arduino.h"


// setup spi and pins
void setup_SPI0_Master(uint8_t spi_mode);
bool SPI0_Clock_Rate_khz(uint16_t f);
void setup_PIOA25_as_SPI0_MISO();
void setup_PIOA26_as_SPI0_MOSI();
void setup_PIOA27_as_SPI0_SCK();
bool SPI0_Enabled_Status();

// write/read through spi
uint8_t write_SPI0_blocking(uint8_t val);

// write/read through spi (non-blocking)
bool is_SPI0_Transmit_Available();
bool write_SPI0_nonblocking(uint8_t val);
bool is_SPI0_Data_Available();
int read_SPI0_nonblocking();

// spi interrupts
void SPI0_Enable_Int_TXEMPTY();
void SPI0_Disable_Int_TXEMPTY();
void SPI0_Enable_Int_RDRF();
void SPI0_Disable_Int_RDRF();


#endif