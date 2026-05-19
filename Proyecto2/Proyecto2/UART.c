#include "config.h"
#include "UART.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define UART_RX_BUFFER_SIZE 64

static volatile char rx_buffer_uart[UART_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

void UART_Init(uint32_t baud)
{
	uint16_t ubrr = (uint16_t)((F_CPU / (16UL * baud)) - 1UL);

	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;

	// Habilita RX, TX e interrupcion por recepcion.
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1
}

void UART_TxChar(char c)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

void UART_Print(const char *str)
{
	while (*str)
	{
		UART_TxChar(*str++);
	}
}

void UART_Println(const char *str)
{
	UART_Print(str);
	UART_Print("\r\n");
}

void UART_PrintUInt16(uint16_t value)
{
	char buffer[8];
	utoa(value, buffer, 10);
	UART_Print(buffer);
}

void UART_PrintUInt8(uint8_t value)
{
	char buffer[5];
	utoa(value, buffer, 10);
	UART_Print(buffer);
}

uint8_t UART_Available(void)
{
	return (rx_head != rx_tail);
}

char UART_RxChar(void)
{
	char data;

	while (!UART_Available());

	data = rx_buffer_uart[rx_tail];
	rx_tail = (uint8_t)((rx_tail + 1) % UART_RX_BUFFER_SIZE);

	return data;
}

ISR(USART_RX_vect)
{
	char data = UDR0;
	uint8_t next_head = (uint8_t)((rx_head + 1) % UART_RX_BUFFER_SIZE);

	// Si el buffer no esta lleno, guardar el caracter recibido.
	// Si se llena, se descarta el caracter nuevo para no bloquear el programa.
	if (next_head != rx_tail)
	{
		rx_buffer_uart[rx_head] = data;
		rx_head = next_head;
	}
}
