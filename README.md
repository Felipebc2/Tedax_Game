# Keep Solving and Nobody Explodes

Jogo de desarmamento de bombas implementado em C com ncurses, pthreads e suporte a áudio.

## Estrutura do Projeto

```
bomb_game/
├── src/
│   ├── main/              # Ponto de entrada do jogo
│   │   └── main.c
│   ├── game/              # Lógica do jogo e implementação das threads
│   │   ├── game.h
│   │   └── game.c
│   ├── ui/                # Interface ncurses
│   │   ├── ui.h
│   │   └── ui.c
│   ├── audio/             # Sistema de áudio (SDL2_mixer)
│   │   ├── audio.h
│   │   └── audio.c
│   └── fases/             # Configurações das fases/dificuldades
│       ├── fases.h
│       └── fases.c
├── others/           
│   └── trabalho-pc-2025-02.pdf
├── sounds/                # Músicas e Sound Effects
│       └── .mp3           # Diversos .mp3 sem Copyright
├── Manual.md              # Manual de Instruções dos módulos
├── Makefile               # Compilação
└── Readme.md
```

### Instalação de Dependências

```bash
# Biblioteca ncurses (obrigatória)
sudo apt-get install libncurses5-dev libncursesw5-dev

# Biblioteca SDL2_mixer (opcional - para suporte a música)
sudo apt-get install libsdl2-mixer-dev
```

**Nota**: O jogo funciona sem SDL2_mixer, mas a funcionalidade de música estará desabilitada.

## Compilação

### Usando Makefile

```bash
make
```

O Makefile detecta automaticamente se SDL2_mixer está disponível e compila com suporte a áudio se encontrado.

### Compilação manual

```bash
gcc -Wall -Wextra -std=c11 -Isrc/main -Isrc/game -Isrc/ui -Isrc/audio -Isrc/fases -pthread \
    src/main/main.c src/game/game.c src/ui/ui.c src/audio/audio.c src/fases/fases.c \
    -o jogo -lncurses -pthread -lSDL2_mixer -lSDL2
```

## Execução

```bash
./jogo
```

## Como Jogar

1. Ao iniciar o jogo, você verá o **menu principal** com as seguintes opções:
   - **Classico**: Modo clássico do jogo (funcional)
   - **Especialistas [Em Breve]**: Modo com tedaxes especialistas
   - **Sobrevivencia [Em Breve]**: Modo de sobrevivência
   - **Extras [Em Breve]**: Conteúdo extra, como desafios
   - **Treino [Em Breve]**: Modo de treino, treine os módulos mais difíceis
   - **Custom [Em Breve]**: Modo personalizado
   - **M. Ligar/Desligar Musica**: Switch para Ligar e Desligar sons
   - **C. Configs [Em Breve]**: Configurações
   - **Q. Sair**: Sair do jogo

2. Selecione **Classico** para jogar o modo principal.

3. No menu de dificuldades, escolha uma das três opções: **Fácil, Médio e Difícil**

4. Use as setas do teclado para navegar e ENTER para selecionar.

5. Cada módulo requer uma sequência específica (consulte o `Manual.md` para detalhes):

6. Digite o comando no formato `T<tedax>B<bancada>M<modulo>:<instrucao>` (veja seção "Sistema de Input" abaixo).

7. O tedax ocupará a bancada pelo tempo necessário para desarmar o módulo (varia conforme a dificuldade).

8. Se a instrução estiver correta, o módulo é desarmado. Se estiver incorreta, ele volta para o mural como pendente.

9. O jogo termina quando:
   - Todos os módulos necessários são desarmados (VITÓRIA)
   - O tempo acaba e ainda há módulos pendentes (DERROTA)

10. Após o fim do jogo, você verá um menu pós-jogo com opções para voltar ao menu principal ou sair.

### Regras Gerais

- O jogo começa com alguns módulos já gerados
- Novos módulos são gerados automaticamente conforme um intervalo randômico
- A geração de módulos **para** quando o número máximo necessário é atingido
- Para vencer, você precisa resolver **todos os módulos necessários**
- Módulos resolvidos são removidos automaticamente da exibição após um tempo para manter a tela limpa
- Cada tedax pode ter no máximo **1 módulo em espera** quando estiver ocupado, criando filas.
- Se um tedax estiver ocupado e você atribuir um novo módulo, ele será adicionado à fila de espera
- Se uma bancada estiver ocupada, o tedax pode entrar em espera por aquela bancada específica

## Controles

- `BACKSPACE`: Remove o último caractere do comando
- `ENTER`: Envia o comando para processar
- `q`: Sair do jogo (força fim imediato)
- `M`: No menu principal, alterna música ligada/desligada (se áudio estiver disponível)

## Sistema de Input

O jogo utiliza um sistema de comandos explícito onde você especifica qual tedax, bancada e módulo deseja usar, seguido da instrução. O sistema é flexível e permite especificação parcial ou completa dos recursos.

### Formato do Comando

O formato do comando é: `T<tedax>B<bancada>M<modulo>:<instrucao>`

**Exemplos:**
- `T1B1M1:ppp` - Tedax 1, Bancada 1, Módulo 1, instrução "ppp"
- `T3B2M5:pp` - Tedax 3, Bancada 2, Módulo 5, instrução "pp"
- `B1M2:p` - Bancada 1, Módulo 2, instrução "p" (tedax será o primeiro disponível)
- `M1:ppp` - Módulo 1, instrução "ppp" (tedax e bancada serão os primeiros disponíveis)
- `ppp` - Apenas instrução "ppp" (tedax, bancada e módulo serão os primeiros disponíveis)
- `:pp` - Apenas instrução "pp" (tedax, bancada e módulo serão os primeiros disponíveis)

### Regras de Default

