# Processador de Imagens: Filtro de Mediana, Tons de Cinza e Equalização de Histograma

O objetivo deste projeto é desenvolver um programa que processa uma imagem no formato BMP 24 bits, aplicando um fluxo de tratamento para melhorar sua qualidade, especialmente em imagens com ruído.

## Etapas do Processamento de Imagem

O fluxo de processamento é composto por três etapas principais:

### 1. Suavização com Filtro Mediana

Para reduzir ruídos, como o "sal e pimenta", aplica-se um filtro de suavização.

> O **filtro mediana** é uma técnica que substitui o valor de cada pixel pela **mediana** dos valores em sua vizinhança, definida por uma máscara (ex: 3×3, 5×5). Para cada pixel, os valores da vizinhança são ordenados e o valor central (mediana) torna-se o novo valor do pixel. Isso atenua ruídos preservando as bordas da imagem.

### 2. Conversão para Tons de Cinza

A imagem suavizada é convertida para tons de cinza, pois a equalização de histograma opera em imagens de um único canal de intensidade.

> A conversão é feita combinando os canais de cor (Vermelho, Verde e Azul) com uma média ponderada, que leva em conta a percepção humana da luminosidade:
> **Intensidade = 0,299 * R + 0,587 * G + 0,114 * B**

### 3. Equalização de Histograma

Por fim, realiza-se a equalização do histograma para aumentar o contraste da imagem e tornar os detalhes mais visíveis.

> O processo envolve:
> 1.  **Cálculo do histograma:** Contagem da frequência de cada nível de intensidade (0 a 255).
> 2.  **Cálculo da Função de Distribuição Acumulada (CDF):** Soma cumulativa das frequências do histograma.
> 3.  **Normalização da CDF:** Ajuste dos valores da CDF para a escala de 0 a 255, criando um mapa de transformação.
> 4.  **Aplicação do mapa:** Substituição de cada nível de intensidade original pelo novo valor correspondente no mapa.

## Requisitos de Implementação

O programa deve ser desenvolvido para executar o fluxo de processamento de forma sequencial e paralela, gravando a imagem final em um arquivo BMP.

### Implementação com MPI (Message Passing Interface)

Desenvolva uma versão paralela utilizando **MPI** que paralelize as três etapas do processamento.

**Parâmetros de entrada:**
*   Tamanho da máscara do filtro mediana (N para N×N).
*   Número de processos a serem utilizados.

**Avaliação de Desempenho:**
*   Medir o tempo total de processamento com **1, 2, 3 e 4 processos**, para máscaras **3×3, 5×5 e 7×7**.
*   Calcular o **Speedup** e a **Eficiência** em relação à versão sequencial.

### Reimplementação com OpenMP

Reescreva a solução paralela utilizando **OpenMP**.

**Parâmetros de entrada:**
*   Tamanho da máscara.
*   Número de threads.

**Avaliação de Desempenho:**
*   Testar o programa com **1, 2, 3 e 4 threads**, para máscaras **3×3, 5×5 e 7×7**.
*   Calcular o **Speedup** e a **Eficiência** da versão paralela.
*   Comparar o comportamento e os resultados entre as abordagens **MPI** e **OpenMP**.