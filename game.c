#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Inicializa o estado do jogo com a dificuldade escolhida
void inicializar_jogo(GameState *g, Dificuldade dificuldade) {
    g->dificuldade = dificuldade;
    g->tempo_total_partida = 120;  // 120 segundos de partida
    g->tempo_restante = g->tempo_total_partida;
    g->qtd_modulos = 0;
    g->proximo_id_modulo = 1;
    
    // Inicializar tedax
    g->tedax.id = 1;
    g->tedax.estado = TEDAX_LIVRE;
    g->tedax.modulo_atual = -1;
    
    // Inicializar semente do rand (deve ser antes de usar rand())
    srand(time(NULL));
    
    // Configuração baseada na dificuldade
    switch (dificuldade) {
        case DIFICULDADE_FACIL:
            g->modulos_necessarios = 4;
            g->intervalo_geracao = 100; // 20 segundos (100 ticks * 0.2s)
            break;
        case DIFICULDADE_MEDIO:
            g->modulos_necessarios = 8;
            g->intervalo_geracao = 75; // 15 segundos (75 ticks * 0.2s)
            break;
        case DIFICULDADE_DIFICIL:
            g->modulos_necessarios = 12;
            g->intervalo_geracao = 50; // 10 segundos (50 ticks * 0.2s)
            break;
    }
    
    g->max_modulos = g->modulos_necessarios; // máximo = necessário para vencer
    g->ticks_desde_ultimo_modulo = 0;
    
    // Gerar primeiro módulo imediatamente
    gerar_novo_modulo(g);
}

// FUTURO: Esta função será uma thread (Mural de Módulos Pendentes)
// Gera um novo módulo e adiciona ao jogo
void gerar_novo_modulo(GameState *g) {
    if (g->qtd_modulos >= 100) {
        return; // Limite máximo de módulos atingido
    }
    
    Modulo *novo = &g->modulos[g->qtd_modulos];
    
    // Atribuir ID
    novo->id = g->proximo_id_modulo++;
    
    // Sortear cor (0 = Vermelho, 1 = Verde, 2 = Azul)
    int cor_aleatoria = rand() % 3;
    novo->cor = (CorBotao)cor_aleatoria;
    
    // Definir tempo total (entre 3 e 8 segundos)
    novo->tempo_total = 3 + (rand() % 6);
    novo->tempo_restante = novo->tempo_total;
    
    // Definir instrução correta baseada na cor
    switch (novo->cor) {
        case COR_VERMELHO:
            strcpy(novo->instrucao_correta, "p");
            break;
        case COR_VERDE:
            strcpy(novo->instrucao_correta, "pp");
            break;
        case COR_AZUL:
            strcpy(novo->instrucao_correta, "ppp");
            break;
    }
    
    // Limpar instrução digitada
    novo->instrucao_digitada[0] = '\0';
    
    // Estado inicial: pendente
    novo->estado = MOD_PENDENTE;
    
    g->qtd_modulos++;
}

// FUTURO: Esta função será parte da thread do Mural
// Atualiza o mural, gerando novos módulos conforme necessário
// NOTA: Agora processamos 5 ticks por segundo (0.2s), então intervalo_geracao está em ticks
void atualizar_mural(GameState *g) {
    // Se já gerou o máximo de módulos (igual a modulos_necessarios), não gera mais
    if (g->qtd_modulos >= g->max_modulos) {
        return;
    }
    
    g->ticks_desde_ultimo_modulo++;
    
    // Gerar novo módulo a cada intervalo_geracao ticks
    // O intervalo é definido pela dificuldade (fixo, não aleatório)
    if (g->ticks_desde_ultimo_modulo >= g->intervalo_geracao) {
        gerar_novo_modulo(g);
        g->ticks_desde_ultimo_modulo = 0;
    }
}

// FUTURO: Esta função será uma thread (Tedax)
// Atualiza o estado do tedax e do módulo em execução
// NOTA: O decremento do tempo_restante é feito no main.c a cada segundo completo
// Esta função apenas verifica se o tempo acabou e processa o resultado
void atualizar_tedax(GameState *g) {
    // Se o tedax está ocupado, verificar se o tempo do módulo acabou
    if (g->tedax.estado == TEDAX_OCUPADO && g->tedax.modulo_atual >= 0) {
        Modulo *mod = &g->modulos[g->tedax.modulo_atual];
        
        // Quando o tempo acabar, verificar se a instrução estava correta
        if (mod->tempo_restante <= 0) {
            // Comparar instrução digitada com a correta
            if (strcmp(mod->instrucao_digitada, mod->instrucao_correta) == 0) {
                // Instrução correta: módulo resolvido
                mod->estado = MOD_RESOLVIDO;
            } else {
                // Instrução incorreta: módulo volta para o mural
                mod->estado = MOD_PENDENTE;
                mod->tempo_restante = mod->tempo_total; // Resetar tempo
                mod->instrucao_digitada[0] = '\0';      // Limpar instrução
            }
            
            // Liberar tedax em ambos os casos
            g->tedax.estado = TEDAX_LIVRE;
            g->tedax.modulo_atual = -1;
            
            // Se não há módulos pendentes e ainda não gerou o máximo, gerar um novo imediatamente
            if (!tem_modulos_pendentes(g) && g->qtd_modulos < g->max_modulos) {
                gerar_novo_modulo(g);
                g->ticks_desde_ultimo_modulo = 0; // Resetar contador de ticks
            }
        }
    }
}

// Conta quantos módulos foram resolvidos
int contar_modulos_resolvidos(const GameState *g) {
    int resolvidos = 0;
    for (int i = 0; i < g->qtd_modulos; i++) {
        if (g->modulos[i].estado == MOD_RESOLVIDO) {
            resolvidos++;
        }
    }
    return resolvidos;
}

// Verifica se há módulos pendentes
int tem_modulos_pendentes(const GameState *g) {
    for (int i = 0; i < g->qtd_modulos; i++) {
        if (g->modulos[i].estado == MOD_PENDENTE) {
            return 1; // Há pelo menos um módulo pendente
        }
    }
    return 0; // Não há módulos pendentes
}

// Verifica se todos os módulos necessários foram resolvidos
int todos_modulos_resolvidos(const GameState *g) {
    int resolvidos = contar_modulos_resolvidos(g);
    // Vitória quando resolver pelo menos o número necessário de módulos
    return (resolvidos >= g->modulos_necessarios);
}

// Retorna o nome da cor como string
const char* nome_cor(CorBotao cor) {
    switch (cor) {
        case COR_VERMELHO:
            return "Vermelho";
        case COR_VERDE:
            return "Verde";
        case COR_AZUL:
            return "Azul";
        default:
            return "Desconhecida";
    }
}

// Retorna o estado do módulo como string
const char* nome_estado_modulo(EstadoModulo estado) {
    switch (estado) {
        case MOD_PENDENTE:
            return "PENDENTE";
        case MOD_EM_EXECUCAO:
            return "EM_EXECUCAO";
        case MOD_RESOLVIDO:
            return "RESOLVIDO";
        default:
            return "DESCONHECIDO";
    }
}

// Retorna o nome da dificuldade como string
const char* nome_dificuldade(Dificuldade dificuldade) {
    switch (dificuldade) {
        case DIFICULDADE_FACIL:
            return "FACIL";
        case DIFICULDADE_MEDIO:
            return "MEDIO";
        case DIFICULDADE_DIFICIL:
            return "DIFICIL";
        default:
            return "DESCONHECIDA";
    }
}

