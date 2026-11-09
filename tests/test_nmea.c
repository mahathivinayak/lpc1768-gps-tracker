#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nmea.h"

/* Append a correct *XX checksum so test sentences are easy to write. */
static void with_checksum(const char *body, char *out)
{
    unsigned char sum = 0;
    const char *p;
    for (p = body + 1; *p; p++) sum ^= (unsigned char)*p;
    sprintf(out, "%s*%02X", body, sum);
}

int main(void)
{
    gps_fix_t g;
    char s[128];

    /* Classic reference sentence */
    assert(nmea_parse_rmc("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A", &g));
    assert(g.hh == 12 && g.mm == 35 && g.ss == 19);
    assert(g.day == 23 && g.month == 3 && g.year == 94);
    assert(g.lat_udeg == 48117300);
    assert(g.lon_udeg == 11516666);
    assert(g.speed_centiknots == 2240);

    /* Corrupted checksum is rejected */
    assert(!nmea_parse_rmc("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6B", &g));

    /* Southern/western hemisphere, GNRMC talker */
    with_checksum("$GNRMC,081836,A,3751.65,S,14507.36,W,000.0,360.0,130998,011.3,E", s);
    assert(nmea_parse_rmc(s, &g));
    assert(g.lat_udeg == -37860833);
    assert(g.lon_udeg == -145122666);

    /* No fix: time parsed, status 'V' */
    with_checksum("$GPRMC,235959,V,,,,,,,010126,,", s);
    assert(nmea_parse_rmc(s, &g));
    assert(g.status == 'V' && g.hh == 23);

    puts("all NMEA tests passed");
    return 0;
}
