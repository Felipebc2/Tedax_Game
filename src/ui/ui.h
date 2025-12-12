#ifndef UI_H
#define UI_H

#include "../game/game.h"

void desenhar_tela(const GameState *g, const char *buffer_instrucao);
void inicializar_ncurses(void);
void finalizar_ncurses(void);
void mostrar_mensagem_vitoria(void);
void mostrar_mensagem_derrota(void);

int mostrar_menu_principal(void);
int mostrar_menu_dificuldades(void);
int mostrar_menu_pos_jogo(int vitoria, int tempo_restante, int erros);

#endif