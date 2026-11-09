#ifndef NMEA_H
#define NMEA_H
#include <stdint.h>

typedef struct {
    uint8_t  hh, mm, ss;
    uint8_t  day, month, year;
    int32_t  lat_udeg;          /* micro-degrees, +N / -S */
    int32_t  lon_udeg;          /* micro-degrees, +E / -W */
    uint32_t speed_centiknots;
    char     status;            /* 'A' = valid fix, 'V' = no fix */
} gps_fix_t;

int nmea_checksum_ok(const char *line);
int nmea_parse_rmc(const char *line, gps_fix_t *out);   /* 1 = parsed */

#endif
