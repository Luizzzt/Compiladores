/*
 * Compilador Fase 2: Análise Semântica e Geração de Código MEPA da linguagem Portugol
 * (a fase 1, análise léxica e sintática, continua neste mesmo arquivo)
 *
 * Compilar:
 *   gcc -Wall -Wno-unused-result -g -Og compilador.c hash_mack.o -o compilador
 * Executar:
 *   ./compilador <arquivo_fonte>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

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

// Nome de cada átomo, usado na busca das palavras reservadas (mesma ordem do enum)
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

/* ===================== TABELA DE SÍMBOLOS ===================== */
#define PRIME_NUMER 211     // numero primo para reduzir colisoes
#define MAX_PARAMETROS 20   // quantidade máxima de parâmetros de uma rotina

// Categoria de cada identificador guardado na tabela
typedef enum {
    CAT_PROGRAMA,
    CAT_VARIAVEL,
    CAT_PARAMETRO,
    CAT_FUNCAO,
    CAT_PROCEDIMENTO
} TCategoria;

char *nome_categoria[] = { "programa", "variavel", "parametro", "funcao", "procedimento" };

typedef struct _TNo {
    char ID[16];
    int endereco;           // deslocamento na MEPA (variável, parâmetro ou retorno da função)
    TCategoria categoria;
    int nivel;              // 0 = escopo global, 1 = escopo local (dentro de rotina)
    int por_referencia;     // 1 se for parâmetro declarado com var
    struct _TNo *rotina;    // rotina onde o identificador local foi declarado (NULL se global)
    int rotulo;             // rótulo MEPA onde começa a função ou procedimento
    int num_parametros;     // quantidade de parâmetros da função ou procedimento
    struct _TNo *parametros[MAX_PARAMETROS]; // parâmetros da rotina na ordem declarada
    struct _TNo *prox;
} TNo;

typedef struct {
    TNo *entradas[PRIME_NUMER];
} TTabelaSimbolos;

// função de dispersão implementada em hash_mack.o
int hashMack(char *s);

TTabelaSimbolos tabela;     // tabela de símbolos do compilador

// Deixa todas as entradas da tabela vazias
void inicializar_tabela(void) {
    for (int i = 0; i < PRIME_NUMER; i++)
        tabela.entradas[i] = NULL;
}

// Libera todos os nós das listas encadeadas da tabela
void liberar_tabela(void) {
    for (int i = 0; i < PRIME_NUMER; i++) {
        TNo *no = tabela.entradas[i];
        while (no != NULL) {
            TNo *prox = no->prox;
            free(no);
            no = prox;
        }
        tabela.entradas[i] = NULL;
    }
}

TNo *rotina_atual = NULL;   // rotina sendo analisada (NULL = programa principal)

// Copia o identificador em letras minúsculas, já que a linguagem não diferencia
// maiúsculas de minúsculas (num_1 e NUM_1 são o mesmo identificador)
void para_minusculo(char *destino, const char *origem) {
    int i;
    for (i = 0; origem[i] != '\0'; i++)
        destino[i] = tolower((unsigned char)origem[i]);
    destino[i] = '\0';
}

// Procura o identificador declarado no escopo atual.
// Dentro de uma rotina procura primeiro os locais (parâmetros e variáveis da rotina)
// e depois os globais. Retorna NULL se o identificador não foi declarado.
TNo *buscar_simbolo(const char *id) {
    char chave[16];
    TNo *no;
    para_minusculo(chave, id);
    int entrada = hashMack(chave);

    if (rotina_atual != NULL) {
        for (no = tabela.entradas[entrada]; no != NULL; no = no->prox)
            if (no->rotina == rotina_atual && strcmp(no->ID, chave) == 0)
                return no;
    }
    for (no = tabela.entradas[entrada]; no != NULL; no = no->prox)
        if (no->rotina == NULL && strcmp(no->ID, chave) == 0)
            return no;
    return NULL;
}

