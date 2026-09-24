/*
 * Compilador Fasecode .gitignore1: Análise Léxica e Sintática da linguagem Portugol
 *
 * Compilar:
 *   gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
 * Executar:
 *   ./compilador <arquivo_fonte>
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    printf("Arquivo informado: %s\n", argv[1]);
    return 0;
}