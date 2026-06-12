#include <cups/raster.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LABEL_W_MM 50
#define LABEL_H_MM 30
#define GAP_MM 5
#define DENSITY 8
#define SPEED 4

#define OUT_W_DOTS 400
#define OUT_H_DOTS 240
#define OUT_W_BYTES 50

#define INVERT_BITS 1
#define FLIP_Y 0
#define FLIP_X 0

static int get_1bit_pixel(const unsigned char *row, unsigned x) {
    unsigned byte_index = x / 8;
    unsigned bit_index = 7 - (x % 8);
    return (row[byte_index] >> bit_index) & 1;
}

static void set_bit(unsigned char bitmap[OUT_H_DOTS][OUT_W_BYTES], unsigned x, unsigned y, int black) {
    if (x >= OUT_W_DOTS || y >= OUT_H_DOTS) return;

    if (black) {
        bitmap[y][x / 8] |= (0x80 >> (x % 8));
    }
}

int main(int argc, char *argv[]) {
    cups_raster_t *ras;
    cups_page_header2_t header;

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

        fprintf(stderr,
            "DEBUG: PM220 raster page: cupsWidth=%u cupsHeight=%u bpl=%u bpp=%u colors=%u xdpi=%u ydpi=%u\n",
            header.cupsWidth,
            header.cupsHeight,
            header.cupsBytesPerLine,
            header.cupsBitsPerPixel,
            header.cupsNumColors,
            header.HWResolution[0],
            header.HWResolution[1]
        );

        if (in_w == 0 || in_h == 0 || bpl == 0) {
            fprintf(stderr, "ERROR: PM220 bad raster header\n");
            cupsRasterClose(ras);
            return 1;
        }

        unsigned char bitmap[OUT_H_DOTS][OUT_W_BYTES];
        memset(bitmap, 0x00, sizeof(bitmap));

        unsigned char *row = calloc(1, bpl);
        if (!row) {
            fprintf(stderr, "ERROR: PM220 out of memory\n");
            cupsRasterClose(ras);
            return 1;
        }

        for (unsigned y = 0; y < in_h; y++) {
            if (cupsRasterReadPixels(ras, row, bpl) < bpl) {
                fprintf(stderr, "ERROR: PM220 short raster read\n");
                free(row);
                cupsRasterClose(ras);
                return 1;
            }

            if (y >= OUT_H_DOTS) continue;

            unsigned out_y = FLIP_Y ? (OUT_H_DOTS - 1 - y) : y;

            if (bpp == 1 && bpl == OUT_W_BYTES && in_w == OUT_W_DOTS && in_h == OUT_H_DOTS && !FLIP_X) {
                for (unsigned i = 0; i < OUT_W_BYTES; i++) {
                    unsigned char v = row[i];

                    if (INVERT_BITS) {
                        v ^= 0xFF;
                    }

                    bitmap[out_y][i] = v;
                }

                continue;
            }

            if (bpp == 1) {
                for (unsigned x = 0; x < in_w && x < OUT_W_DOTS; x++) {
                    unsigned out_x = FLIP_X ? (OUT_W_DOTS - 1 - x) : x;
                    int black = get_1bit_pixel(row, x);

                    if (INVERT_BITS) {
                        black = !black;
                    }

                    set_bit(bitmap, out_x, out_y, black);
                }
            } else {
                fprintf(stderr, "ERROR: PM220 unsupported raster bpp=%u\n", bpp);
                free(row);
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
        printf("SIZE %d mm,%d mm\n", LABEL_W_MM, LABEL_H_MM);
        printf("GAP %d mm,0\n", GAP_MM);
        printf("DENSITY %d\n", DENSITY);
        printf("SPEED %d\n", SPEED);
        printf("DIRECTION 0\n");
        printf("REFERENCE 0,0\n");
        printf("CLS\n");
        printf("BITMAP 0,0,%d,%d,0,", OUT_W_BYTES, OUT_H_DOTS);
        fwrite(bitmap, 1, OUT_W_BYTES * OUT_H_DOTS, stdout);
        printf("\nPRINT 1,1\n\n");
        fflush(stdout);
    }

    cupsRasterClose(ras);
    return 0;
}
