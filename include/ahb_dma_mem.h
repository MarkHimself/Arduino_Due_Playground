#ifndef AHB_DMA_MEM_H
#define AHB_DMA_MEM_H
#include <Arduino.h>

/*
 create code that uses AHB DMA to copy an array to another array.
AHB DMA is on ch. 22
instance i.d.: 39  
instance name: DMAC



*/

bool setup_AHB_DMA_mem_to_mem();
void start_AHB_DMA_mem_to_mem(char *source, char *dest, uint8_t str_length);







#endif