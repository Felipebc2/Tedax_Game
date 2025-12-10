#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#include "game.h"
#include "ui.h"

// FUTURO: Esta função será uma thread (Coordenador/Jogador)
// Processa a entrada do teclado e atualiza o buffer de instrução
void processar_entrada(GameState *g, char *buffer_instrucao, int *buffer_len) {
    int ch = getch();
    
    if (ch == ERR) {
        return; // Nenhuma tecla pressionada neste tick
    }
    
    // Processar tecla 'p' para adicionar aperto
    if (ch == 'p' || ch == 'P') {
        if (*buffer_len < 3) { // Máximo de 3 'p'
            buffer_instrucao[*buffer_len] = 'p';
            (*buffer_len)++;
            buffer_instrucao[*buffer_len] = '\0';
        }
    }
    // Processar BACKSPACE
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
        if (*buffer_len > 0) {
            (*buffer_len)--;
            buffer_instrucao[*buffer_len] = '\0';
        }
    }
    // Processar ENTER para enviar instrução
    else if (ch == '\n' || ch == '\r') {
        // Verificar se o tedax está livre
        if (g->tedax.estado == TEDAX_LIVRE) {
            // Procurar o primeiro módulo pendente
            int modulo_encontrado = -1;
            for (int i = 0; i < g->qtd_modulos; i++) {
                if (g->modulos[i].estado == MOD_PENDENTE) {
                    modulo_encontrado = i;
                    break;
                }
            }
            
            if (modulo_encontrado >= 0) {
                // Designar módulo para o tedax
                Modulo *mod = &g->modulos[modulo_encontrado];
                
                // Copiar instrução digitada
                strncpy(mod->instrucao_digitada, buffer_instrucao, 15);
                mod->instrucao_digitada[15] = '\0';
                
                // Mudar estado do módulo
                mod->estado = MOD_EM_EXECUCAO;
                mod->tempo_restante = mod->tempo_total;
                
                // Ocupar tedax
                g->tedax.estado = TEDAX_OCUPADO;
                g->tedax.modulo_atual = modulo_encontrado;
                
                // Limpar buffer
                *buffer_len = 0;
                buffer_instrucao[0] = '\0';
            }
            // Se não há módulos pendentes, ignorar ENTER
        }
        // Se tedax está ocupado, ignorar ENTER
    }
    // Tecla 'q' para sair (opcional, para debug)
    else if (ch == 'q' || ch == 'Q') {
        g->tempo_restante = 0; // Forçar fim do jogo
    }
}

int main(void) {
    GameState g;
    char buffer_instrucao[16] = "";
    int buffer_len = 0;
    
    // Inicializar ncurses
    inicializar_ncurses();
    
    // Inicializar cores (se disponível)
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Tempo restante
        init_pair(2, COLOR_GREEN, COLOR_BLACK);   // Livre/Sucesso
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Ocupado/Atenção
    }
    
    // Mostrar menu inicial e obter dificuldade
    int dificuldade_escolhida = mostrar_menu_inicial();
    if (dificuldade_escolhida == -1) {
        finalizar_ncurses();
        printf("Jogo encerrado.\n");
        return 0;
    }
    
    // Inicializar jogo com a dificuldade escolhida
    inicializar_jogo(&g, (Dificuldade)dificuldade_escolhida);
    
    // Contador de ticks (5 ticks = 1 segundo, pois tick = 0.2s)
    int tick_count = 0;
    
    // Loop principal do jogo
    while (g.tempo_restante > 0) {
        // 1) FUTURO: Thread do Coordenador - ler tecla e atualizar buffer/ações
        processar_entrada(&g, buffer_instrucao, &buffer_len);
        
        // 2) FUTURO: Thread do Mural - gerar módulos de tempos em tempos
        atualizar_mural(&g);
        
        // 3) FUTURO: Thread do Tedax - atualizar execução do tedax
        atualizar_tedax(&g);
        
        // 4) FUTURO: Thread de Exibição - redesenhar a tela
        desenhar_tela(&g, buffer_instrucao);
        
        // 5) Checar condição de vitória
        if (todos_modulos_resolvidos(&g) && g.qtd_modulos > 0) {
            mostrar_mensagem_vitoria();
            break;
        }
        
        // 6) Esperar 0.2 segundos (tick de 200ms - 5 ticks por segundo)
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 200000000L; // 200ms em nanossegundos
        nanosleep(&ts, NULL);
        
        // Decrementar tempos apenas a cada 5 ticks (1 segundo completo)
        tick_count++;
        if (tick_count >= 5) {
            g.tempo_restante--;
            
            // Decrementar tempo do módulo em execução também
            if (g.tedax.estado == TEDAX_OCUPADO && g.tedax.modulo_atual >= 0) {
                g.modulos[g.tedax.modulo_atual].tempo_restante--;
            }
            
            tick_count = 0;
        }
    }
    
    // Verificar se perdeu (tempo acabou e ainda há módulos não resolvidos)
    if (!todos_modulos_resolvidos(&g) && g.qtd_modulos > 0) {
        mostrar_mensagem_derrota();
    } else if (g.qtd_modulos == 0) {
        // Caso especial: tempo acabou mas nenhum módulo foi gerado
        clear();
        mvprintw(LINES / 2, COLS / 2 - 15, "Tempo esgotado - Nenhum modulo gerado");
        refresh();
        sleep(2);
    }
    
    // Finalizar ncurses
    finalizar_ncurses();
    
    printf("Fim da execucao\n");
    return 0;
}

/*
 * Compilar com:
 * gcc -Wall -Wextra -std=c11 main.c game.c ui.c -o jogo -lncurses
 * 
 * Ou usar o Makefile:
 * make
 */