// Insere o identificador no escopo atual (global ou da rotina atual).
// Retorna NULL se o identificador já existe nesse mesmo escopo.
TNo *inserir_simbolo(const char *id, TCategoria categoria) {
    char chave[16];
    TNo *no;
    para_minusculo(chave, id);
    int entrada = hashMack(chave);

    // identificador repetido no mesmo escopo
    for (no = tabela.entradas[entrada]; no != NULL; no = no->prox)
        if (no->rotina == rotina_atual && strcmp(no->ID, chave) == 0)
            return NULL;

    no = (TNo *)malloc(sizeof(TNo));
    if (no == NULL) {
        printf("Erro: memória insuficiente\n");
        exit(1);
    }
    memset(no, 0, sizeof(TNo));
    strcpy(no->ID, chave);
    no->categoria = categoria;
    no->rotina = rotina_atual;
    no->nivel = (rotina_atual == NULL) ? 0 : 1;

    // insere no começo da lista encadeada da entrada
    no->prox = tabela.entradas[entrada];
    tabela.entradas[entrada] = no;
    return no;
}

/* ===================== GERAÇÃO DE CÓDIGO MEPA ===================== */
// O código MEPA é guardado em memória e só é impresso no final da compilação,
// assim se acontecer algum erro, nenhuma instrução aparece na saída.
char *codigo = NULL;        // texto com as instruções MEPA geradas
size_t tamanho_codigo = 0;  // quantidade de caracteres usados em 'codigo'
size_t capacidade_codigo = 0;
int contador_rotulo = 0;    // último rótulo usado (L1, L2, ...)

// Acrescenta um texto ja formatado no fim do codigo
void acrescenta_codigo(const char *texto, size_t tamanho) {
    if (tamanho_codigo + tamanho + 1 > capacidade_codigo) {
        size_t nova = (capacidade_codigo == 0) ? 1024 : capacidade_codigo * 2;
        while (nova < tamanho_codigo + tamanho + 1)
            nova *= 2;
        char *novo = (char *)realloc(codigo, nova);
        if (novo == NULL) {
            printf("Erro: memória insuficiente\n");
            exit(1);
        }
        codigo = novo;
        capacidade_codigo = nova;
    }
    memcpy(codigo + tamanho_codigo, texto, tamanho);
    tamanho_codigo += tamanho;
    codigo[tamanho_codigo] = '\0';
}

// Gera uma instrução MEPA (mesmo formato do printf), ex: gera("CRVL %d,%d", 0, 1)
void gera(const char *formato, ...) {
    char linha[64];
    va_list args;
    va_start(args, formato);
    int n = vsnprintf(linha + 3, sizeof(linha) - 4, formato, args);
    va_end(args);
    memcpy(linha, "   ", 3);   // recuo das instruções, como nos exemplos da MEPA
    n += 3;
    linha[n++] = '\n';
    acrescenta_codigo(linha, n);
}

// Gera a linha de um rótulo: Ln:NADA
void gera_rotulo(int rotulo) {
    char linha[32];
    int n = sprintf(linha, "L%d:NADA\n", rotulo);
    acrescenta_codigo(linha, n);
}

// Retorna o próximo rótulo livre (1, 2, 3, ...)
int proximo_rotulo(void) {
    return ++contador_rotulo;
}

// Gera de novo uma cópia do trecho de codgo que está entre as posições inicio e fim.
// É usado no operador mod que não existe na MEPA.
void repete_codigo(size_t inicio, size_t fim) {
    size_t tamanho = fim - inicio;
    char *copia = (char *)malloc(tamanho + 1);
    if (copia == NULL) {
        printf("Erro: memória insuficiente\n");
        exit(1);
    }
    memcpy(copia, codigo + inicio, tamanho);   // copia antes pois o realoc pode mover 'codigo'
    acrescenta_codigo(copia, tamanho);
    free(copia);
}

