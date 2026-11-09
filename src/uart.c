#include "LPC17xx.h"
#include "uart.h"

/* Assumes CCLK = 100 MHz (Keil system_LPC17xx.c default) and
   PCLK = CCLK/4 = 25 MHz (PCLKSEL reset value). */

#define LSR_RDR   (1u << 0)     /* receive data ready      */
#define LSR_THRE  (1u << 5)     /* transmit holding empty  */

#define RX_BUF_SIZE 256u        /* must be a power of two */
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint32_t rx_head, rx_tail;

void uart0_init_115200(void)
{
    LPC_SC->PCONP |= (1u << 3);                                   /* power UART0 */
    LPC_PINCON->PINSEL0 = (LPC_PINCON->PINSEL0 & ~(0xFu << 4))
                        | (0x5u << 4);                            /* P0.2=TXD0, P0.3=RXD0 */

    LPC_UART0->LCR = 0x83;          /* 8N1, DLAB = 1 */
    LPC_UART0->DLM = 0;
    LPC_UART0->DLL = 9;
    LPC_UART0->FDR = (2u << 4) | 1u;/* MULVAL=2, DIVADDVAL=1:
                                       25e6 / (16*9*1.5) = 115740 baud (+0.47%) */
    LPC_UART0->LCR = 0x03;          /* DLAB = 0 */
    LPC_UART0->FCR = 0x07;          /* enable and reset FIFOs */
}

void uart0_putc(char c)
{
    while (!(LPC_UART0->LSR & LSR_THRE)) { }
    LPC_UART0->THR = (uint8_t)c;
}

void uart0_puts(const char *s)
{
    while (*s) uart0_putc(*s++);
}

void uart2_init_9600(void)
{
    LPC_SC->PCONP |= (1u << 24);                                  /* UART2 is off after reset */
    LPC_PINCON->PINSEL0 = (LPC_PINCON->PINSEL0 & ~(0xFu << 20))
                        | (0x5u << 20);                           /* P0.10=TXD2, P0.11=RXD2 */

    LPC_UART2->LCR = 0x83;
    LPC_UART2->DLM = 0;
    LPC_UART2->DLL = 163;           /* 25e6 / (16*163) = 9586 baud (-0.15%) */
    LPC_UART2->FDR = 0x10;          /* fractional divider bypassed */
    LPC_UART2->LCR = 0x03;
    LPC_UART2->FCR = 0x07;          /* FIFO on, RX trigger = 1 byte */
    LPC_UART2->IER = 0x01;          /* RBR interrupt */

    NVIC_EnableIRQ(UART2_IRQn);
}

void UART2_IRQHandler(void)
{
    while (LPC_UART2->LSR & LSR_RDR) {
        uint8_t  b    = (uint8_t)LPC_UART2->RBR;     /* reading RBR clears the interrupt */
        uint32_t next = (rx_head + 1u) & (RX_BUF_SIZE - 1u);
        if (next != rx_tail) {                       /* drop byte if buffer full */
            rx_buf[rx_head] = b;
            rx_head = next;
        }
    }
}

int uart2_getc(char *c)
{
    if (rx_tail == rx_head) return 0;
    *c = (char)rx_buf[rx_tail];
    rx_tail = (rx_tail + 1u) & (RX_BUF_SIZE - 1u);
    return 1;
}
