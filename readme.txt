Compilador Portugol - Fase 2 (semantico + geracao de codigo MEPA)
Luiz Fernando Ferrari Batistela - 10427397

Pra compilar:
    gcc -Wall -Wno-unused-result -g -Og compilador.c hash_mack.o -o compilador

Pra rodar:
    ./compilador <arquivo_fonte>

Pra testar memoria:
    valgrind --leak-check=yes ./compilador <arquivo_fonte>

Pra rodar o codigo gerado na MEPA, copiei as linhas do INPP ate o PARA pra
um arquivo .mepa e rodei:
    ./mepa -r -f programa.mepa

O que o compilador mostra:
    Se nao tiver erro, mostra o codigo MEPA e depois a tabela de simbolos
    (entrada da hash, ID, categoria, escopo e endereco).
    Se tiver erro, mostra so a linha e o tipo do erro (lexico, sintatico ou
    semantico) e nao imprime nada de MEPA.

O que foi feito:
    - A fase 1 continua igual, so tirei a impressao dos atomos.
    - Tabela de simbolos com hash (TNo e TTabelaSimbolos do enunciado) usando
      o hashMack() do hash_mack.o. Cada entrada e uma lista encadeada.
    - Erro semantico pra identificador repetido no mesmo escopo, identificador
      nao declarado, uso errado (tipo chamar variavel como procedimento),
      numero errado de argumentos e passar algo que nao e variavel pra
      parametro var.
    - Geracao de MEPA pra tudo: variaveis, atribuicao, leia, escreva, se/senao,
      enquanto, expressoes, procedimentos, funcoes (inclusive recursivas) e
      parametros por valor e por referencia.

Algumas decisoes:
    - O codigo MEPA vai sendo guardado num buffer e so e impresso no final,
      assim se der erro no meio nao sai MEPA pela metade.
    - Globais ficam no nivel 0 e o que e de dentro de rotina fica no nivel 1.
      Os locais continuam na tabela depois que a rotina acaba (pra aparecer
      na impressao), mas so sao achados dentro da propria rotina.
    - Os identificadores sao salvos em minusculo porque a linguagem nao
      diferencia maiuscula de minuscula.
    - Parametros ficam em -(n+3-i) e o retorno da funcao em -(n+3). Quem chama
      a funcao faz AMEM 1 antes dos argumentos.
    - Parametro var: quem chama manda o endereco (CREN) e dentro da rotina usa
      CRVI e ARMI.
    - A MEPA nao tem mod, entao a mod b vira a - (a div b) * b.
    - O interpretador da MEPA nao roda programa sem nenhum rotulo, entao o
      rotulo do programa principal e sempre gerado.
    - O interpretador tambem nao tira os parametros da pilha no RTPR n (o
      procedimento3.mepa da aula fica em loop). Por isso usei RTPR 0 e quem
      chama tira os parametros com DMEM n depois do CHPR.

Bugs conhecidos:
    - Numero inteiro muito grande estoura e nao da erro.
    - No mod, se tiver chamada de funcao nos operandos a funcao roda 2 vezes.
    - Nao tem verificacao de tipo (o enunciado deixa so inteiro).
