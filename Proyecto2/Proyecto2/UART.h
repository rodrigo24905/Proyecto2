#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>

void UART_Init(uint32_t baud);
void UART_TxChar(char c);
void UART_Print(const char *str);
void UART_Println(const char *str);
void UART_PrintUInt16(uint16_t value);
void UART_PrintUInt8(uint8_t value);
uint8_t UART_Available(void);
char UART_RxChar(void);

#endif 
