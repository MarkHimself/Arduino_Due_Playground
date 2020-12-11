#include "dma_controller.h"
#include <Arduino.h>


void DMA_power_on(){
	// DMA Controller is instance ID 39.										pg. 39
	PMC->PMC_PCER1 = PMC_PCER1_PID39;	// enable clock to DMA Controller		pg. 563
}

void DMA_AHB_Unlock_Write_Protect(){
	DMAC->DMAC_WPMR = 0x444D41;			// unlock write protect					pg. 380
}











