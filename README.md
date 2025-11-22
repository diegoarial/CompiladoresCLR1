# CLR(1) Parser - Documentação Simples

Este projeto implementa um analisador sintático CLR(1) em C.

## Como utilizar

1. **Defina a gramática**
   - Edite a função `build_grammar()` no arquivo `grammar.c`.
   - Altere as produções e os tokens de teste conforme a gramática desejada.

2. **Compilação**
   - Compile o projeto com o comando:
     ```bash
     gcc parserCLR1.c grammar.c -o parser
     ```

3. **Execução**
   - Execute o analisador com:
     ```bash
     ./parser
     ```
   - O programa irá mostrar o processo de análise sintática dos tokens definidos.

## Observações
- Para testar diferentes gramáticas, basta modificar a função `build_grammar()` e recompilar.
- O resultado mostrará as operações de shift, reduce e se a entrada foi aceita ou rejeitada.

---

Projeto simples para fins didáticos e experimentação com gramáticas LR(1).
