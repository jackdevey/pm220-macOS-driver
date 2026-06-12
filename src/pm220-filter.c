#include <cups/cups.h>
#include <cups/raster.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_DPI 203
#define DEFAULT_GAP_MM 5
#define DEFAULT_DENSITY 8
#define DEFAULT_SPEED 4
#define DEFAULT_DIRECTION 0

#define MIN_GAP_MM 0
#define MAX_GAP_MM 10
#define MIN_DENSITY 1
#define MAX_DENSITY 15
#define MIN_SPEED 1
#define MAX_SPEED 6

#define INVERT_BITS 1
#define FLIP_Y 0
#define FLIP_X 0

typedef struct {
    int gap_mm;
    int density;
    int speed;
    int direction;
} pm220_options_t;

static int get_1bit_pixel(const unsigned char *row, unsigned x) {
    unsigned byte_index = x / 8;
    unsigned bit_index = 7 - (x % 8);
    return (row[byte_index] >> bit_index) & 1;
}

static void set_bit(unsigned char *bitmap, unsigned width_dots, unsigned height_dots, unsigned bytes_per_line, unsigned x, unsigned y, int black) {
    if (x >= width_dots || y >= height_dots) return;

    if (black) {
        bitmap[(y * bytes_per_line) + (x / 8)] |= (0x80 >> (x % 8));
    }
}

static int parse_int_option(const char *value, int min_value, int max_value, int fallback) {
    char *end = NULL;
    long parsed;

    if (!value || !*value) {
        return fallback;
    }

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || parsed < min_value || parsed > max_value) {
        return fallback;
    }

    return (int)parsed;
}

static int dots_to_mm(unsigned dots, unsigned dpi) {
    unsigned long long numerator;
    unsigned long long denominator;
    unsigned long long rounded;

    if (dpi == 0) {
        dpi = DEFAULT_DPI;
    }

    numerator = (unsigned long long)dots * 254ULL;
    denominator = (unsigned long long)dpi * 10ULL;
    rounded = (numerator + (denominator / 2ULL)) / denominator;

    if (rounded < 1ULL) {
        return 1;
    }

    if (rounded > (unsigned long long)INT_MAX) {
        return INT_MAX;
    }

    return (int)rounded;
}

static pm220_options_t read_options(int argc, char *argv[]) {
    pm220_options_t config = {
        DEFAULT_GAP_MM,
        DEFAULT_DENSITY,
        DEFAULT_SPEED,
        DEFAULT_DIRECTION
    };
    cups_option_t *options = NULL;
    int num_options = 0;
    const char *value;

    if (argc <= 5 || !argv[5]) {
        return config;
    }

    num_options = cupsParseOptions(argv[5], 0, &options);

    value = cupsGetOption("PM220Gap", num_options, options);
    config.gap_mm = parse_int_option(value, MIN_GAP_MM, MAX_GAP_MM, config.gap_mm);

    value = cupsGetOption("PM220Density", num_options, options);
    config.density = parse_int_option(value, MIN_DENSITY, MAX_DENSITY, config.density);

    value = cupsGetOption("PM220Speed", num_options, options);
    config.speed = parse_int_option(value, MIN_SPEED, MAX_SPEED, config.speed);

    value = cupsGetOption("PM220Direction", num_options, options);
    config.direction = parse_int_option(value, 0, 1, config.direction);

    cupsFreeOptions(num_options, options);
    return config;
}

