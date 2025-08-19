#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "png.h"

#define MODE_DECODE 0
#define MODE_ENCODE 1
#define MODE_GET_CAPACITY 2

void help(char *name)
{
    puts("PNG Image steganography tool.");
    printf("Usage: %s [OPTIONS] <input file>\n", name);
    puts(" -s Show capacity of image in bytes.");
    puts(" -o <file> Output file. If set will encode data.");
    puts(" -d <data> Data to encode. If not set will use stdin.");
}

int load_image(FILE *fp, png_bytep **row_pointers, unsigned int *width, unsigned int *height, int *bit_depth, int *colour_type, size_t *row_bytes, int no_load)
{
    unsigned char sig[8];
    png_structp png_ptr;
    png_infop info_ptr;

    fread(sig, 1, 8, fp);

    if (!png_check_sig(sig, 8)) {
        return 3;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        return 3;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        return 3;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 3;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, width, height, bit_depth, colour_type, NULL, NULL, NULL);

    *row_bytes = png_get_rowbytes(png_ptr, info_ptr);

    if (no_load == 1) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 0;
    }

    *row_pointers = malloc(sizeof(png_bytep) * *height);
    for (unsigned int i = 0; i < *height; i++) {
        (*(row_pointers))[i] = malloc(*row_bytes);
    }

    png_read_image(png_ptr, *row_pointers);
    png_read_end(png_ptr, NULL);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return 0;
}

int save_image(FILE *fp, png_bytep **row_pointers, unsigned int *width, unsigned int *height, int *bit_depth, int *colour_type, size_t *row_bytes)
{
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        return 3;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        return 3;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 3;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(
        png_ptr,
        info_ptr,
        *width,
        *height,
        *bit_depth,
        *colour_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, *row_pointers);
    png_write_end(png_ptr, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return 0;
}

void free_row_pointers(png_bytep **row_pointers, unsigned int height, size_t row_bytes)
{
    for (unsigned int i = 0; i < height; i++) {
        free((*(row_pointers))[i]);
    }

    free(*row_pointers);
}

void decode_data(png_bytep **row_pointers, unsigned int height, size_t row_bytes)
{
    char character = 0;
    unsigned int bit = 128;

    for (unsigned int y = 0; y < height; y++) {
        for (size_t x = 0; x < row_bytes; x++) {
            if ((*(row_pointers))[y][x] % 2 != 0) {
                character += bit;
            }

            if (bit == 1) {
                if (character == 0) {
                    return;
                }

                // Output character
                putchar(character);
                character = 0;
                bit = 128;
            } else {
                bit = bit/2;
            }
        }
    }
}

char get_data_byte(char *data)
{
    static int i = 0;

    return data[i++];
}

char get_stdin_byte()
{
    char byte;

    if (read(STDIN_FILENO, &byte, 1) == 0) {
        byte = '\0';
    }

    return byte;
}

int encode_data(png_bytep **row_pointers, unsigned int height, size_t row_bytes, char *data)
{
    char byte;
    unsigned int bit;
    unsigned int y = 0;
    unsigned int x = 0;

    while (1) {
        byte = data == NULL ? get_stdin_byte() : get_data_byte(data);

        bit = 128;

        while (bit != 0) {
            if (y == height) {
                return 4;
            }

            if ((*(row_pointers))[y][x] % 2 != ((byte & bit) == bit)) {
                if ((*(row_pointers))[y][x] > 127) {
                    (*(row_pointers))[y][x]--;
                } else {
                    (*(row_pointers))[y][x]++;
                }
            }

            if (bit == 1) {
                bit = 0;
            } else {
                bit = bit/2;
            }

            if (++x == row_bytes) {
                x = 0;
                ++y;
            }
        }

        if (byte == '\0') {
            return 0;
        }
    }
}

int main(int argc, char *argv[])
{
    int opt;
    int mode = MODE_DECODE;
    char *outfile;
    char *data = NULL;
    int ret;

    FILE *fp;
    FILE *out_fp;
    unsigned int width = 0;
    unsigned int height = 0;
    int bit_depth = 0;
    int colour_type = 0;
    png_bytep *row_pointers = NULL;
    size_t row_bytes = 0;

    while ((opt = getopt(argc, argv, "hso:d:")) != -1) {
        switch (opt) {
            case 'h':
                help(argv[0]);
                return 0;

            case 's':
                mode = MODE_GET_CAPACITY;
                break;

            case 'o':
                mode = MODE_ENCODE;
                outfile = optarg;
                break;

            case 'd':
                data = optarg;
                break;

            default:
                return 1;
        }
    }

    if (optind > (argc-1)) {
        fputs("Input file not specified!\n", stderr);
        return 1;
    }

    fp = fopen(argv[optind], "r");

    if (fp == NULL) {
        fputs("Cannot open input file!\n", stderr);
        return 2;
    }

    switch (mode) {
        case MODE_DECODE:
            if ((ret = load_image(fp, &row_pointers, &width, &height, &bit_depth, &colour_type, &row_bytes, 0)) != 0) {
                fputs("Cannot load PNG!\n", stderr);
                fclose(fp);
                free_row_pointers(&row_pointers, height, row_bytes);
                return ret;
            }

            decode_data(&row_pointers, height, row_bytes);
            break;

        case MODE_ENCODE:
            if ((ret = load_image(fp, &row_pointers, &width, &height, &bit_depth, &colour_type, &row_bytes, 0)) != 0) {
                fputs("Cannot load PNG!\n", stderr);
                fclose(fp);
                free_row_pointers(&row_pointers, height, row_bytes);
                return ret;
            }

            out_fp = fopen(outfile, "w+");
            if (out_fp == NULL) {
                fputs("Cannot open output file!\n", stderr);
                free_row_pointers(&row_pointers, height, row_bytes);
                return 2;
            }

            if ((ret = encode_data(&row_pointers, height, row_bytes, data)) != 0) {
                fputs("Out of space in image!\n", stderr);
                free_row_pointers(&row_pointers, height, row_bytes);
                return ret;
            }

            if ((ret = save_image(out_fp, &row_pointers, &width, &height, &bit_depth, &colour_type, &row_bytes)) != 0){
                fputs("Cannot save PNG!\n", stderr);
                fclose(fp);
                free_row_pointers(&row_pointers, height, row_bytes);
                return ret;
            }

            fclose(out_fp);

            break;

        case MODE_GET_CAPACITY:
            if ((ret = load_image(fp, &row_pointers, &width, &height, &bit_depth, &colour_type, &row_bytes, 1)) != 0) {
                fputs("Cannot load PNG!\n", stderr);
                free_row_pointers(&row_pointers, height, row_bytes);
                return ret;
            }

            printf("%i\n", (height*row_bytes)/8);

            break;
    }

    free_row_pointers(&row_pointers, height, row_bytes);

    fclose(fp);

    return 0;
}
