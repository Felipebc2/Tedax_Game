#ifndef FASES_H
#define FASES_H

#include "../game/game.h"

typedef struct {
    int num_tedax;
    int num_bancadas;
    int modulos_necessarios;
    int intervalo_geracao;
    int modulos_iniciais;
    int tempo_total_partida;
    int tempo_minimo_execucao;
    int tempo_variacao_execucao;
} ConfigFase;

const ConfigFase* obter_config_fase(Dificuldade dificuldade);

#endif