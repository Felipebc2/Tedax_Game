#ifndef GAME_H
#define GAME_H

// Estados possíveis de um módulo
typedef enum {
    MOD_PENDENTE,
    MOD_EM_EXECUCAO,
    MOD_RESOLVIDO
} EstadoModulo;

// Cores possíveis do botão
typedef enum {
    COR_VERMELHO,
    COR_VERDE,
    COR_AZUL
} CorBotao;

// Estados possíveis do tedax
typedef enum {
    TEDAX_LIVRE,
    TEDAX_OCUPADO
} EstadoTedax;

// Dificuldades do jogo
typedef enum {
    DIFICULDADE_FACIL,
    DIFICULDADE_MEDIO,
    DIFICULDADE_DIFICIL
} Dificuldade;

// Estrutura que representa um módulo da bomba
typedef struct {
    int id;
    CorBotao cor;
    int tempo_total;            // tempo necessário para desarmar (em segundos)
    int tempo_restante;         // tempo restante quando estiver em execução
    EstadoModulo estado;
    
    char instrucao_correta[4];  // "p", "pp" ou "ppp"
    char instrucao_digitada[16]; // o que o jogador enviou para este módulo
} Modulo;

// Estrutura que representa um tedax
typedef struct {
    int id;                     // sempre 1 nesta versão
    EstadoTedax estado;
    int modulo_atual;           // índice do módulo que está desarmando, ou -1 se livre
} Tedax;

// Estado geral do jogo
typedef struct {
    Dificuldade dificuldade;    // dificuldade escolhida
    int tempo_total_partida;    // tempo total da partida em segundos
    int tempo_restante;         // tempo restante da partida
    
    Modulo modulos[100];        // fila de módulos
    int qtd_modulos;            // quantidade de módulos criados
    int proximo_id_modulo;      // próximo ID a ser atribuído
    int modulos_necessarios;    // número de módulos necessários para vencer
    
    Tedax tedax;                // único tedax
    
    // Controle de geração de módulos
    int ticks_desde_ultimo_modulo; // ticks desde o último módulo gerado
    int intervalo_geracao;      // intervalo entre gerações (em ticks)
    int max_modulos;            // máximo de módulos a gerar (igual a modulos_necessarios)
} GameState;

// Funções do jogo

// Inicializa o estado do jogo com a dificuldade escolhida
void inicializar_jogo(GameState *g, Dificuldade dificuldade);

// FUTURO: Esta função será uma thread (Mural de Módulos Pendentes)
// Gera um novo módulo e adiciona ao jogo
void gerar_novo_modulo(GameState *g);

// FUTURO: Esta função será parte da thread do Mural
// Atualiza o mural, gerando novos módulos conforme necessário
void atualizar_mural(GameState *g);

// FUTURO: Esta função será uma thread (Tedax)
// Atualiza o estado do tedax e do módulo em execução
void atualizar_tedax(GameState *g);

// Verifica se todos os módulos foram resolvidos
int todos_modulos_resolvidos(const GameState *g);

// Conta quantos módulos foram resolvidos
int contar_modulos_resolvidos(const GameState *g);

// Verifica se há módulos pendentes
int tem_modulos_pendentes(const GameState *g);

// Retorna o nome da cor como string
const char* nome_cor(CorBotao cor);

// Retorna o estado do módulo como string
const char* nome_estado_modulo(EstadoModulo estado);

// Retorna o nome da dificuldade como string
const char* nome_dificuldade(Dificuldade dificuldade);

#endif // GAME_H