// Imprime todo o código MEPA gerado
void imprimir_codigo(void) {
    if (codigo != NULL)
        printf("%s", codigo);
}

void liberar_codigo(void) {
    free(codigo);
    codigo = NULL;
    tamanho_codigo = capacidade_codigo = 0;
}

/* ===================== VARIÁVEIS GLOBAIS DO LÉXICO ===================== */
char *buffer;          // posição atual de leitura no código fonte
char *inicio_buffer;   // início do buffer (usado para liberar a memória)
int conta_linha = 1;   // linha atual do código fonte
char msg_erro_lexico[100]; // descrição do último erro léxico encontrado

/* ===================== VARIÁVEIS GLOBAIS DO SINTÁTICO ===================== */
TAtomo lookahead;       // átomo atual
TInfoAtomo info_atomo;  // informações do átomo atual

/* ===================== VARIÁVEIS GLOBAIS DO SEMÂNTICO ===================== */
char ultimo_id[16];     // lexema do último identificador consumido
int linha_ultimo_id;    // linha do último identificador consumido
int proximo_endereco;   // próximo endereço livre para variável no escopo atual

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

/* ===================== ANALISADOR SINTÁTICO ===================== */

// protótipos das funções da gramática
void programa(void);
void bloco(void);
int declaracao_variaveis(void);
void lista_variaveis(TCategoria categoria, int por_referencia);
void declaracao_de_rotinas(void);
void declaracao_de_funcao(void);
void declaracao_de_procedimento(void);
void parametros_formais(void);
void corpo_rotina(TNo *rotina);
void parametro_formal(void);
void tipo(void);
void comando_composto(void);
void comando(void);
void comando_atribuicao_ou_chamada(void);
void comando_entrada(void);
void comando_saida(void);
void comando_condicional(void);
void comando_repeticao(void);
void argumentos(TNo *rotina);
void expressao(void);
void expressao_simples(void);
void termo(void);
void fator(void);

// Libera a memória e termina o programa
void encerrar(int codigo_saida) {
    liberar_buffer();
    liberar_tabela();
    liberar_codigo();
    exit(codigo_saida);
}

// Mostra o erro sintático e termina a execução
void erro_sintatico(const char *esperado) {
    printf("#%3d:erro sintatico, esperado [%s] encontrado [%s]\n",
           info_atomo.linha, esperado, simbolo_atomo[lookahead]);
    encerrar(1);
}

// Mostra o erro semantico com a linha onde ocorreu e termina a execução
void erro_semantico(int linha, const char *formato, ...) {
    va_list args;
    printf("#%3d:erro semantico, ", linha);
    va_start(args, formato);
    vprintf(formato, args);
    va_end(args);
    printf("\n");
    encerrar(1);
}

// Pede o próximo átomo ao léxico.
// Comentários são descartados; erro léxico termina a execução.
void proximo_atomo(void) {
    info_atomo = obter_atomo();
    while (info_atomo.atomo == COMENTARIO)
        info_atomo = obter_atomo();
    if (info_atomo.atomo == ERRO) {
        printf("#%3d:erro lexico, %s\n", info_atomo.linha, msg_erro_lexico);
        encerrar(1);
    }
    lookahead = info_atomo.atomo;
}

// Confere se o átomo atual é o esperado e avança
// (na fase 2 os átomos não são mais impressos)
void consome(TAtomo atomo) {
    if (lookahead == atomo) {
        proximo_atomo();
    }
    else {
        erro_sintatico(simbolo_atomo[atomo]);
    }
}

// Consome um identificador guardando o lexema e a linha em ultimo_id e linha_ultimo_id
void consome_identificador(void) {
    if (lookahead == IDENTIFICADOR) {
        strcpy(ultimo_id, info_atomo.atributo.id);
        linha_ultimo_id = info_atomo.linha;
    }
    consome(IDENTIFICADOR);
}

