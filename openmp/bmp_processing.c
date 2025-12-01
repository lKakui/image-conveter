#include "bmp_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

// Função de comparação para qsort
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

    // Lê os cabeçalhos
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

    // Aloca memória para os pixels
    int width = image->info_header.width;
    int height = image->info_header.height;
    image->pixels = (Pixel**)malloc(height * sizeof(Pixel*));
    if (!image->pixels) {
        fclose(file);
        free(image);
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        image->pixels[i] = (Pixel*)malloc(width * sizeof(Pixel));
        if (!image->pixels[i]) {
            // Libera memória já alocada
            for (int j = 0; j < i; j++) free(image->pixels[j]);
            free(image->pixels);
            fclose(file);
            free(image);
            return NULL;
        }
    }

    // Lê os dados dos pixels
    fseek(file, image->file_header.offset, SEEK_SET);
    int padding = (4 - (width * sizeof(Pixel)) % 4) % 4;
    for (int i = 0; i < height; i++) {
        if (fread(image->pixels[i], sizeof(Pixel), width, file) != (size_t)width) {
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

    // Escreve os cabeçalhos
    fwrite(&image->file_header, sizeof(BMPFileHeader), 1, file);
    fwrite(&image->info_header, sizeof(BMPInfoHeader), 1, file);

    // Escreve os dados dos pixels
    int width = image->info_header.width;
    int height = image->info_header.height;
    int padding = (4 - (width * sizeof(Pixel)) % 4) % 4;
    uint8_t padding_data[3] = {0, 0, 0};

    for (int i = 0; i < height; i++) {
        fwrite(image->pixels[i], sizeof(Pixel), width, file);
        fwrite(padding_data, 1, padding, file);
    }

    fclose(file);
}

void free_image(Image* image) {
    if (!image) return;
    if (image->pixels) {
        for (int i = 0; i < image->info_header.height; i++) {
            free(image->pixels[i]);
        }
        free(image->pixels);
    }
    free(image);
}

void apply_median_filter(Image* image, int n) {
    int width = image->info_header.width;
    int height = image->info_header.height;
    int margin = n / 2;

    Pixel** new_pixels = (Pixel**)malloc(height * sizeof(Pixel*));
    for (int i = 0; i < height; i++) {
        new_pixels[i] = (Pixel*)malloc(width * sizeof(Pixel));
    }

    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        int window_size = n * n;
        uint8_t* window_r = (uint8_t*)malloc(window_size * sizeof(uint8_t));
        uint8_t* window_g = (uint8_t*)malloc(window_size * sizeof(uint8_t));
        uint8_t* window_b = (uint8_t*)malloc(window_size * sizeof(uint8_t));

        for (int x = 0; x < width; x++) {
            int count = 0;
            for (int j = -margin; j <= margin; j++) {
                for (int i = -margin; i <= margin; i++) {
                    int ny = y + j;
                    int nx = x + i;
                    if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                        window_r[count] = image->pixels[ny][nx].red;
                        window_g[count] = image->pixels[ny][nx].green;
                        window_b[count] = image->pixels[ny][nx].blue;
                        count++;
                    }
                }
            }

            qsort(window_r, count, sizeof(uint8_t), compare_bytes);
            qsort(window_g, count, sizeof(uint8_t), compare_bytes);
            qsort(window_b, count, sizeof(uint8_t), compare_bytes);

            int median_index = count / 2;
            new_pixels[y][x].red = window_r[median_index];
            new_pixels[y][x].green = window_g[median_index];
            new_pixels[y][x].blue = window_b[median_index];
        }
        free(window_r);
        free(window_g);
        free(window_b);
    }

    for (int i = 0; i < height; i++) {
        memcpy(image->pixels[i], new_pixels[i], width * sizeof(Pixel));
        free(new_pixels[i]);
    }
    free(new_pixels);
}

void convert_to_grayscale(Image* image) {
    int width = image->info_header.width;
    int height = image->info_header.height;

    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Pixel* p = &image->pixels[y][x];
            uint8_t gray = (uint8_t)(0.299 * p->red + 0.587 * p->green + 0.114 * p->blue);
            p->red = gray;
            p->green = gray;
            p->blue = gray;
        }
    }
}

void apply_histogram_equalization(Image* image) {
    int width = image->info_header.width;
    int height = image->info_header.height;
    long int total_pixels = width * height;

    // O histograma só precisa ser calculado para um canal, já que a imagem está em tons de cinza
    unsigned int histogram[256] = {0};
    #pragma omp parallel
    {
        unsigned int local_histogram[256] = {0};
        #pragma omp for nowait
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                local_histogram[image->pixels[y][x].blue]++;
            }
        }
        #pragma omp critical
        for (int i = 0; i < 256; i++) {
            histogram[i] += local_histogram[i];
        }
    }

    // Calcula o histograma cumulativo
    unsigned long cdf[256] = {0};
    cdf[0] = histogram[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i - 1] + histogram[i];
    }

    // Encontra o valor mínimo do cdf (não zero)
    unsigned long cdf_min = 0;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] != 0) {
            cdf_min = cdf[i];
            break;
        }
    }

    // Aplica a fórmula de equalização
    uint8_t new_gray_levels[256];
    for (int i = 0; i < 256; i++) {
        new_gray_levels[i] = (uint8_t)((((double)(cdf[i] - cdf_min)) / (total_pixels - cdf_min)) * 255.0);
    }

    // Atualiza os pixels da imagem
    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t old_gray = image->pixels[y][x].blue;
            uint8_t new_gray = new_gray_levels[old_gray];
            image->pixels[y][x].red = new_gray;
            image->pixels[y][x].green = new_gray;
            image->pixels[y][x].blue = new_gray;
        }
    }
}
