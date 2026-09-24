/*
 * Compilador Fase1: Análise Léxica e Sintática da linguagem Portugol
 *
 * Compilar:
 *   gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
 * Executar:
 *   ./compilador <arquivo_fonte>
 */
#include <stdio.h>
#include <stdlib.h>

/* ===================== VARIÁVEIS GLOBAIS DO LEXICO===================== */
char *buffer;          // posição atual de leitura no código fonte
char *inicio_buffer;   // início do buffer (usado para liberar a memória)

/* ===================== LEITURA DO ARQUIVO ===================== */

// Lê todo o arquivo fonte para a memória e aponta 'buffer' para o início
void ler_arquivo(const char *nome_arquivo) {
    FILE *arq = fopen(nome_arquivo, "r");
    if (arq == NULL) {
        printf("Erro: não foi possível abrir o arquivo %s\n", nome_arquivo);
        exit(1);
    }

    fseek(arq, 0, SEEK_END);
    long tamanho = ftell(arq);
    fseek(arq, 0, SEEK_SET);

    inicio_buffer = (char *)malloc(tamanho + 1);
    if (inicio_buffer == NULL) {
        printf("Erro: memória insuficiente\n");
        fclose(arq);
        exit(1);
    }

    size_t lidos = fread(inicio_buffer, 1, tamanho, arq);
    inicio_buffer[lidos] = '\0';   // marca o fim do buffer
    fclose(arq);

    buffer = inicio_buffer;
}

// Libera a memória do buffer
void liberar_buffer(void) {
    free(inicio_buffer);
    inicio_buffer = NULL;
    buffer = NULL;
}

/* ===================== PRINCIPAL ===================== */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    ler_arquivo(argv[1]);
    printf("%s", buffer);   // teste:mostra o conteúdo lido
    liberar_buffer();
    return 0;
}