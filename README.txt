Compilador Fase 1 - Análise Léxica e Sintática (Portugol)

Como compilar:
    gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador

Como executar:
    ./compilador <arquivo_fonte>

Teste de memória:
    valgrind --leak-check=yes ./compilador <arquivo_fonte>

O que foi concluído:
    - Analisador léxico completo: delimitadores, contagem de linhas, comentários
      {- -}, identificadores (limite de 15 caracteres), palavras reservadas sem
      diferenciar maiúsculas e minúsculas, constint (com notação exponencial,
      ex: 12E2 = 1200), constchar e todos os símbolos.
    - Analisador sintático descendente recursivo completo, com uma função para
      cada regra da gramática.
    - Mensagens de erro léxico e sintático com a linha do erro. No erro
      sintático é mostrado o átomo esperado e o átomo encontrado.

Decisões de projeto:
    - Tudo está em um único arquivo (compilador.c), como pede o comando de
      compilação
    - O arquivo fonte é lido inteiro para um buffer em memória, que é liberado
      no fim da execução (inclusive quando há erro), sem vazamento no valgrind.
    - Os comentários são enviados ao sintático, que imprime e descarta cada um
      na função proximo_atomo().
    - A atribuição e a chamada de procedimento começam com identificador. Por
      isso foram tratadas na mesma função, e o átomo seguinte (':=' ou não)
      decide qual das duas é
    - A palavra "nao" foi tratada como reservada porque aparece na gramática
      (regra <fator>), mesmo não estando na lista de palavras reservadas
    - A palavra "identificador" que aparece na lista de reservadas não foi
      incluída, pois não é usada em nenhuma regra da gramática.
    - Em caso de erro o programa termina com código de saída 1.
    - O número de linhas analisadas é o total de linhas do arquivo fonte.

Bugs conhecidos:
    - Constantes inteiras muito grandes acima do limite de int nao são
      verificadas e estouram o valor.