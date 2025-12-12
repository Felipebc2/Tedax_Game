#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <ncurses.h>
#include "../game/game.h"
#include "../ui/ui.h"
#include "../audio/audio.h"
#include "../fases/fases.h"

// Buffer de instrução global (compartilhado entre threads)
// Suporta comandos do formato T1B1M1:ppp
char buffer_instrucao_global[64] = "";
int audio_disponivel_global = 0;

int main(void) {
    // Inicializar áudio (Música começa desligada)
    audio_disponivel_global = inicializar_audio();
    
    GameState g;
    
    while (1) {
        inicializar_ncurses();
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_CYAN, COLOR_BLACK);
            init_pair(2, COLOR_GREEN, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_RED, COLOR_BLACK);
            init_pair(5, COLOR_BLUE, COLOR_BLACK);
            init_pair(6, COLOR_WHITE, COLOR_BLACK);
            init_pair(7, COLOR_BLACK, COLOR_WHITE);
        }
        
        int modo_escolhido = mostrar_menu_principal();
        if (modo_escolhido == -1) {
            finalizar_ncurses();
            printf("Jogo encerrado.\n");
            return 0;
        }
        
        // Se escolheu Classico, mostrar menu de dificuldades do modo classico
        if (modo_escolhido == 0) {
            int dificuldade_menu = mostrar_menu_dificuldades();
            if (dificuldade_menu == -1) {
                finalizar_ncurses();
                continue;
            }
            
            Dificuldade dificuldade_escolhida;
            switch (dificuldade_menu) {
                case 0: // Fácil
                    dificuldade_escolhida = DIFICULDADE_FACIL;
                    break;
                case 1: // Médio
                    dificuldade_escolhida = DIFICULDADE_MEDIO;
                    break;
                case 2: // Difícil
                    dificuldade_escolhida = DIFICULDADE_DIFICIL;
                    break;
                default:
                    dificuldade_escolhida = DIFICULDADE_FACIL;
                    break;
            }
    
            const ConfigFase *config = obter_config_fase(dificuldade_escolhida);
            int num_tedax = config->num_tedax;
            int num_bancadas = config->num_bancadas;
            
            // Finalizar ncurses temporário (será reinicializado nas threads)
            finalizar_ncurses();
            // Parar música do menu e tocar música da fase baseada na dificuldade
            parar_musica();
            definir_dificuldade_musica(dificuldade_escolhida == DIFICULDADE_MEDIO);
            
            const char* musica_fase = NULL;
            switch (dificuldade_escolhida) {
                case DIFICULDADE_FACIL:
                    musica_fase = "sounds/Fase_1.mp3";
                    break;
                case DIFICULDADE_MEDIO:
                    musica_fase = "sounds/Fase_2.mp3";
                    break;
                case DIFICULDADE_DIFICIL:
                    musica_fase = "sounds/Fase_3.mp3";
                    break;
            }
            if (musica_fase && audio_disponivel_global) {
                tocar_musica(musica_fase);
            }
            
            inicializar_jogo(&g, dificuldade_escolhida, num_tedax, num_bancadas);
    
    // Criar threads
    pthread_t thread_mural_id;
    pthread_t thread_exibicao_id;
    pthread_t thread_tedax_ids[5];
    pthread_t thread_coordenador_id;
    
    // Thread do Mural
    pthread_create(&thread_mural_id, NULL, thread_mural, &g);
    
    // Thread de Exibição
    pthread_create(&thread_exibicao_id, NULL, thread_exibicao, &g);
    
    // Threads dos Tedax (até 5)
    for (int i = 0; i < g.qtd_tedax; i++) {
        typedef struct {
            GameState *g;
            int tedax_id;
        } TedaxArgs;
        
        TedaxArgs *args = malloc(sizeof(TedaxArgs));
        args->g = &g;
        args->tedax_id = i;
        pthread_create(&thread_tedax_ids[i], NULL, thread_tedax, args);
    }
    
    // Thread do Coordenador
    pthread_create(&thread_coordenador_id, NULL, thread_coordenador, &g);
    
    // Thread principal: controla o tempo e verifica condições de vitória/derrota
    int tick_count = 0;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 200000000L; // 0.2 segundos
    
    while (g.jogo_rodando && !g.jogo_terminou) {
        nanosleep(&ts, NULL);
        
        tick_count++;
        if (tick_count >= 5) { // 1 segundo
            pthread_mutex_lock(&g.mutex_jogo);
            g.tempo_restante--;
            
            if (todos_modulos_resolvidos(&g) && g.qtd_modulos > 0) {
                g.jogo_terminou = 1;
                g.jogo_rodando = 0;
                pthread_mutex_unlock(&g.mutex_jogo);
                break;
            }
            
            if (g.tempo_restante <= 0) {
                g.jogo_terminou = 1;
                g.jogo_rodando = 0;
                pthread_mutex_unlock(&g.mutex_jogo);
                break;
            }
            
            pthread_mutex_unlock(&g.mutex_jogo);
            tick_count = 0;
        }
    }
    
    pthread_join(thread_mural_id, NULL);
    pthread_join(thread_exibicao_id, NULL);
    for (int i = 0; i < g.qtd_tedax; i++) {
        pthread_join(thread_tedax_ids[i], NULL);
    }
    pthread_join(thread_coordenador_id, NULL);
    
    finalizar_ncurses();
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    timeout(-1);
    curs_set(0);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_RED, COLOR_BLACK);
        init_pair(5, COLOR_BLUE, COLOR_BLACK);
        init_pair(6, COLOR_WHITE, COLOR_BLACK);
        init_pair(7, COLOR_BLACK, COLOR_WHITE);
    }
    clear();
    refresh();
    
            pthread_mutex_lock(&g.mutex_jogo);
            int vitoria = 0;
            int tempo_restante_final = g.tempo_restante;
            int erros_final = g.erros_cometidos;
            if (todos_modulos_resolvidos(&g) && g.qtd_modulos > 0) {
                vitoria = 1;
            } else if (g.qtd_modulos > 0) {
                vitoria = 0;
            } else {
                clear();
                mvprintw(LINES / 2, COLS / 2 - 15, "Tempo esgotado - Nenhum modulo gerado");
                refresh();
                sleep(2);
                finalizar_ncurses();
                continue;
            }
            pthread_mutex_unlock(&g.mutex_jogo);
            
            parar_musica();
            if (audio_disponivel_global) {
                if (vitoria) {
                    tocar_sound_effect("sounds/win.mp3");
                } else {
                    tocar_sound_effect("sounds/failed.mp3");
                }
            }
            
            int opcao = mostrar_menu_pos_jogo(vitoria, tempo_restante_final, erros_final);
            
            if (audio_disponivel_global) {
                while (musica_tocando()) {
                    struct timespec ts;
                    ts.tv_sec = 0;
                    ts.tv_nsec = 100000000L; // 0.1 segundos
                    nanosleep(&ts, NULL);
                }
                definir_dificuldade_musica(0);
                tocar_musica("sounds/Menu.mp3");
            }
            
            finalizar_ncurses();
            
            if (opcao == 'q' || opcao == 'Q') {
                printf("Jogo encerrado.\n");
                return 0;
            } else if (opcao == 'r' || opcao == 'R') {
                continue;
            }
        } else {
            finalizar_ncurses();
        }
        
        parar_musica();
    }
    
    parar_musica();
    finalizar_audio();
    
    return 0;
    finalizar_jogo(&g);
    
    printf("Fim da execucao\n");
    return 0;
}