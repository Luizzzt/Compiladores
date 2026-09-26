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

/* ===================== VARIÁVEIS GLOBAIS DO SINTÁTICO ===================== */
TAtomo lookahead;       // átomo atual
TInfoAtomo info_atomo;  // informações do átomo atual

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

// Conta quantas linhas o arquivo fonte possui
int contar_linhas(void) {
    int linhas = 0;
    char *p;
    for (p = inicio_buffer; *p != '\0'; p++) {
        if (*p == '\n')
            linhas++;
    }
    // a última linha pode não terminar com '\n'
    if (p != inicio_buffer && *(p - 1) != '\n')
        linhas++;
    return linhas;
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

// constchar -> 'caractere ASCII'   Exemplos: 'a', '0'
// Autômato:  q0 --'--> q1 ;  q1 --caractere ASCII--> q2 ;  q2 --'--> q3 (final)
void reconhece_constchar(TInfoAtomo *info_atomo) {
    info_atomo->atomo = ERRO;

    if (*buffer == '\'') {
        buffer++;
        goto q1;
    }
    return;

q1:
    // aceita qualquer caractere ASCII (1 a 127), menos o fim do arquivo
    if (*buffer != '\0' && (unsigned char)*buffer <= 127) {
        info_atomo->atributo.ch = *buffer;
        buffer++;
        goto q2;
    }
    sprintf(msg_erro_lexico, "constante caractere mal formada");
    return;

q2:
    if (*buffer == '\'') {
        buffer++;
        goto q3;
    }
    sprintf(msg_erro_lexico, "constante caractere mal formada");
    return;

q3:
    info_atomo->atomo = CONSTCHAR;
}

// Comentário de várias linhas: começa com {- e termina com -}
// A contagem de linhas continua dentro do comentário.
// Autômato:  q0 --{--> q1 ;  q1 --  -  --> q2 ;
//            q2 --  -  --> q3 ;  q2 --outro--> q2 ;
//            q3 --  }  --> q4 (final) ;  q3 --  -  --> q3 ;  q3 --outro--> q2
void reconhece_comentario(TInfoAtomo *info_atomo) {
    info_atomo->atomo = ERRO;

    if (*buffer == '{') {
        buffer++;
        goto q1;
    }
    return;

q1:
    if (*buffer == '-') {
        buffer++;
        goto q2;
    }
    return;

q2:
    if (*buffer == '\0') {
        sprintf(msg_erro_lexico, "comentario nao foi fechado");
        return;
    }
    if (*buffer == '-') {
        buffer++;
        goto q3;
    }
    if (*buffer == '\n')
        conta_linha++;
    buffer++;
    goto q2;

q3:
    if (*buffer == '\0') {
        sprintf(msg_erro_lexico, "comentario nao foi fechado");
        return;
    }
    if (*buffer == '}') {
        buffer++;
        goto q4;
    }
    if (*buffer == '-') {
        buffer++;
        goto q3;
    }
    if (*buffer == '\n')
        conta_linha++;
    buffer++;
    goto q2;

q4:
    info_atomo->atomo = COMENTARIO;
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
    else if (*buffer == '\'') {
        reconhece_constchar(&info_atomo);
    }
    else if (*buffer == '{' && *(buffer + 1) == '-') {
        reconhece_comentario(&info_atomo);
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

// Imprime o átomo no formato pedido: "# linha:atomo"
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

/* ===================== ANALISADOR SINTÁTICO ===================== */

// protótipos das funções da gramática
void programa(void);
void bloco(void);
void declaracao_variaveis(void);
void lista_variaveis(void);
void declaracao_de_rotinas(void);
void declaracao_de_funcao(void);
void declaracao_de_procedimento(void);
void parametros_formais(void);
void parametro_formal(void);
void tipo(void);
void comando_composto(void);
void comando(void);
void comando_atribuicao_ou_chamada(void);
void comando_entrada(void);
void comando_saida(void);
void comando_condicional(void);
void comando_repeticao(void);
void lista_expressao(void);
void expressao(void);
void expressao_simples(void);
void termo(void);
void fator(void);

// Libera a memória e termina o programa
void encerrar(int codigo) {
    liberar_buffer();
    exit(codigo);
}

// Mostra o erro sintático e termina a execução
void erro_sintatico(const char *esperado) {
    printf("#%3d:erro sintatico, esperado [%s] encontrado [%s]\n",
           info_atomo.linha, esperado, simbolo_atomo[lookahead]);
    encerrar(1);
}

// Pede o próximo átomo ao léxico.
// Comentários são impressos e descartados; erro léxico termina a execução.
void proximo_atomo(void) {
    info_atomo = obter_atomo();
    while (info_atomo.atomo == COMENTARIO) {
        imprimir_atomo(info_atomo);
        info_atomo = obter_atomo();
    }
    if (info_atomo.atomo == ERRO) {
        printf("#%3d:erro lexico, %s\n", info_atomo.linha, msg_erro_lexico);
        encerrar(1);
    }
    lookahead = info_atomo.atomo;
}

// Confere se o átomo atual é o esperado, imprime e avança
void consome(TAtomo atomo) {
    if (lookahead == atomo) {
        if (atomo != EOS)   // o fim de arquivo não é impresso
            imprimir_atomo(info_atomo);
        proximo_atomo();
    }
    else {
        erro_sintatico(simbolo_atomo[atomo]);
    }
}

// <programa> ::= algoritmo identificador ';' <bloco> '.'
void programa(void) {
    consome(ALGORITMO);
    consome(IDENTIFICADOR);
    consome(PONTO_VIRGULA);
    bloco();
    consome(PONTO);
}

// <bloco> ::= <declaracao_variaveis> <declaracao_de_rotinas> <comando_composto>
void bloco(void) {
    declaracao_variaveis();
    declaracao_de_rotinas();
    comando_composto();
}

// <declaracao_variaveis> ::= [ var <lista_variaveis> ';' { <lista_variaveis> ';' } ]
void declaracao_variaveis(void) {
    if (lookahead == VAR) {
        consome(VAR);
        lista_variaveis();
        consome(PONTO_VIRGULA);
        while (lookahead == IDENTIFICADOR) {
            lista_variaveis();
            consome(PONTO_VIRGULA);
        }
    }
}

// <lista_variaveis> ::= identificador { ',' identificador } ':' <tipo>
void lista_variaveis(void) {
    consome(IDENTIFICADOR);
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        consome(IDENTIFICADOR);
    }
    consome(DOIS_PONTOS);
    tipo();
}

// <declaracao_de_rotinas> ::= { <declaracao_de_funcao> | <declaracao_de_procedimento> }
void declaracao_de_rotinas(void) {
    while (lookahead == FUNCAO || lookahead == PROCEDIMENTO) {
        if (lookahead == FUNCAO)
            declaracao_de_funcao();
        else
            declaracao_de_procedimento();
    }
}

// <declaracao_de_funcao> ::= funcao <tipo> identificador <parametros_formais>
//                            <declaracao_variaveis> <comando_composto>
void declaracao_de_funcao(void) {
    consome(FUNCAO);
    tipo();
    consome(IDENTIFICADOR);
    parametros_formais();
    declaracao_variaveis();
    comando_composto();
}

// <declaracao_de_procedimento> ::= procedimento identificador <parametros_formais>
//                                  <declaracao_variaveis> <comando_composto>
void declaracao_de_procedimento(void) {
    consome(PROCEDIMENTO);
    consome(IDENTIFICADOR);
    parametros_formais();
    declaracao_variaveis();
    comando_composto();
}

// <parametros_formais> ::= '(' <parametro_formal> { ';' <parametro_formal> } ')' | '(' ')'
void parametros_formais(void) {
    consome(ABRE_PAR);
    if (lookahead != FECHA_PAR) {
        parametro_formal();
        while (lookahead == PONTO_VIRGULA) {
            consome(PONTO_VIRGULA);
            parametro_formal();
        }
    }
    consome(FECHA_PAR);
}

// <parametro_formal> ::= [ var ] <lista_variaveis>
void parametro_formal(void) {
    if (lookahead == VAR)
        consome(VAR);
    lista_variaveis();
}

// <tipo> ::= caractere | inteiro | logico
void tipo(void) {
    if (lookahead == CARACTERE)
        consome(CARACTERE);
    else if (lookahead == INTEIRO)
        consome(INTEIRO);
    else if (lookahead == LOGICO)
        consome(LOGICO);
    else
        erro_sintatico("tipo");
}

// <comando_composto> ::= inicio <comando> { ';' <comando> } fim
void comando_composto(void) {
    consome(INICIO);
    comando();
    while (lookahead == PONTO_VIRGULA) {
        consome(PONTO_VIRGULA);
        comando();
    }
    consome(FIM);
}

// <comando> ::= <comando_atribuicao> | <comando_entrada> | <comando_saida> |
//               <comando_condicional> | <comando_repeticao> |
//               <chamada_procedimento> | <comando_composto>
void comando(void) {
    switch (lookahead) {
        case IDENTIFICADOR: comando_atribuicao_ou_chamada(); break;
        case LEIA:          comando_entrada();               break;
        case ESCREVA:       comando_saida();                 break;
        case SE:            comando_condicional();           break;
        case ENQUANTO:      comando_repeticao();             break;
        case INICIO:        comando_composto();              break;
        default:            erro_sintatico("comando");
    }
}

// <comando_atribuicao>   ::= identificador ':=' <expressao>
// <chamada_procedimento> ::= identificador [ '(' <lista_expressao> ')' ]
// Os dois começam com identificador, então o próximo átomo decide qual é.
void comando_atribuicao_ou_chamada(void) {
    consome(IDENTIFICADOR);
    if (lookahead == ATRIBUICAO) {
        consome(ATRIBUICAO);
        expressao();
    }
    else if (lookahead == ABRE_PAR) {
        consome(ABRE_PAR);
        lista_expressao();
        consome(FECHA_PAR);
    }
}

// <comando_entrada> ::= leia '(' identificador { ',' identificador } ')'
void comando_entrada(void) {
    consome(LEIA);
    consome(ABRE_PAR);
    consome(IDENTIFICADOR);
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        consome(IDENTIFICADOR);
    }
    consome(FECHA_PAR);
}

// <comando_saida> ::= escreva '(' <lista_expressao> ')'
void comando_saida(void) {
    consome(ESCREVA);
    consome(ABRE_PAR);
    lista_expressao();
    consome(FECHA_PAR);
}

// <comando_condicional> ::= se <expressao> entao <comando> [ senao <comando> ]
void comando_condicional(void) {
    consome(SE);
    expressao();
    consome(ENTAO);
    comando();
    if (lookahead == SENAO) {
        consome(SENAO);
        comando();
    }
}

// <comando_repeticao> ::= enquanto <expressao> faca <comando>
void comando_repeticao(void) {
    consome(ENQUANTO);
    expressao();
    consome(FACA);
    comando();
}

// <lista_expressao> ::= <expressao> { ',' <expressao> }
void lista_expressao(void) {
    expressao();
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        expressao();
    }
}