// Consome o identificador que esta sendo declarado e insere na tabela de símbolos.
// Se ele já existe no escopo atual ocorre erro semântico.
TNo *declara_identificador(TCategoria categoria) {
    consome_identificador();
    TNo *no = inserir_simbolo(ultimo_id, categoria);
    if (no == NULL)
        erro_semantico(linha_ultimo_id, "identificador [%s] ja foi declarado neste escopo", ultimo_id);
    return no;
}

// Consome um identificador usado no corpo do programa e devolve o nó dele na tabela.
// Se o identificador nao foi declarado ocorre erro semântico.
TNo *usa_identificador(void) {
    consome_identificador();
    TNo *no = buscar_simbolo(ultimo_id);
    if (no == NULL)
        erro_semantico(linha_ultimo_id, "identificador [%s] nao foi declarado", ultimo_id);
    return no;
}

// Retorna 1 se o identificador pode ser usado como variável (variável ou parâmetro)
int eh_variavel(TNo *no) {
    return no->categoria == CAT_VARIAVEL || no->categoria == CAT_PARAMETRO;
}

// Gera o código que empilha o valor da variável ou parâmetro.
// nivel 0 = global (D[0]), nivel 1 = local (D[1]).
// Parametro por referência guarda o endereço entao usa carga indireta (CRVI).
void carrega_variavel(TNo *no) {
    if (no->categoria == CAT_PARAMETRO && no->por_referencia)
        gera("CRVI %d,%d", no->nivel, no->endereco);
    else
        gera("CRVL %d,%d", no->nivel, no->endereco);
}

// Gera o código que tira o valor do topo da pilha e guarda na varável ou parâmetro.
// Parametro por referencia usa armazenamento indireto (ARMI).
void armazena_variavel(TNo *no) {
    if (no->categoria == CAT_PARAMETRO && no->por_referencia)
        gera("ARMI %d,%d", no->nivel, no->endereco);
    else
        gera("ARMZ %d,%d", no->nivel, no->endereco);
}

// <programa> ::= algoritmo identificador ';' <bloco> '.'
void programa(void) {
    consome(ALGORITMO);
    declara_identificador(CAT_PROGRAMA);
    proximo_endereco = 0;   // variáveis globais começam no endereço 0 de D[0]
    consome(PONTO_VIRGULA);
    gera("INPP");
    bloco();
    consome(PONTO);
    gera("PARA");
}

// <bloco> ::= <declaracao_variaveis> <declaracao_de_rotinas> <comando_composto>
// MEPA:  AMEM n            (variáveis globais)
//        DSVS Lp           (pula o codigo das rotinas se existirem)
//        ... rotinas ...
//     Lp:NADA              (sempre gerado: o interpretador da MEPA nao executa
//                           programas que não têm nenhum rótulo)
//        ... comandos do programa principal ...
//        DMEM n
void bloco(void) {
    int num_variaveis = declaracao_variaveis();
    if (num_variaveis > 0)
        gera("AMEM %d", num_variaveis);

    int rotulo_principal = proximo_rotulo();
    if (lookahead == FUNCAO || lookahead == PROCEDIMENTO)
        gera("DSVS L%d", rotulo_principal);
    declaracao_de_rotinas();
    gera_rotulo(rotulo_principal);

    comando_composto();
    if (num_variaveis > 0)
        gera("DMEM %d", num_variaveis);
}

// <declaracao_variaveis> ::= [ var <lista_variaveis> ';' { <lista_variaveis> ';' } ]
// Retorna quantas variaveis foram declaradas (usado no AMEM e DMEM)
int declaracao_variaveis(void) {
    int inicio = proximo_endereco;
    if (lookahead == VAR) {
        consome(VAR);
        lista_variaveis(CAT_VARIAVEL, 0);
        consome(PONTO_VIRGULA);
        while (lookahead == IDENTIFICADOR) {
            lista_variaveis(CAT_VARIAVEL, 0);
            consome(PONTO_VIRGULA);
        }
    }
    return proximo_endereco - inicio;
}

