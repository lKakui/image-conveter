#include <stdio.h>
#include <stdlib.h>
#include "bmp_processing.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <arquivo_entrada.bmp> <arquivo_saida.bmp> <tamanho_filtro_mediana>\n", argv[0]);
        return 1;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];
    int n = atoi(argv[3]);

    if (n <= 0 || n % 2 == 0) {
        fprintf(stderr, "O tamanho do filtro de mediana (N) deve ser um inteiro ímpar positivo.\n");
        return 1;
    }

    // Carregar a imagem
    Image* image = load_bmp(input_filename);
    if (!image) {
        return 1;
    }

    printf("Imagem '%s' carregada com sucesso.\n", input_filename);
    printf("Dimensões: %d x %d\n", image->info_header.width, image->info_header.height);

    // Aplicar filtro de mediana
    printf("Aplicando filtro de mediana %dx%d...\n", n, n);
    apply_median_filter(image, n);

    // Converter para tons de cinza
    printf("Convertendo para tons de cinza...\n");
    convert_to_grayscale(image);

    // Aplicar equalização de histograma
    printf("Aplicando equalização de histograma...\n");
    apply_histogram_equalization(image);

    // Salvar a imagem resultante
    save_bmp(output_filename, image);
    printf("Imagem processada salva em '%s'.\n", output_filename);

    // Liberar memória
    free_image(image);

    return 0;
}
