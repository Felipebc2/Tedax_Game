# Keep Solving and Nobody Explodes

Jogo de desarmamento de bombas implementado em C com ncurses e pthreads.

## Estrutura do Projeto

```
bomb_game/
├── src/
│   ├── main.c
│   ├── game.h/.c                # Lógica do jogo e implementação das threads
│   └── ui.h/.c                  # Interface ncurses
├── others/           
│   └── trabalho-pc-2025-02.pdf
├── Manual.md                    # Manual de Instruções dos módulos
└── Makefile                     # Compilação
```

### Instalação do ncurses (Ubuntu/Debian)

```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
```

## Compilação

### Usando Makefile

```bash
make
```

### Compilação manual

```bash
gcc -Wall -Wextra -std=c11 -Isrc -pthread src/main.c src/game.c src/ui.c -o jogo -lncurses -pthread
```

## Execução

```bash
./jogo
```

## Como Jogar

1. Ao iniciar o jogo, você verá um **menu de seleção de dificuldade** com três opções: **Fácil, Médio e Difícil**
2. Use as setas do teclado para navegar e ENTER para selecionar a dificuldade.
3. Cada módulo tem uma cor e requer uma sequência específica, que no arquivo de instruções está expecificado ao jogador
4. Digite o comando no formato `T<tedax>B<bancada>M<modulo>:<instrucao>` (veja seção "Sistema de Input" abaixo)
5. O tedax ocupará a bancada pelo tempo necessário para desarmar o módulo.
6. Se a instrução estiver correta, o módulo é desarmado. Se estiver incorreta, ele volta para o mural como ultimo da fila.
7. O jogo termina quando:
   - Todos os módulos necessários são desarmados (VITÓRIA)
   - O tempo acaba e ainda há módulos pendentes (DERROTA)
  
### Regras Gerais
- O jogo começa com **1 módulo já gerado**
- Novos módulos são gerados automaticamente conforme o intervalo da dificuldade escolhida
- A geração de módulos **para** quando o número máximo necessário é atingido
- Para vencer, você precisa resolver **todos os módulos necessários** da dificuldade escolhida

## Controles

- `BACKSPACE`: Remove o último caractere do comando
- `ENTER`: Envia o comando para processar
- `q`: Sair do jogo (força fim imediato)

## Sistema de Input

O jogo utiliza um sistema de comandos explícito onde você especifica qual tedax, bancada e módulo deseja usar, seguido da instrução.

### Formato do Comando

O formato do comando é: `T<tedax>B<bancada>M<modulo>:<instrucao>`

**Exemplos:**
- `T1B1M1:ppp` - Tedax 1, Bancada 1, Módulo 1, instrução "ppp"
- `T3B2M1:pp` - Tedax 3, Bancada 2, Módulo 1, instrução "pp"
- `B1M2:p` - Bancada 1, Módulo 2, instrução "p" (tedax será o primeiro disponível)
- `M1:ppp` - Módulo 1, instrução "ppp" (tedax e bancada serão os primeiros disponíveis)
- `:pp` - Apenas instrução "pp" (tedax, bancada e módulo serão os primeiros disponíveis)

### Regras de Default

Se você não especificar algum componente do comando, o sistema aplica as seguintes regras:

- **Tedax não especificado**: Usa o tedax livre com o índice mais baixo (primeiro disponível)
- **Bancada não especificada**: Usa a bancada livre com o índice mais baixo (primeira disponível)
- **Módulo não especificado**: Usa o primeiro módulo pendente da fila

### Validação

O sistema valida automaticamente:
- Se o tedax especificado está livre
- Se a bancada especificada está livre
- Se o módulo especificado está pendente
- Se todos os recursos necessários estão disponíveis

Se algum recurso não estiver disponível, o comando será ignorado e o buffer será limpo.

## Funcionamento das Threads

O jogo foi implementado usando programação concorrente com pthreads. Cada componente principal do jogo roda em uma thread separada, permitindo execução paralela e melhor responsividade.

### Threads Implementadas

1. **Thread do Mural de Módulos Pendentes** (`thread_mural`)
   - Responsável por gerar novos módulos conforme o intervalo da dificuldade
   - Gera módulos automaticamente a cada X segundos (dependendo da dificuldade)
   - Gera imediatamente um novo módulo se não houver módulos pendentes
   - Executa em loop contínuo enquanto o jogo está rodando

2. **Thread de Exibição de Informações** (`thread_exibicao`)
   - Responsável por atualizar a interface do jogo na tela
   - Redesenha a tela a cada 0.2 segundos
   - Mostra estado dos tedax, bancadas, módulos e informações do jogo
   - Filtra módulos resolvidos antigos para manter a tela limpa

3. **Threads dos Tedax** (`thread_tedax`)
   - Uma thread para cada tedax disponível (1-3 tedax)
   - Cada tedax processa seu módulo em execução independentemente
   - Decrementa o tempo restante do módulo a cada segundo
   - Verifica se a instrução estava correta quando o tempo acaba
   - Incrementa o contador de tempo desde resolvido para módulos resolvidos

4. **Thread do Coordenador** (`thread_coordenador`)
   - Responsável por processar a entrada do jogador
   - Lê teclas do teclado em tempo real
   - Processa comandos no formato `T<tedax>B<bancada>M<modulo>:<instrucao>`
   - Aplica regras de default quando componentes não são especificados
   - Valida disponibilidade de recursos antes de designar módulos
   - Gerencia o buffer de comando do jogador

### Sincronização

O jogo utiliza mecanismos de sincronização para garantir consistência dos dados compartilhados:

- **Mutex (`mutex_jogo`)**: Protege todas as operações de leitura/escrita no estado do jogo
  - Lista de módulos
  - Vetor de tedax
  - Vetor de bancadas
  - Tempo restante
  - Flags de controle do jogo

- **Condition Variables**: Usadas para sinalizar eventos importantes
  - `cond_modulo_disponivel`: Sinaliza quando há um novo módulo disponível
  - `cond_bancada_disponivel`: Sinaliza quando uma bancada fica livre
  - `cond_tela_atualizada`: Sinaliza quando a tela precisa ser atualizada

### Múltiplos Tedax e Bancadas

O jogo suporta configuração de 1 a 3 tedax e 1 a 3 bancadas:
- Cada tedax pode trabalhar em paralelo em um módulo diferente
- Cada tedax precisa ocupar uma bancada livre para desarmar um módulo

### Remoção Automática de Módulos Resolvidos

Para evitar que a tela fique cheia de módulos resolvidos:
- Módulos resolvidos são removidos automaticamente da exibição após um tempo
- Isso garante que sempre haja espaço na tela para módulos pendentes

## Notas

- **Mural de Módulos Pendentes**: Implementado na thread `thread_mural`
- **Exibição de Informações**: Implementado na thread `thread_exibicao`
- **Coordenador (Jogador)**: Implementado na thread `thread_coordenador`
- **Tedax**: Implementado nas threads `thread_tedax` (uma por tedax)