// Declara um identificador da lista: variável recebe o próximo endereço livre,
// parâmetro é guardado na rotina atual (o endereço é calculado depois)
void declara_variavel_ou_parametro(TCategoria categoria, int por_referencia) {
    TNo *no = declara_identificador(categoria);
    if (categoria == CAT_VARIAVEL) {
        no->endereco = proximo_endereco++;
    }
    else {
        if (rotina_atual->num_parametros == MAX_PARAMETROS)
            erro_semantico(linha_ultimo_id, "rotina [%s] com mais de %d parametros",
                           rotina_atual->ID, MAX_PARAMETROS);
        no->por_referencia = por_referencia;
        rotina_atual->parametros[rotina_atual->num_parametros++] = no;
    }
}

// <lista_variaveis> ::= identificador { ',' identificador } ':' <tipo>
void lista_variaveis(TCategoria categoria, int por_referencia) {
    declara_variavel_ou_parametro(categoria, por_referencia);
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        declara_variavel_ou_parametro(categoria, por_referencia);
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

// Parte comum da funcão e do procedimento:
//   <parametros_formais> <declaracao_variaveis> <comando_composto>
// MEPA:   Lr:NADA
//            ENPR
//            AMEM k      (variáveis locais)
//            [comandos]
//            DMEM k
//            RTPR 0
// Observacao: no interpretador da MEPA usado na disciplina a instrução RTPR n
// não retira os n parâmetros da pilha (só retira o endereço de retorno e o D[1]
// salvo). Por isso é gerado RTPR 0 e quem chama a rotina retira os parametos
// com DMEM n logo depois do CHPR. Assim o código funciona tanto nesse
// interpretador quanto numa MEPA que siga a definiçao da aula.
void corpo_rotina(TNo *rotina) {
    rotina_atual = rotina;     // a partir daqui o escopo é local
    proximo_endereco = 0;      // variáveis locais começam no endereço 0 de D[1]
    rotina->rotulo = proximo_rotulo();
    gera_rotulo(rotina->rotulo);
    gera("ENPR");

    parametros_formais();
    if (rotina->categoria == CAT_FUNCAO)
        // o valor de retorno fica logo abaixo dos parâmetros na pilha: -(n+3)
        rotina->endereco = -(rotina->num_parametros + 3);

    int num_locais = declaracao_variaveis();
    if (num_locais > 0)
        gera("AMEM %d", num_locais);
    comando_composto();
    if (num_locais > 0)
        gera("DMEM %d", num_locais);
    gera("RTPR 0");

    rotina_atual = NULL;       // volta para o escopo global
}

// <declaracao_de_funcao> ::= funcao <tipo> identificador <parametros_formais>
//                            <declaracao_variaveis> <comando_composto>
void declaracao_de_funcao(void) {
    consome(FUNCAO);
    tipo();
    TNo *funcao = declara_identificador(CAT_FUNCAO);   // nome da função é global
    corpo_rotina(funcao);
}

// <declaracao_de_procedimento> ::= procedimento identificador <parametros_formais>
//                                  <declaracao_variaveis> <comando_composto>
void declaracao_de_procedimento(void) {
    consome(PROCEDIMENTO);
    TNo *procedimento = declara_identificador(CAT_PROCEDIMENTO);   // nome é global
    corpo_rotina(procedimento);
}

// <parametros_formais> ::= '(' <parametro_formal> { ';' <parametro_formal> } ')' | '(' ')'
// Depois de ler todos os parâmetros calcula o endereço de cada um.
// Com n parâmetros, o i-ésimo (i começa em 1) fica em -(n+3-i) relativo a D[1].
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

    int n = rotina_atual->num_parametros;
    for (int i = 1; i <= n; i++)
        rotina_atual->parametros[i - 1]->endereco = -(n + 3 - i);
}

