/*
 * Compilador Fase 1: Análise Léxica e Sintática da linguagem Portugol
 *
 * Compilar:
 *   gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
 * Executar:
 *   ./compilador <arquivo_fonte>
 */
#include <stdio.h>
#include <stdlib.h>

/* ===================== ÁTOMOS DA LINGUAGEM ===================== */
typedef enum {
    ERRO,           // erro léxico
    EOS,            // fim do arquivo (End Of String)
    COMENTARIO,
    IDENTIFICADOR,
    CONSTINT,       // constante inteira
    CONSTCHAR,      // constante caractere

    // palavras reservadas
    ALGORITMO, CARACTERE, DIV, E, ENQUANTO, ENTAO, ESCREVA, FACA, FALSO,
    FIM, FUNCAO, INICIO, INTEIRO, LEIA, LOGICO, MOD, NAO, OU,
    PROCEDIMENTO, SE, SENAO, VAR, VERDADEIRO,

    // símbolos
    ABRE_PAR,       // (
    FECHA_PAR,      // )
    PONTO_VIRGULA,  // ;
    VIRGULA,        // ,
    PONTO,          // .
    DOIS_PONTOS,    // :
    ATRIBUICAO,     // :=
    MAIS,           // +
    MENOS,          // -
    VEZES,          // *
    MENOR,          // <
    MENOR_IGUAL,    // <=
    MAIOR,          // >
    MAIOR_IGUAL,    // >=
    IGUAL,          // =
    DIFERENTE       // <>
} TAtomo;

// Nome de cada átomo, usado na saída do compilador (mesma ordem do enum)
char *nome_atomo[] = {
    "erro", "fim_arquivo", "comentario", "identificador", "constint", "constchar",
    "algoritmo", "caractere", "div", "e", "enquanto", "entao", "escreva", "faca", "falso",
    "fim", "funcao", "inicio", "inteiro", "leia", "logico", "mod", "nao", "ou",
    "procedimento", "se", "senao", "var", "verdadeiro",
    "abre_par", "fecha_par", "ponto_virgula", "virgula", "ponto", "dois_pontos",
    "atribuicao", "mais", "menos", "vezes", "menor", "menor_igual", "maior",
    "maior_igual", "igual", "diferente"
};

// Símbolo de cada átomo, usado nas mensagens de erro sintático (mesma ordem do enum)
char *simbolo_atomo[] = {
    "erro lexico", "fim de arquivo", "comentario", "identificador", "constint", "constchar",
    "algoritmo", "caractere", "div", "e", "enquanto", "entao", "escreva", "faca", "falso",
    "fim", "funcao", "inicio", "inteiro", "leia", "logico", "mod", "nao", "ou",
    "procedimento", "se", "senao", "var", "verdadeiro",
    "(", ")", ";", ",", ".", ":",
    ":=", "+", "-", "*", "<", "<=", ">",
    ">=", "=", "<>"
};

// Estrutura de comunicação entre o léxico e o sintático
typedef struct {
    TAtomo atomo;
    int linha;
    union {
        int numero;    // atributo do átomo constint
        char id[16];   // atributo do identificador
        char ch;       // atributo do átomo constchar
    } atributo;
} TInfoAtomo;

/* ===================== VARIÁVEIS GLOBAIS DO LÉXICO ===================== */
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
    printf("%s", buffer);   // teste: mostra o conteúdo lido
    liberar_buffer();
    return 0;
}