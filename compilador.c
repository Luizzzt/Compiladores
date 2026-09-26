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
#include <string.h>
#include <ctype.h>

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
int conta_linha = 1;   // linha atual do código fonte
char msg_erro_lexico[100]; // descrição do último erro léxico encontrado

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

/* ===================== ANALISADOR LÉXICO ===================== */

// Verifica se o lexema é palavra reservada (sem diferenciar maiúsculas e minúsculas).
// Retorna o átomo da palavra reservada ou IDENTIFICADOR.
TAtomo busca_palavra_reservada(const char *lexema) {
    char minusculo[16];
    int i;
    for (i = 0; lexema[i] != '\0'; i++)
        minusculo[i] = tolower((unsigned char)lexema[i]);
    minusculo[i] = '\0';

    // as palavras reservadas estão em sequência no enum, de ALGORITMO até VERDADEIRO
    for (int atomo = ALGORITMO; atomo <= VERDADEIRO; atomo++) {
        if (strcmp(minusculo, nome_atomo[atomo]) == 0)
            return (TAtomo)atomo;
    }
    return IDENTIFICADOR;
}

// identificador -> letra (letra | _ | digito)*   (no máximo 15 caracteres)
// Autômato:  q0 --letra--> q1 ;  q1 --letra, _ ou digito--> q1
void reconhece_identificador(TInfoAtomo *info_atomo) {
    char *ini_lexema = buffer;
    int tamanho;
    info_atomo->atomo = ERRO;

    if (isalpha((unsigned char)*buffer)) {
        buffer++;
        goto q1;
    }
    return;

q1:
    if (isalpha((unsigned char)*buffer) || isdigit((unsigned char)*buffer) || *buffer == '_') {
        buffer++;
        goto q1;
    }

    // estado final: confere o tamanho e recorta o lexema
    tamanho = buffer - ini_lexema;
    if (tamanho > 15) {
        sprintf(msg_erro_lexico, "identificador com mais de 15 caracteres");
        return;
    }
    strncpy(info_atomo->atributo.id, ini_lexema, tamanho);
    info_atomo->atributo.id[tamanho] = '\0';
    info_atomo->atomo = busca_palavra_reservada(info_atomo->atributo.id);
}

// constint -> digito+ ((E (+|vazio) digito+) | vazio)
// Exemplos: 1, 000, 124, 12E2 (=1200), 12E+2 (=1200)
// Autômato:  q0 --digito--> q1 ;  q1 --digito--> q1 ;  q1 --E--> q2 ;
//            q2 --+--> q3 ;  q2 --digito--> q4 ;  q3 --digito--> q4 ;  q4 --digito--> q4
// Estados finais: q1 e q4
void reconhece_constint(TInfoAtomo *info_atomo) {
    int valor = 0;
    int expoente = 0;
    int i;
    info_atomo->atomo = ERRO;

    if (isdigit((unsigned char)*buffer)) {
        valor = *buffer - '0';
        buffer++;
        goto q1;
    }
    return;

q1:
    if (isdigit((unsigned char)*buffer)) {
        valor = valor * 10 + (*buffer - '0');
        buffer++;
        goto q1;
    }
    if (*buffer == 'E') {
        buffer++;
        goto q2;
    }
    goto final;   // q1 é estado final

q2:
    if (*buffer == '+') {
        buffer++;
        goto q3;
    }
    if (isdigit((unsigned char)*buffer)) {
        expoente = *buffer - '0';
        buffer++;
        goto q4;
    }
    sprintf(msg_erro_lexico, "constante inteira mal formada");
    return;

q3:
    if (isdigit((unsigned char)*buffer)) {
        expoente = *buffer - '0';
        buffer++;
        goto q4;
    }
    sprintf(msg_erro_lexico, "constante inteira mal formada");
    return;

q4:
    if (isdigit((unsigned char)*buffer)) {
        expoente = expoente * 10 + (*buffer - '0');
        buffer++;
        goto q4;
    }
    // q4 é estado final: aplica o expoente (12E2 = 12 * 10 * 10)
    for (i = 0; i < expoente; i++)
        valor = valor * 10;

final:
    info_atomo->atomo = CONSTINT;
    info_atomo->atributo.numero = valor;
}

