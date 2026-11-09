#include "LPC17xx.h"
#include "uart.h"
#include "nmea.h"

/* SystemInit() is already called by the Keil startup file before main(). */

static void put_uint(uint32_t v, int min_width)
{
    char tmp[11];
    int i = 0;
    do { tmp[i++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (i < min_width) tmp[i++] = '0';
    while (i) uart0_putc(tmp[--i]);
}

static void put_udeg(int32_t v)
{
    if (v < 0) { uart0_putc('-'); v = -v; }
    put_uint((uint32_t)v / 1000000u, 1);
    uart0_putc('.');
    put_uint((uint32_t)v % 1000000u, 6);
}

static void print_fix(const gps_fix_t *g)
{
    uart0_puts("UTC ");
    put_uint(g->hh, 2); uart0_putc(':');
    put_uint(g->mm, 2); uart0_putc(':');
    put_uint(g->ss, 2);
    uart0_puts("  ");
    put_uint(g->day, 2); uart0_putc('/');
    put_uint(g->month, 2); uart0_putc('/');
    put_uint(g->year, 2);

    if (g->status == 'A') {
        uart0_puts("  lat ");  put_udeg(g->lat_udeg);
        uart0_puts("  lon ");  put_udeg(g->lon_udeg);
        uart0_puts("  spd ");  put_uint(g->speed_centiknots / 100, 1);
        uart0_putc('.');       put_uint(g->speed_centiknots % 100, 2);
        uart0_puts(" kn\r\n");
    } else {
        uart0_puts("  waiting for fix...\r\n");
    }
}

int main(void)
{
    char line[96];
    uint32_t len = 0;
    char c;
    gps_fix_t fix;

    uart0_init_115200();
    uart2_init_9600();
    uart0_puts("\r\nLPC1768 GPS tracker\r\n");

    for (;;) {
        while (!uart2_getc(&c)) __WFI();      /* sleep until the UART2 IRQ fires */

        if (c == '$') len = 0;                /* start of a new sentence */
        if (c == '\r') continue;
        if (c == '\n') {
            line[len] = '\0';
            if (nmea_parse_rmc(line, &fix)) print_fix(&fix);
            len = 0;
            continue;
        }
        if (len < sizeof line - 1) line[len++] = c;
        else len = 0;                         /* overlong line: discard */
    }
}