// <expressao> ::= <expressao_simples> [ <operador_relacional> <expressao_simples> ]
// <operador_relacional> ::= '<>' | '<' | '<=' | '>=' | '>' | '='
void expressao(void) {
    expressao_simples();
    if (lookahead == DIFERENTE || lookahead == MENOR || lookahead == MENOR_IGUAL ||
        lookahead == MAIOR_IGUAL || lookahead == MAIOR || lookahead == IGUAL) {
        consome(lookahead);
        expressao_simples();
    }
}

// <expressao_simples> ::= <termo> { <operador_adicao> <termo> }
// <operador_adicao> ::= '+' | '-' | mod | ou
void expressao_simples(void) {
    termo();
    while (lookahead == MAIS || lookahead == MENOS || lookahead == MOD || lookahead == OU) {
        consome(lookahead);
        termo();
    }
}

// <termo> ::= <fator> { <operador_multiplicacao> <fator> }
// <operador_multiplicacao> ::= '*' | div | e
void termo(void) {
    fator();
    while (lookahead == VEZES || lookahead == DIV || lookahead == E) {
        consome(lookahead);
        fator();
    }
}

// <fator> ::= identificador [ '(' <lista_expressao> ')' ] | constint | constchar |
//             '(' <expressao> ')' | ( '+' | '-' | nao ) <fator> | verdadeiro | falso
void fator(void) {
    switch (lookahead) {
        case IDENTIFICADOR:
            consome(IDENTIFICADOR);
            if (lookahead == ABRE_PAR) {
                consome(ABRE_PAR);
                lista_expressao();
                consome(FECHA_PAR);
            }
            break;
        case CONSTINT:   consome(CONSTINT);   break;
        case CONSTCHAR:  consome(CONSTCHAR);  break;
        case VERDADEIRO: consome(VERDADEIRO); break;
        case FALSO:      consome(FALSO);      break;
        case ABRE_PAR:
            consome(ABRE_PAR);
            expressao();
            consome(FECHA_PAR);
            break;
        case MAIS:
        case MENOS:
        case NAO:
            consome(lookahead);
            fator();
            break;
        default:
            erro_sintatico("fator");
    }
}

/* ===================== PRINCIPAL ===================== */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    ler_arquivo(argv[1]);

    proximo_atomo();   // inicializa o lookahead
    programa();        // símbolo inicial da gramática
    consome(EOS);

    printf("%d linhas analisadas, programa sintaticamente correto\n", contar_linhas());
    liberar_buffer();
    return 0;
}