int main(int argc, char *argv[]) {
    cups_raster_t *ras;
    cups_page_header2_t header;
    pm220_options_t config = read_options(argc, argv);

    ras = cupsRasterOpen(0, CUPS_RASTER_READ);
    if (!ras) {
        fprintf(stderr, "ERROR: PM220 unable to open CUPS raster stream\n");
        return 1;
    }

    while (cupsRasterReadHeader2(ras, &header)) {
        unsigned in_w = header.cupsWidth;
        unsigned in_h = header.cupsHeight;
        unsigned bpl = header.cupsBytesPerLine;
        unsigned bpp = header.cupsBitsPerPixel;
        unsigned out_w_dots = in_w;
        unsigned out_h_dots = in_h;
        unsigned out_w_bytes = (out_w_dots + 7) / 8;
        int label_w_mm = dots_to_mm(out_w_dots, header.HWResolution[0]);
        int label_h_mm = dots_to_mm(out_h_dots, header.HWResolution[1]);
        size_t bitmap_size;

        fprintf(stderr,
            "DEBUG: PM220 raster page: cupsWidth=%u cupsHeight=%u bpl=%u bpp=%u colors=%u xdpi=%u ydpi=%u size=%dx%dmm gap=%dmm density=%d speed=%d direction=%d\n",
            header.cupsWidth,
            header.cupsHeight,
            header.cupsBytesPerLine,
            header.cupsBitsPerPixel,
            header.cupsNumColors,
            header.HWResolution[0],
            header.HWResolution[1],
            label_w_mm,
            label_h_mm,
            config.gap_mm,
            config.density,
            config.speed,
            config.direction
        );

        if (in_w == 0 || in_h == 0 || bpl == 0) {
            fprintf(stderr, "ERROR: PM220 bad raster header\n");
            cupsRasterClose(ras);
            return 1;
        }

        if (out_h_dots > SIZE_MAX / out_w_bytes) {
            fprintf(stderr, "ERROR: PM220 raster page too large\n");
            cupsRasterClose(ras);
            return 1;
        }

        bitmap_size = (size_t)out_h_dots * (size_t)out_w_bytes;

        unsigned char *bitmap = calloc(1, bitmap_size);
        if (!bitmap) {
            fprintf(stderr, "ERROR: PM220 out of memory\n");
            cupsRasterClose(ras);
            return 1;
        }

        unsigned char *row = calloc(1, bpl);
        if (!row) {
            fprintf(stderr, "ERROR: PM220 out of memory\n");
            free(bitmap);
            cupsRasterClose(ras);
            return 1;
        }

        for (unsigned y = 0; y < in_h; y++) {
            if (cupsRasterReadPixels(ras, row, bpl) < bpl) {
                fprintf(stderr, "ERROR: PM220 short raster read\n");
                free(row);
                free(bitmap);
                cupsRasterClose(ras);
                return 1;
            }

            if (y >= out_h_dots) continue;

            unsigned out_y = FLIP_Y ? (out_h_dots - 1 - y) : y;

            if (bpp == 1 && bpl == out_w_bytes && !FLIP_X) {
                for (unsigned i = 0; i < out_w_bytes; i++) {
                    unsigned char v = row[i];

                    if (INVERT_BITS) {
                        v ^= 0xFF;
                    }

                    bitmap[(out_y * out_w_bytes) + i] = v;
                }

                continue;
            }

            if (bpp == 1) {
                for (unsigned x = 0; x < in_w && x < out_w_dots; x++) {
                    unsigned out_x = FLIP_X ? (out_w_dots - 1 - x) : x;
                    int black = get_1bit_pixel(row, x);

                    if (INVERT_BITS) {
                        black = !black;
                    }

                    set_bit(bitmap, out_w_dots, out_h_dots, out_w_bytes, out_x, out_y, black);
                }
            } else {
                fprintf(stderr, "ERROR: PM220 unsupported raster bpp=%u\n", bpp);
                free(row);
                free(bitmap);
                cupsRasterClose(ras);
                return 1;
            }
        }

        free(row);

        /*
         * Match the standalone TSPL shape.
         * No media-moving commands.
         */
        printf("\n\n");
        printf("SIZE %d mm,%d mm\n", label_w_mm, label_h_mm);
        printf("GAP %d mm,0\n", config.gap_mm);
        printf("DENSITY %d\n", config.density);
        printf("SPEED %d\n", config.speed);
        printf("DIRECTION %d\n", config.direction);
        printf("REFERENCE 0,0\n");
        printf("CLS\n");
        printf("BITMAP 0,0,%u,%u,0,", out_w_bytes, out_h_dots);
        fwrite(bitmap, 1, bitmap_size, stdout);
        printf("\nPRINT 1,1\n\n");
        fflush(stdout);
        free(bitmap);
    }

    cupsRasterClose(ras);
    return 0;
}
