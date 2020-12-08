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


// data type for transferring/receiving data through spi with the use of interrupts.
template<int S>
class SPI_INT_DATA{
	public:
		SPI_INT_DATA();
		bool isStartNewTransactionRequest();
		bool isTransactionInProgress();
		bool isTransactionFinished();
		
		void setNewTransactionRequest();			// new transaction
		void resetNewTransactionRequest();
		void setTransactionInProgress();			// transaction in progress
		void resetTransactionInProgress();
		void setTransactionFinished();				// transaction finished
		void resetTransactionFinished();
		
		uint8_t getLocation();
		void incrementLocation();
		void resetLocation();
		bool isEndOfTransfer();
		uint8_t getTransLength();
		void setTransLength(uint8_t length);
		
		uint8_t toTransfer[S];						// array of values that are to be transferred to peripheral device
		uint8_t received[S];						// array that stores received values from peripheral device. (including starting byte which shouldn't matter.)
		
	private:
		
		
		uint8_t loc;								// location in array to transfer the next byte.
		uint8_t transLength;						// amount of bytes to transfer
		
		// control bits
		bool startNewTransaction;					// set this bit to start a new transaction.
		bool transactionInProgress;					// informs user if a transaction is occuring right now.
		bool transactionFinished;					// only used to inform the user the current transaction finished. it is then reset.
};

template<int S>
SPI_INT_DATA<S>::SPI_INT_DATA(){
	loc = 0;
	transLength = 0;
	startNewTransaction = false;
	transactionInProgress = false;
	transactionFinished = true;
}

template<int S>
bool SPI_INT_DATA<S>::isStartNewTransactionRequest(){
	return startNewTransaction;
}

template<int S>
bool SPI_INT_DATA<S>::isTransactionInProgress(){
	return transactionInProgress;
}

template<int S>
bool SPI_INT_DATA<S>::isTransactionFinished(){
	return transactionFinished;
}

template<int S>
void SPI_INT_DATA<S>::setNewTransactionRequest(){
	startNewTransaction = true;
}

template<int S>
void SPI_INT_DATA<S>::resetNewTransactionRequest(){
	startNewTransaction = false;
}

template<int S>
void SPI_INT_DATA<S>::setTransactionInProgress(){
	transactionInProgress = true;
}

template<int S>
void SPI_INT_DATA<S>::resetTransactionInProgress(){
	transactionInProgress = false;
}

template<int S>
void SPI_INT_DATA<S>::setTransactionFinished(){
	transactionFinished = true;
}

template<int S>
void SPI_INT_DATA<S>::resetTransactionFinished(){
	transactionFinished = false;
}

template<int S>
uint8_t SPI_INT_DATA<S>::getLocation(){
	return loc;
}

template<int S>
void SPI_INT_DATA<S>::incrementLocation(){
	loc++;
}

template<int S>
void SPI_INT_DATA<S>::resetLocation(){
	loc = 0;
}

template<int S>
bool SPI_INT_DATA<S>::isEndOfTransfer(){
	return (loc >= transLength);
}

template<int S>
uint8_t SPI_INT_DATA<S>::getTransLength(){
	return transLength;
}

template<int S>
void SPI_INT_DATA<S>::setTransLength(uint8_t length){
	if (!transactionInProgress) transLength = length;
}


#endif