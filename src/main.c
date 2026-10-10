#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

static void fail(const char *message) { fprintf(stderr, "%s\n", message); exit(2); }
double number(const char *s) {
    char *end; errno = 0; double value = strtod(s, &end);
    if (errno || end == s || *end || !isfinite(value)) fail("invalid finite number");
    return value;
}
static size_t fields(char *line, char **out, size_t capacity) {
    line[strcspn(line, "\r\n")] = 0;
    size_t count = 0;
    char *p = line;
    while (*p) {
        if (count == capacity) fail("too many fields");
        out[count++] = p;
        char *separator = strchr(p, ',');
        if (!separator) break;
        *separator = 0; p = separator + 1;
        if (!*p) fail("empty field");
    }
    return count;
}

static uint16_t crc(const uint8_t *data, size_t size) {
    uint16_t value = 0xffff;
    for (size_t i = 0; i < size; i++) {
        value ^= data[i];
        for (int bit = 0; bit < 8; bit++) value = (value >> 1) ^ ((value & 1) ? 0xa001 : 0);
    }
    return value;
}
static size_t decode(char *line, uint8_t *bytes) {
    char *items[256]; size_t n = fields(line, items, 256);
    for (size_t i = 0; i < n; i++) {
        char *end; errno = 0; unsigned long value = strtoul(items[i], &end, 16);
        if (errno || end == items[i] || *end || value > 255) fail("invalid byte");
        bytes[i] = (uint8_t)value;
    }
    return n;
}
static int inspect(const uint8_t *bytes, size_t n, int print) {
    if (n < 5 || bytes[0] == 0 || bytes[0] > 247) return 0;
    uint16_t received = (uint16_t)(bytes[n - 2] | bytes[n - 1] << 8);
    if (crc(bytes, n - 2) != received) return 0;
    if (bytes[1] & 0x80) {
        if (n != 5 || bytes[2] == 0) return 0;
        if (print) printf("slave=%u exception=%u\n", bytes[0], bytes[2]);
        return 1;
    }
    if ((bytes[1] != 3 && bytes[1] != 4) || bytes[2] % 2 || bytes[2] == 0 || n != (size_t)bytes[2] + 5) return 0;
    if (print) {
        printf("slave=%u function=%u registers=", bytes[0], bytes[1]);
        for (size_t i = 3; i + 1 < n - 2; i += 2) printf("%s%u", i == 3 ? "" : ",", bytes[i] * 256u + bytes[i + 1]);
        puts("");
    }
    return 1;
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        uint8_t frame[] = {1, 3, 2, 0, 10, 0, 0}; uint16_t c = crc(frame, 5);
        frame[5] = c & 255; frame[6] = c >> 8;
        assert(inspect(frame, 7, 0)); frame[3] ^= 1; assert(!inspect(frame, 7, 0));
        assert(crc((const uint8_t *)"123456789", 9) == 0x4b37);
        puts("CRC and corruption tests passed"); return 0;
    }
    char line[4096]; unsigned rejected = 0, accepted = 0;
    while (fgets(line, sizeof line, stdin)) {
        uint8_t bytes[256]; size_t n = decode(line, bytes);
        if (inspect(bytes, n, 1)) accepted++; else { rejected++; puts("rejected"); }
    }
    if (ferror(stdin) || accepted + rejected == 0) fail("missing input");
    printf("accepted=%u rejected=%u\n", accepted, rejected);
    return rejected ? 1 : 0;
}
