#ifndef BMP_PROCESSING_H
#define BMP_PROCESSING_H

#include <stdint.h>

// Estrutura para o cabeçalho do arquivo BMP
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPFileHeader;

// Estrutura para o cabeçalho de informações do BMP
typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t x_pels_per_meter;
    int32_t y_pels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;
} BMPInfoHeader;
#pragma pack(pop)

// Estrutura para representar um pixel de 24 bits (BGR)
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} Pixel;

// Estrutura para a imagem
typedef struct {
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    Pixel* data;
} Image;

// Protótipos das Funções

// Carrega uma imagem BMP de um arquivo
Image* load_bmp(const char* filename);

// Salva uma imagem BMP em um arquivo
void save_bmp(const char* filename, Image* image);

// Libera a memória alocada para a imagem
void free_image(Image* image);

// Aplica o filtro de mediana N_N
void apply_median_filter(Image* image, int n);

// Converte a imagem para tons de cinza
void convert_to_grayscale(Image* image);

// Aplica a equalização de histograma
void apply_histogram_equalization(Image* image);

#endif // BMP_PROCESSING_H
