#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "bmp_processing.h"

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc != 4) {
        if (world_rank == 0) {
            fprintf(stderr, "Uso: %s <arquivo_entrada.bmp> <arquivo_saida.bmp> <tamanho_filtro_mediana>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];
    int n = atoi(argv[3]);

    if (n <= 0 || n % 2 == 0) {
        if (world_rank == 0) {
            fprintf(stderr, "O tamanho do filtro de mediana (N) deve ser um inteiro ímpar positivo.\n");
        }
        MPI_Finalize();
        return 1;
    }

    Image* image = NULL;
    if (world_rank == 0) {
        // Carregar a imagem
        image = load_bmp(input_filename);
        if (!image) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        printf("Imagem '%s' carregada com sucesso.\n", input_filename);
        printf("Dimensões: %d x %d\n", image->info_header.width, image->info_header.height);
    }

    // Broadcast das informações do cabeçalho da imagem
    BMPInfoHeader info_header;
    if (world_rank == 0) {
        info_header = image->info_header;
    }
    MPI_Bcast(&info_header, sizeof(BMPInfoHeader), MPI_BYTE, 0, MPI_COMM_WORLD);

    int width = info_header.width;
    int height = info_header.height;
    int rows_per_proc = height / world_size;
    int extra_rows = height % world_size;

    int start_row = world_rank * rows_per_proc + (world_rank < extra_rows ? world_rank : extra_rows);
    int num_rows = rows_per_proc + (world_rank < extra_rows ? 1 : 0);

    // Adicionar sobreposição para o filtro de mediana
    int overlap = n / 2;
    int send_start_row = start_row - overlap;
    int send_num_rows = num_rows + 2 * overlap;

    if (start_row == 0) {
        send_start_row = 0;
        send_num_rows = num_rows + overlap;
    }
    if (start_row + num_rows == height) {
        send_num_rows = num_rows + overlap;
    }


    Pixel* local_data = (Pixel*)malloc(width * send_num_rows * sizeof(Pixel));

    if (world_rank == 0) {
        for (int i = 1; i < world_size; i++) {
            int dest_start_row = i * rows_per_proc + (i < extra_rows ? i : extra_rows);
            int dest_num_rows = rows_per_proc + (i < extra_rows ? 1 : 0);
            int dest_send_start = dest_start_row - overlap;
            int dest_send_rows = dest_num_rows + 2 * overlap;

            if (dest_start_row == 0) {
                dest_send_start = 0;
                dest_send_rows = dest_num_rows + overlap;
            }
            if (dest_start_row + dest_num_rows == height) {
                dest_send_rows = dest_num_rows + overlap;
            }
            MPI_Send(image->data + dest_send_start * width, dest_send_rows * width * sizeof(Pixel), MPI_BYTE, i, 0, MPI_COMM_WORLD);
        }
        // Copiar dados para o processo 0
        for(int i = 0; i < send_num_rows * width; i++){
            local_data[i] = image->data[send_start_row * width + i];
        }
    } else {
        MPI_Recv(local_data, width * send_num_rows * sizeof(Pixel), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    Image local_image;
    local_image.info_header = info_header;
    local_image.info_header.height = send_num_rows;
    local_image.data = local_data;


    // Aplicar filtro de mediana
    if(world_rank == 0) printf("Aplicando filtro de mediana %dx%d...\n", n, n);
    apply_median_filter(&local_image, n);

    // Converter para tons de cinza
    if(world_rank == 0) printf("Convertendo para tons de cinza...\n");
    convert_to_grayscale(&local_image);

    Pixel* processed_data = (Pixel*)malloc(width * num_rows * sizeof(Pixel));
    int copy_start_offset = (start_row == 0) ? 0 : overlap;
    for(int i = 0; i < num_rows; i++){
        for(int j = 0; j < width; j++){
            processed_data[i*width + j] = local_data[(copy_start_offset + i)*width + j];
        }
    }

    if (world_rank == 0) {
        // Coletar os dados processados
        for(int i = 0; i < num_rows * width; i++){
            image->data[start_row * width + i] = processed_data[i];
        }
        for (int i = 1; i < world_size; i++) {
            int src_start_row = i * rows_per_proc + (i < extra_rows ? i : extra_rows);
            int src_num_rows = rows_per_proc + (i < extra_rows ? 1 : 0);
            MPI_Recv(image->data + src_start_row * width, src_num_rows * width * sizeof(Pixel), MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Send(processed_data, num_rows * width * sizeof(Pixel), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }


    if (world_rank == 0) {
        // Aplicar equalização de histograma na imagem completa
        printf("Aplicando equalização de histograma...\n");
        apply_histogram_equalization(image);

        // Salvar a imagem resultante
        save_bmp(output_filename, image);
        printf("Imagem processada salva em '%s'.\n", output_filename);

        // Liberar memória
        free_image(image);
    }

    free(local_data);
    free(processed_data);

    MPI_Finalize();
    return 0;
}
