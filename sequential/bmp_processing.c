#include "bmp_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int compare_bytes(const void* a, const void* b) {
    return (*(uint8_t*)a - *(uint8_t*)b);
}

Image* load_bmp(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir o arquivo de entrada");
        return NULL;
    }

    Image* image = (Image*)malloc(sizeof(Image));
    if (!image) {
        fclose(file);
        return NULL;
    }

    if (fread(&image->file_header, sizeof(BMPFileHeader), 1, file) != 1 ||
        fread(&image->info_header, sizeof(BMPInfoHeader), 1, file) != 1) {
        fprintf(stderr, "Erro ao ler os cabeçalhos BMP.\n");
        fclose(file);
        free(image);
        return NULL;
    }

    if (image->file_header.type != 0x4D42) {
        fprintf(stderr, "O arquivo não é um BMP válido.\n");
        fclose(file);
        free(image);
        return NULL;
    }

    if (image->info_header.bit_count != 24) {
        fprintf(stderr, "A imagem não é de 24 bits.\n");
        fclose(file);
        free(image);
        return NULL;
    }

    int width = image->info_header.width;
    int height = image->info_header.height;
    image->data = (Pixel*)malloc(width * height * sizeof(Pixel));
    if (!image->data) {
        fclose(file);
        free(image);
        return NULL;
    }

    fseek(file, image->file_header.offset, SEEK_SET);
    int padding = (4 - (width * sizeof(Pixel)) % 4) % 4;
    for (int i = 0; i < height; i++) {
        if (fread(image->data + i * width, sizeof(Pixel), width, file) != (size_t)width) {
            fprintf(stderr, "Erro ao ler os dados dos pixels.\n");
            free_image(image);
            fclose(file);
            return NULL;
        }
        fseek(file, padding, SEEK_CUR);
    }

    fclose(file);
    return image;
}

void save_bmp(const char* filename, Image* image) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Erro ao criar o arquivo de saída");
        return;
    }

    fwrite(&image->file_header, sizeof(BMPFileHeader), 1, file);
    fwrite(&image->info_header, sizeof(BMPInfoHeader), 1, file);

    int width = image->info_header.width;
    int height = image->info_header.height;
    int padding = (4 - (width * sizeof(Pixel)) % 4) % 4;
    uint8_t padding_data[3] = {0, 0, 0};

    for (int i = 0; i < height; i++) {
        fwrite(image->data + i * width, sizeof(Pixel), width, file);
        fwrite(padding_data, 1, padding, file);
    }

    fclose(file);
}

void free_image(Image* image) {
    if (!image) return;
    if (image->data) {
        free(image->data);
    }
    free(image);
}

void apply_median_filter(Image* image, int n) {
    int width = image->info_header.width;
    int height = image->info_header.height;
    int margin = n / 2;
    int window_size = n * n;

    Pixel* new_data = (Pixel*)malloc(width * height * sizeof(Pixel));

    uint8_t* window_r = (uint8_t*)malloc(window_size * sizeof(uint8_t));
    uint8_t* window_g = (uint8_t*)malloc(window_size * sizeof(uint8_t));
    uint8_t* window_b = (uint8_t*)malloc(window_size * sizeof(uint8_t));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int count = 0;
            for (int j = -margin; j <= margin; j++) {
                for (int i = -margin; i <= margin; i++) {
                    int ny = y + j;
                    int nx = x + i;
                    if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                        window_r[count] = image->data[ny * width + nx].red;
                        window_g[count] = image->data[ny * width + nx].green;
                        window_b[count] = image->data[ny * width + nx].blue;
                        count++;
                    }
                }
            }

            qsort(window_r, count, sizeof(uint8_t), compare_bytes);
            qsort(window_g, count, sizeof(uint8_t), compare_bytes);
            qsort(window_b, count, sizeof(uint8_t), compare_bytes);

            int median_index = count / 2;
            new_data[y * width + x].red = window_r[median_index];
            new_data[y * width + x].green = window_g[median_index];
            new_data[y * width + x].blue = window_b[median_index];
        }
    }

    memcpy(image->data, new_data, width * height * sizeof(Pixel));
    free(new_data);
    free(window_r);
    free(window_g);
    free(window_b);
}

void convert_to_grayscale(Image* image) {
    int width = image->info_header.width;
    int height = image->info_header.height;

    for (int i = 0; i < height * width; i++) {
        Pixel* p = &image->data[i];
        uint8_t gray = (uint8_t)(0.299 * p->red + 0.587 * p->green + 0.114 * p->blue);
        p->red = gray;
        p->green = gray;
        p->blue = gray;
    }
}

void apply_histogram_equalization(Image* image) {
    int width = image->info_header.width;
    int height = image->info_header.height;
    long int total_pixels = width * height;

    unsigned int histogram[256] = {0};
    for (int i = 0; i < height * width; i++) {
        histogram[image->data[i].blue]++;
    }

    unsigned long cdf[256] = {0};
    cdf[0] = histogram[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i - 1] + histogram[i];
    }

    unsigned long cdf_min = 0;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] != 0) {
            cdf_min = cdf[i];
            break;
        }
    }

    uint8_t new_gray_levels[256];
    for (int i = 0; i < 256; i++) {
        new_gray_levels[i] = (uint8_t)((((double)(cdf[i] - cdf_min)) / (total_pixels - cdf_min)) * 255.0);
    }

    for (int i = 0; i < height * width; i++) {
        uint8_t old_gray = image->data[i].blue;
        uint8_t new_gray = new_gray_levels[old_gray];
        image->data[i].red = new_gray;
        image->data[i].green = new_gray;
        image->data[i].blue = new_gray;
    }
}
