#ifndef UI_H
#define UI_H

#include "game.h"

// FUTURO: Esta função será uma thread (Exibição de Informações)
// Desenha toda a interface do jogo na tela
void desenhar_tela(const GameState *g, const char *buffer_instrucao);

// Inicializa o ncurses
void inicializar_ncurses(void);

// Finaliza o ncurses
void finalizar_ncurses(void);

// Mostra mensagem de vitória
void mostrar_mensagem_vitoria(void);

// Mostra mensagem de derrota
void mostrar_mensagem_derrota(void);

// Mostra menu inicial e retorna a dificuldade escolhida (ou -1 para sair)
// Retorna DIFICULDADE_FACIL, DIFICULDADE_MEDIO, DIFICULDADE_DIFICIL, ou -1 para sair
int mostrar_menu_inicial(void);

#endif // UI_H

