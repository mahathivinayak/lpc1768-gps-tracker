#include <string.h>
#include "nmea.h"

#define MAX_FIELDS 20
#define MAX_LINE   96

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int nmea_checksum_ok(const char *line)
{
    uint8_t sum = 0;
    const char *p;
    int hi, lo;

    if (line[0] != '$') return 0;
    for (p = line + 1; *p && *p != '*'; p++) sum ^= (uint8_t)*p;
    if (*p != '*') return 0;
    hi = hexval(p[1]);
    lo = hexval(p[2]);
    if (hi < 0 || lo < 0) return 0;
    return sum == (uint8_t)((hi << 4) | lo);
}

/* Split in place on ','. Unlike strtok, keeps empty fields (",,"). */
static int split_fields(char *buf, char *fields[], int max)
{
    int n = 0;
    char *p;
    fields[n++] = buf;
    for (p = buf; *p && n < max; p++) {
        if (*p == ',')      { *p = '\0'; fields[n++] = p + 1; }
        else if (*p == '*') { *p = '\0'; break; }
    }
    return n;
}

static uint8_t two_digits(const char *s)
{
    return (uint8_t)((s[0] - '0') * 10 + (s[1] - '0'));
}

/* "ddmm.mmmmm" (deg_digits=2) or "dddmm.mmmmm" (deg_digits=3) -> micro-degrees.
   Integer-only so no float library is needed on the Cortex-M3. */
static int32_t coord_to_udeg(const char *s, int deg_digits)
{
    int32_t deg = 0, min_e5;
    int32_t scale = 10000;
    const char *f;
    int i;

    for (i = 0; i < deg_digits; i++) deg = deg * 10 + (s[i] - '0');
    min_e5 = two_digits(s + deg_digits) * 100000;        /* minutes x 1e5 */

    f = s + deg_digits + 2;
    if (*f == '.') {
        for (f++; *f >= '0' && *f <= '9' && scale > 0; f++) {
            min_e5 += (*f - '0') * scale;
            scale /= 10;
        }
    }
    /* 1 minute = 1/60 degree -> (min_e5 / 1e5) / 60 * 1e6 = min_e5 / 6 */
    return deg * 1000000 + min_e5 / 6;
}

static uint32_t parse_centi(const char *s)
{
    uint32_t v = 0;
    int frac = -1;
    for (; *s; s++) {
        if (*s == '.') { frac = 0; continue; }
        if (*s < '0' || *s > '9') break;
        if (frac < 0)       v = v * 10 + (uint32_t)(*s - '0');
        else if (frac < 2) { v = v * 10 + (uint32_t)(*s - '0'); frac++; }
    }
    if (frac < 0) frac = 0;
    while (frac < 2) { v *= 10; frac++; }
    return v;
}

/* $xxRMC,hhmmss.ss,A,ddmm.mmmm,N,dddmm.mmmm,E,speed,course,ddmmyy,...*CS */
int nmea_parse_rmc(const char *line, gps_fix_t *out)
{
    char  buf[MAX_LINE];
    char *f[MAX_FIELDS];
    size_t len = strlen(line);
    int n;

    if (len >= sizeof buf || !nmea_checksum_ok(line)) return 0;
    memcpy(buf, line, len + 1);
    n = split_fields(buf, f, MAX_FIELDS);

    /* accept any talker ID: GPRMC, GNRMC, GLRMC... */
    if (n < 10 || strlen(f[0]) != 6 || strcmp(f[0] + 3, "RMC") != 0) return 0;

    memset(out, 0, sizeof *out);
    out->status = f[2][0] ? f[2][0] : 'V';

    if (strlen(f[1]) >= 6) {
        out->hh = two_digits(f[1]);
        out->mm = two_digits(f[1] + 2);
        out->ss = two_digits(f[1] + 4);
    }
    if (strlen(f[9]) == 6) {
        out->day   = two_digits(f[9]);
        out->month = two_digits(f[9] + 2);
        out->year  = two_digits(f[9] + 4);
    }

    if (out->status != 'A') return 1;          /* time is valid even without a fix */
    if (strlen(f[3]) < 4 || strlen(f[5]) < 5) return 0;

    out->lat_udeg = coord_to_udeg(f[3], 2);
    if (f[4][0] == 'S') out->lat_udeg = -out->lat_udeg;
    out->lon_udeg = coord_to_udeg(f[5], 3);
    if (f[6][0] == 'W') out->lon_udeg = -out->lon_udeg;
    out->speed_centiknots = parse_centi(f[7]);
    return 1;
}
