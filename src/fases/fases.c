#include "fases.h"
#include "../game/game.h"

// Config por dificuldade
static const ConfigFase config_fases[] = {
    // FACIL
    {
        .num_tedax = 2,
        .num_bancadas = 1,
        .modulos_necessarios = 6,
        .intervalo_geracao = 130,        // 26 segundos (130 ticks * 0.2s)
        .modulos_iniciais = 2,
        .tempo_total_partida = 120,      // em Segundos
        .tempo_minimo_execucao = 3,     // 3 segundos mínimo
        .tempo_variacao_execucao = 8    // Variação de 0 a 8 segundos
    },
    
    // MEDIO
    {
        .num_tedax = 3,
        .num_bancadas = 2,
        .modulos_necessarios = 10,
        .intervalo_geracao = 100,        // 20 segundos (100 ticks * 0.2s)
        .modulos_iniciais = 3,
        .tempo_total_partida = 180,     // em Segundos
        .tempo_minimo_execucao = 5,     // 5 segundos mínimo (+2 do fácil)
        .tempo_variacao_execucao = 15   // Variação de 0 a 15 segundos (50% mais que fácil)
    },
    
    // DIFICIL
    {
        .num_tedax = 4,
        .num_bancadas = 3,
        .modulos_necessarios = 15,
        .intervalo_geracao = 70,        // 14 segundos (70 ticks * 0.2s)
        .modulos_iniciais = 5,
        .tempo_total_partida = 210,     // em Segundos
        .tempo_minimo_execucao = 9,     // 9 segundos mínimo (+4 do fácil)
        .tempo_variacao_execucao = 20   // Variação de 0 a 20 segundos (100% mais que fácil)
    }
};

// Get baseado na dificuldade
const ConfigFase* obter_config_fase(Dificuldade dificuldade) {
    int index = 0;
    
    switch (dificuldade) {
        case DIFICULDADE_FACIL:
            index = 0;
            break;
        case DIFICULDADE_MEDIO:
            index = 1;
            break;
        case DIFICULDADE_DIFICIL:
            index = 2;
            break;
        default:
            index = 0;
            break;
    }
    
    return &config_fases[index];
}