Se você não especificar algum componente do comando, o sistema aplica as seguintes regras:

- **Tedax não especificado**: Usa o tedax livre com o índice mais baixo (primeiro disponível). Se todos estiverem ocupados, retorna um aviso.
- **Bancada não especificada**: Usa a bancada livre com o índice mais baixo (primeira disponível). Se não houver livre, permite que o tedax entre em espera pela primeira bancada.
- **Módulo não especificado**: Usa o primeiro módulo pendente da lista

### Validação

O sistema valida automaticamente:
- Se o tedax especificado está livre ou em espera (pode ser reutilizado se em espera)
- Se a bancada especificada existe
- Se o módulo especificado está pendente
- Se todos os recursos necessários estão disponíveis

## Funcionamento das Threads

O jogo foi implementado usando programação concorrente com pthreads. Cada componente principal do jogo roda em uma thread separada, permitindo execução paralela e melhor responsividade. O estado do jogo é protegido por um mutex global para evitar condições de corrida.

### Threads Implementadas

1. **Thread do Mural de Módulos Pendentes** (`thread_mural`)
   - Responsável por gerar novos módulos conforme o intervalo configurado na fase
   - Gera módulos automaticamente a cada X segundos (dependendo da dificuldade)
   - Gera imediatamente um novo módulo se não houver módulos pendentes
   - Executa em loop contínuo enquanto o jogo está rodando
   - Usa mutex para proteger acesso ao estado do jogo

2. **Thread de Exibição de Informações** (`thread_exibicao`)
   - Responsável por atualizar a interface do jogo na tela
   - Redesenha a tela a cada 0.2 segundos
   - Mostra estado dos tedax, bancadas, módulos e informações do jogo
   - Filtra módulos resolvidos antigos para manter a tela limpa (remove após 10-20 segundos dependendo da quantidade)
   - Exibe mensagens de erro quando comandos inválidos são inseridos
   - Mostra fila de espera dos tedax quando aplicável

3. **Threads dos Tedax** (`thread_tedax`)
   - Uma thread para cada tedax disponível (1-4 tedax dependendo da dificuldade)
   - Cada tedax processa seu módulo em execução independentemente
   - Decrementa o tempo restante do módulo a cada segundo
   - Verifica se a instrução estava correta quando o tempo acaba
   - Incrementa o contador de tempo desde resolvido para módulos resolvidos
   - Quando termina um módulo, verifica se há módulos na fila de espera e processa o próximo automaticamente
   - Gerencia a transição de tedax em espera para ocupado quando a bancada fica livre
   - Usa mutex para proteger acesso ao estado do jogo

4. **Thread do Coordenador** (`thread_coordenador`)
   - Responsável por processar a entrada do jogador
   - Lê teclas do teclado em tempo real
   - Processa comandos no formato `T<tedax>B<bancada>M<modulo>:<instrucao>`
   - Aplica regras de default quando componentes não são especificados
   - Valida disponibilidade de recursos antes de designar módulos
   - Gerencia o buffer de comando do jogador
   - Implementa lógica de fila de espera para tedax ocupados
   - Implementa lógica de espera de bancadas para tedax
   - Usa mutex para proteger acesso ao estado do jogo

### Sincronização

O jogo utiliza mecanismos de sincronização para garantir consistência dos dados compartilhados:

- **Mutex (`mutex_jogo`)**: Protege todas as operações de leitura/escrita no estado do jogo
  - Lista de módulos
  - Vetor de tedax (incluindo filas de espera)
  - Vetor de bancadas (incluindo informações de espera)
  - Tempo restante
  - Flags de controle do jogo (jogo_rodando, jogo_terminou)
  - Contador de erros
  - Mensagens de erro

- **Condition Variables**: Usadas para sinalizar eventos importantes e evitar busy-waiting
  - `cond_modulo_disponivel`: Sinaliza quando há um novo módulo disponível
  - `cond_bancada_disponivel`: Sinaliza quando uma bancada fica livre
  - `cond_tela_atualizada`: Sinaliza quando a tela precisa ser atualizada

### Múltiplos Tedax e Bancadas

O número de tedax e bancadas varia conforme a dificuldade escolhida (configurado em `src/fases/fases.c`):

Cada tedax pode trabalhar em paralelo em um módulo diferente, e cada tedax precisa ocupar uma bancada livre para desarmar um módulo. Com mais recursos disponíveis, você pode processar múltiplos módulos simultaneamente.

**Sistema de Fila de Espera:**
- Cada tedax pode ter no máximo **1 módulo em espera** quando estiver ocupado
- Quando um tedax termina de processar um módulo, ele automaticamente pega o próximo da fila (se houver)
- Módulos na fila permanecem como `PENDENTE` até serem processados

**Sistema de Espera de Bancadas:**
- Se um tedax for designado para uma bancada ocupada, ele entra em estado `ESPERANDO`
- O tedax aguarda especificamente por aquela bancada até ela ficar livre
- Quando a bancada fica livre, o tedax em espera é automaticamente atribuído a ela

### Sistema de Configuração de Fases

Todas as configurações das dificuldades estão centralizadas no arquivo `src/fases/fases.c`:

Para modificar qualquer configuração, edite apenas o arquivo `fases.c` e recompile.

## Notas

- **Mural de Módulos Pendentes**: Implementado na thread `thread_mural`
- **Exibição de Informações**: Implementado na thread `thread_exibicao`
- **Coordenador (Jogador)**: Implementado na thread `thread_coordenador`
- **Tedax**: Implementado nas threads `thread_tedax` (uma por tedax)
- **Configurações**: Centralizadas em `src/fases/fases.c` para fácil modificação
- **Áudio**: Implementado em `src/audio/audio.c` com suporte opcional a SDL2_mixer