// <parametro_formal> ::= [ var ] <lista_variaveis>
// Com var o parâmetro é passado por referência sem var é passado por valor
void parametro_formal(void) {
    int por_referencia = 0;
    if (lookahead == VAR) {
        consome(VAR);
        por_referencia = 1;
    }
    lista_variaveis(CAT_PARAMETRO, por_referencia);
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
    TNo *no = usa_identificador();
    int linha = linha_ultimo_id;

    if (lookahead == ATRIBUICAO) {
        // pode receber valor: variável, parâmetro ou o nome da funçao
        // dentro dela mesma (valor de retorno)
        if (!eh_variavel(no) && !(no->categoria == CAT_FUNCAO && no == rotina_atual))
            erro_semantico(linha, "[%s] e %s e nao pode receber atribuicao",
                           no->ID, nome_categoria[no->categoria]);
        consome(ATRIBUICAO);
        expressao();
        if (no->categoria == CAT_FUNCAO)
            gera("ARMZ 1,%d", no->endereco);   // valor de retono da função
        else
            armazena_variavel(no);
    }
    else {
        if (no->categoria != CAT_PROCEDIMENTO)
            erro_semantico(linha, "[%s] e %s e nao pode ser chamado como procedimento",
                           no->ID, nome_categoria[no->categoria]);
        argumentos(no);
    }
}

// Argumento passado para um parâmetro por referência (var) precisa ser uma variavel.
// É empilhado o endereço da variável e não o seu valor:
//   variável ou parâmetro por valor -> CREN nivel,endereco
//   parâmetro por referência        -> CRVL nivel,endereco (ele já guarda um endereço)
void argumento_referencia(TNo *rotina) {
    if (lookahead != IDENTIFICADOR)
        erro_semantico(info_atomo.linha, "parametro por referencia de [%s] precisa receber uma variavel",
                       rotina->ID);
    TNo *no = usa_identificador();
    if (!eh_variavel(no) || (lookahead != VIRGULA && lookahead != FECHA_PAR))
        erro_semantico(linha_ultimo_id, "parametro por referencia de [%s] precisa receber uma variavel",
                       rotina->ID);
    if (no->categoria == CAT_PARAMETRO && no->por_referencia)
        gera("CRVL %d,%d", no->nivel, no->endereco);
    else
        gera("CREN %d,%d", no->nivel, no->endereco);
}

// Um argumento da chamada: usa a forma de passagem do parâmetro na mesma posiçao
void argumento(TNo *rotina, int posicao) {
    if (posicao < rotina->num_parametros && rotina->parametros[posicao]->por_referencia)
        argumento_referencia(rotina);
    else
        expressao();   // passagem por valor: empilha o valor da expresão
}

// Argumentos da chamada de função ou procedimento: [ '(' <lista_expressao> ')' ]
// Confere se a quantidade de argumentos é igual à quantidade de parâmetros.
// MEPA: cada argumento é empilhado na ordem, depois CHPR Lr e DMEM n
// (o DMEM retira os parâmetros da pilha veja o comentario de corpo_rotina)
void argumentos(TNo *rotina) {
    int linha = linha_ultimo_id;
    int quantidade = 0;
    if (lookahead == ABRE_PAR) {
        consome(ABRE_PAR);
        argumento(rotina, quantidade);
        quantidade++;
        while (lookahead == VIRGULA) {
            consome(VIRGULA);
            argumento(rotina, quantidade);
            quantidade++;
        }
        consome(FECHA_PAR);
    }
    if (quantidade != rotina->num_parametros)
        erro_semantico(linha, "[%s] espera %d parametro(s) e recebeu %d",
                       rotina->ID, rotina->num_parametros, quantidade);
    gera("CHPR L%d", rotina->rotulo);
    if (rotina->num_parametros > 0)
        gera("DMEM %d", rotina->num_parametros);
}