// Reconhece os símbolos da linguagem. Retorna 1 se reconheceu, 0 caso contrário
int reconhece_simbolo(TInfoAtomo *info_atomo) {
    switch (*buffer) {
        case '(': info_atomo->atomo = ABRE_PAR;      break;
        case ')': info_atomo->atomo = FECHA_PAR;     break;
        case ';': info_atomo->atomo = PONTO_VIRGULA; break;
        case ',': info_atomo->atomo = VIRGULA;       break;
        case '.': info_atomo->atomo = PONTO;         break;
        case '+': info_atomo->atomo = MAIS;          break;
        case '-': info_atomo->atomo = MENOS;         break;
        case '*': info_atomo->atomo = VEZES;         break;
        case '=': info_atomo->atomo = IGUAL;         break;
        case ':':
            if (*(buffer + 1) == '=') {       // :=
                info_atomo->atomo = ATRIBUICAO;
                buffer++;
            }
            else
                info_atomo->atomo = DOIS_PONTOS;
            break;
        case '<':
            if (*(buffer + 1) == '=') {       // <=
                info_atomo->atomo = MENOR_IGUAL;
                buffer++;
            }
            else if (*(buffer + 1) == '>') {  // <>
                info_atomo->atomo = DIFERENTE;
                buffer++;
            }
            else
                info_atomo->atomo = MENOR;
            break;
        case '>':
            if (*(buffer + 1) == '=') {       // >=
                info_atomo->atomo = MAIOR_IGUAL;
                buffer++;
            }
            else
                info_atomo->atomo = MAIOR;
            break;
        default:
            return 0;
    }
    buffer++;   // consome o último caractere do símbolo
    return 1;
}

// Retorna o próximo átomo do código fonte
TInfoAtomo obter_atomo(void) {
    TInfoAtomo info_atomo;
    info_atomo.atomo = ERRO;

    // ignora delimitadores e conta as linhas
    while (*buffer == ' ' || *buffer == '\n' || *buffer == '\t' || *buffer == '\r') {
        if (*buffer == '\n')
            conta_linha++;
        buffer++;
    }

    info_atomo.linha = conta_linha;

    if (*buffer == '\0') {
        info_atomo.atomo = EOS;
    }
    else if (isalpha((unsigned char)*buffer)) {
        reconhece_identificador(&info_atomo);
    }
    else if (isdigit((unsigned char)*buffer)) {
        reconhece_constint(&info_atomo);
    }
    else if (reconhece_simbolo(&info_atomo)) {
        // símbolo reconhecido, nada mais a fazer
    }
    else {
        sprintf(msg_erro_lexico, "caractere invalido [%c]", *buffer);
        buffer++;
    }

    return info_atomo;
}

// Imprime o Atomo no formato pedido: "# linha:atomo"
void imprimir_atomo(TInfoAtomo info_atomo) {
    printf("#%3d:%s", info_atomo.linha, nome_atomo[info_atomo.atomo]);
    if (info_atomo.atomo == IDENTIFICADOR)
        printf(": %s", info_atomo.atributo.id);
    else if (info_atomo.atomo == CONSTINT)
        printf(": %d", info_atomo.atributo.numero);
    else if (info_atomo.atomo == CONSTCHAR)
        printf(": %c", info_atomo.atributo.ch);
    printf("\n");
}

/* ===================== PRINCIPAL ===================== */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    ler_arquivo(argv[1]);

    // teste do léxico: imprimetodos os átomos até o fim do arquivo
    TInfoAtomo info_atomo = obter_atomo();
    while (info_atomo.atomo != EOS && info_atomo.atomo != ERRO) {
        imprimir_atomo(info_atomo);
        info_atomo = obter_atomo();
    }
    if (info_atomo.atomo == ERRO)
        printf("#%3d:erro lexico, %s\n", info_atomo.linha, msg_erro_lexico);

    liberar_buffer();
    return 0;
}