#ifndef GAME_H
#define GAME_H

#include <pthread.h>
#include <semaphore.h>

typedef enum {
    MOD_PENDENTE,
    MOD_EM_EXECUCAO,
    MOD_RESOLVIDO
} EstadoModulo;

typedef enum {
    TIPO_BOTAO,
    TIPO_SENHA,
    TIPO_FIOS
} TipoModulo;

typedef enum {
    COR_VERMELHO,
    COR_VERDE,
    COR_AZUL
} CorBotao;

typedef enum {
    COR_FIO_VERMELHO = 0,
    COR_FIO_VERDE = 1,
    COR_FIO_AZUL = 2,
    COR_FIO_AMARELO = 3,
    COR_FIO_BRANCO = 4,
    COR_FIO_PRETO = 5
} CorFio;

typedef enum {
    TEDAX_LIVRE,
    TEDAX_OCUPADO,
    TEDAX_ESPERANDO
} EstadoTedax;

typedef enum {
    BANCADA_LIVRE,
    BANCADA_OCUPADA
} EstadoBancada;

typedef enum {
    DIFICULDADE_FACIL,
    DIFICULDADE_MEDIO,
    DIFICULDADE_DIFICIL
} Dificuldade;

typedef struct {
    CorBotao cor;
} DadosBotao;

typedef struct {
    char hash[32];
    char senha_correta[16];
    char mapeamento[256];
} DadosSenha;

typedef struct {
    char sequencia[32];
    int padrao;
    char instrucao_correta[16];
} DadosFios;

typedef union {
    DadosBotao botao;
    DadosSenha senha;
    DadosFios fios;
} DadosModulo;

typedef struct {
    int id;
    TipoModulo tipo;
    int tempo_total;
    int tempo_restante;
    EstadoModulo estado;
    
    DadosModulo dados;
    char instrucao_correta[32];
    char instrucao_digitada[32];
    
    int tempo_desde_resolvido;
} Modulo;

typedef struct {
    int id;
    EstadoTedax estado;
    int modulo_atual;
    int bancada_atual;
    pthread_t thread_id;
    
    int fila_modulos[1];
    int qtd_fila;
} Tedax;

typedef struct {
    int id;
    EstadoBancada estado;
    int tedax_ocupando;
    int tedax_esperando;
} Bancada;

typedef struct {
    Dificuldade dificuldade;
    int tempo_total_partida;
    int tempo_restante;
    
    Modulo modulos[100];
    int qtd_modulos;
    int proximo_id_modulo;
    int modulos_necessarios;
    
    Tedax tedax[5];
    Bancada bancadas[5];
    int qtd_tedax;
    int qtd_bancadas;
    
    int ticks_desde_ultimo_modulo;
    int intervalo_geracao;
    int max_modulos;
    
    int jogo_rodando;
    int jogo_terminou;
    
    pthread_mutex_t mutex_jogo;
    pthread_cond_t cond_modulo_disponivel;
    pthread_cond_t cond_bancada_disponivel;
    pthread_cond_t cond_tela_atualizada;
    
    char mensagem_erro[64];
    int erros_cometidos;
} GameState;

void inicializar_jogo(GameState *g, Dificuldade dificuldade, int num_tedax, int num_bancadas);
void finalizar_jogo(GameState *g);
void gerar_novo_modulo(GameState *g);

int todos_modulos_resolvidos(const GameState *g);
int contar_modulos_resolvidos(const GameState *g);
int tem_modulos_pendentes(const GameState *g);

const char* nome_cor(CorBotao cor);
const char* nome_estado_modulo(EstadoModulo estado);
const char* nome_dificuldade(Dificuldade dificuldade);

void* thread_mural(void* arg);
void* thread_exibicao(void* arg);
void* thread_tedax(void* arg);
void* thread_coordenador(void* arg);

int processar_comando(const char* buffer, GameState *g, 
                      int *tedax_idx, int *bancada_idx, int *modulo_idx, 
                      char *instrucao);

#endif