// Identificador do comando leia: precisa ser variável ou parâmetro
void le_variavel(void) {
    TNo *no = usa_identificador();
    if (!eh_variavel(no))
        erro_semantico(linha_ultimo_id, "[%s] e %s e nao pode ser usado no leia",
                       no->ID, nome_categoria[no->categoria]);
    gera("LEIT");
    armazena_variavel(no);
}

// <comando_entrada> ::= leia '(' identificador { ',' identificador } ')'
void comando_entrada(void) {
    consome(LEIA);
    consome(ABRE_PAR);
    le_variavel();
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        le_variavel();
    }
    consome(FECHA_PAR);
}

// <comando_saida> ::= escreva '(' <lista_expressao> ')'
// Cada expressão da lista e calculada e impressa com IMPR
void comando_saida(void) {
    consome(ESCREVA);
    consome(ABRE_PAR);
    expressao();
    gera("IMPR");
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        expressao();
        gera("IMPR");
    }
    consome(FECHA_PAR);
}

// <comando_condicional> ::= se <expressao> entao <comando> [ senao <comando> ]
// MEPA:      [expressao]             MEPA sem senao:    [expressao]
//            DSVF L1                                     DSVF L1
//            [comando do entao]                          [comando do entao]
//            DSVS L2                                  L1:NADA
//         L1:NADA
//            [comando do senao]
//         L2:NADA
void comando_condicional(void) {
    int rotulo_falso = proximo_rotulo();
    consome(SE);
    expressao();
    gera("DSVF L%d", rotulo_falso);
    consome(ENTAO);
    comando();
    if (lookahead == SENAO) {
        int rotulo_fim = proximo_rotulo();
        gera("DSVS L%d", rotulo_fim);
        gera_rotulo(rotulo_falso);
        consome(SENAO);
        comando();
        gera_rotulo(rotulo_fim);
    }
    else {
        gera_rotulo(rotulo_falso);
    }
}

// <comando_repeticao> ::= enquanto <expressao> faca <comando>
// MEPA:   L1:NADA
//            [expressao]
//            DSVF L2
//            [comando]
//            DSVS L1
//         L2:NADA
void comando_repeticao(void) {
    int rotulo_inicio = proximo_rotulo();
    int rotulo_fim = proximo_rotulo();
    consome(ENQUANTO);
    gera_rotulo(rotulo_inicio);
    expressao();
    gera("DSVF L%d", rotulo_fim);
    consome(FACA);
    comando();
    gera("DSVS L%d", rotulo_inicio);
    gera_rotulo(rotulo_fim);
}

// <expressao> ::= <expressao_simples> [ <operador_relacional> <expressao_simples> ]
// <operador_relacional> ::= '<>' | '<' | '<=' | '>=' | '>' | '='
void expressao(void) {
    expressao_simples();
    if (lookahead == DIFERENTE || lookahead == MENOR || lookahead == MENOR_IGUAL ||
        lookahead == MAIOR_IGUAL || lookahead == MAIOR || lookahead == IGUAL) {
        TAtomo operador = lookahead;
        consome(lookahead);
        expressao_simples();
        switch (operador) {
            case DIFERENTE:   gera("CMDG"); break;
            case MENOR:       gera("CMME"); break;
            case MENOR_IGUAL: gera("CMEG"); break;
            case MAIOR_IGUAL: gera("CMAG"); break;
            case MAIOR:       gera("CMMA"); break;
            default:          gera("CMIG"); break;   // IGUAL
        }
    }
}

