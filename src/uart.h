#ifndef UART_H
#define UART_H
#include <stdint.h>

void uart0_init_115200(void);   /* debug output to PC  : P0.2 TXD0, P0.3 RXD0 */
void uart0_putc(char c);
void uart0_puts(const char *s);

void uart2_init_9600(void);     /* GPS module input    : P0.10 TXD2, P0.11 RXD2 */
int  uart2_getc(char *c);       /* non-blocking, returns 1 if a byte was read */

#endif