// <expressao_simples> ::= <termo> { <operador_adicao> <termo> }
// <operador_adicao> ::= '+' | '-' | mod | ou
// A instrução do operador é gerada depois dos dois operandos (notação pós-fixa)
//
// A MEPA não tem instrução para o resto da divisao, então a mod b é calculado
// como a - (a div b) * b. O código de a e b é gerado de novo depois deles:
//   [a] [b] [a] [b] DIVI MULT SUBT
// pilha: a b a b -> a b (a div b) -> a (b * (a div b)) -> a - b * (a div b)
void expressao_simples(void) {
    size_t inicio = tamanho_codigo;   // onde começa o código do operando da esquerda
    termo();
    while (lookahead == MAIS || lookahead == MENOS || lookahead == MOD || lookahead == OU) {
        TAtomo operador = lookahead;
        consome(lookahead);
        termo();
        if (operador == MAIS)
            gera("SOMA");
        else if (operador == MENOS)
            gera("SUBT");
        else if (operador == OU)
            gera("DISJ");
        else {   // MOD
            repete_codigo(inicio, tamanho_codigo);
            gera("DIVI");
            gera("MULT");
            gera("SUBT");
        }
    }
}

// <termo> ::= <fator> { <operador_multiplicacao> <fator> }
// <operador_multiplicacao> ::= '*' | div | e
void termo(void) {
    fator();
    while (lookahead == VEZES || lookahead == DIV || lookahead == E) {
        TAtomo operador = lookahead;
        consome(lookahead);
        fator();
        if (operador == VEZES)
            gera("MULT");
        else if (operador == DIV)
            gera("DIVI");
        else
            gera("CONJ");
    }
}

// <fator> ::= identificador [ '(' <lista_expressao> ')' ] | constint | constchar |
//             '(' <expressao> ')' | ( '+' | '-' | nao ) <fator> | verdadeiro | falso
void fator(void) {
    switch (lookahead) {
        case IDENTIFICADOR: {
            TNo *no = usa_identificador();
            if (no->categoria == CAT_FUNCAO) {
                // chamada de função: AMEM 1 reserva o espaço do valor de retorno,
                // que fica no topo da pilha depois que os parâmetros sao retirados
                gera("AMEM 1");
                argumentos(no);
            }
            else if (eh_variavel(no)) {
                if (lookahead == ABRE_PAR)
                    erro_semantico(linha_ultimo_id, "[%s] e %s e nao pode ser chamado como funcao",
                                   no->ID, nome_categoria[no->categoria]);
                carrega_variavel(no);
            }
            else {
                erro_semantico(linha_ultimo_id, "[%s] e %s e nao pode ser usado em expressao",
                               no->ID, nome_categoria[no->categoria]);
            }
            break;
        }
        case CONSTINT:
            gera("CRCT %d", info_atomo.atributo.numero);
            consome(CONSTINT);
            break;
        case CONSTCHAR:   // o caractere vira o seu código ASCII
            gera("CRCT %d", (int)info_atomo.atributo.ch);
            consome(CONSTCHAR);
            break;
        case VERDADEIRO:  // verdadeiro = 1 e falso = 0
            gera("CRCT 1");
            consome(VERDADEIRO);
            break;
        case FALSO:
            gera("CRCT 0");
            consome(FALSO);
            break;
        case ABRE_PAR:
            consome(ABRE_PAR);
            expressao();
            consome(FECHA_PAR);
            break;
        case MAIS:        // + unário não muda o valor
            consome(MAIS);
            fator();
            break;
        case MENOS:       // - unario inverte o sinal
            consome(MENOS);
            fator();
            gera("INVR");
            break;
        case NAO:         // nao inverte o valor logco (1 vira 0 e 0 vira 1)
            consome(NAO);
            fator();
            gera("NEGA");
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
    inicializar_tabela();

    proximo_atomo();   // inicializa o lookahead
    programa();        // símbolo inicial da gramática
    consome(EOS);

    imprimir_codigo();
    printf("%d linhas analisadas, programa sintaticamente correto\n", contar_linhas());
    liberar_buffer();
    liberar_tabela();
    liberar_codigo();
    return 0;
}