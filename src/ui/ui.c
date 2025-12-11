#define _POSIX_C_SOURCE 200809L
#include "ui.h"
#include "../game/game.h"
#include "../audio/audio.h"
#include <ncurses.h>
#include <string.h>
#include <time.h>

// Inicializa o ncurses
void inicializar_ncurses(void) {
    initscr();                  // Inicializa a tela
    cbreak();                   // Desativa buffer de linha (CTRL+Z e CTRL+C ainda funcionam)
    noecho();                   // Não exibe o que é digitado
    keypad(stdscr, TRUE);       // Habilita teclas especiais
    curs_set(0);                // Esconde o cursor
    nodelay(stdscr, TRUE);      // getch() não bloqueia
    timeout(0);                 // getch() retorna ERR imediatamente se não houver entrada
}

// Finaliza o ncurses
void finalizar_ncurses(void) {
    keypad(stdscr, FALSE);
    echo();
    curs_set(1);
    endwin();
}

// Desenha toda a interface do jogo na tela
void desenhar_tela(const GameState *g, const char *buffer_instrucao) {
    clear();
    
    int linha = 0;
    int cores_disponiveis = has_colors();
    
    // Título do jogo
    attron(A_BOLD);
    mvprintw(linha++, 0, "=== KEEP SOLVING AND NOBODY EXPLODES ===");
    attroff(A_BOLD);
    linha++;
    
    // Tempo restante e dificuldade
    if (cores_disponiveis) {
        attron(A_BOLD | COLOR_PAIR(1));
    } else {
        attron(A_BOLD);
    }
    mvprintw(linha++, 0, "Dificuldade: %s | Tempo Restante: %d segundos | Modulos: %d/%d resolvidos", 
             nome_dificuldade(g->dificuldade), g->tempo_restante, 
             contar_modulos_resolvidos(g), g->modulos_necessarios);
    if (cores_disponiveis) {
        attroff(A_BOLD | COLOR_PAIR(1));
    } else {
        attroff(A_BOLD);
    }
    linha++;
    
    // Estado dos Tedax
    mvprintw(linha++, 0, "--- TEDAX (%d total) ---", g->qtd_tedax);
    for (int i = 0; i < g->qtd_tedax; i++) {
        const Tedax *t = &g->tedax[i];
        if (t->estado == TEDAX_LIVRE) {
            if (cores_disponiveis) {
                attron(COLOR_PAIR(2)); // Verde
            }
            mvprintw(linha++, 0, "  Tedax %d: LIVRE", t->id);
            if (cores_disponiveis) {
                attroff(COLOR_PAIR(2));
            }
        } else if (t->estado == TEDAX_ESPERANDO) {
            if (cores_disponiveis) {
                attron(COLOR_PAIR(3)); // Amarelo
            }
            mvprintw(linha++, 0, "  Tedax %d: ESPERANDO (Bancada %d)", t->id, 
                     t->bancada_atual >= 0 ? g->bancadas[t->bancada_atual].id : 0);
            if (t->modulo_atual >= 0) {
                const Modulo *mod = &g->modulos[t->modulo_atual];
                mvprintw(linha++, 0, "    Aguardando para M%d Botao %s",
                         mod->id, nome_cor(mod->cor));
            }
            if (cores_disponiveis) {
                attroff(COLOR_PAIR(3));
            }
        } else {
            if (cores_disponiveis) {
                attron(COLOR_PAIR(3)); // Amarelo/Vermelho
            }
            mvprintw(linha++, 0, "  Tedax %d: OCUPADO", t->id);
            if (t->modulo_atual >= 0) {
                const Modulo *mod = &g->modulos[t->modulo_atual];
                mvprintw(linha++, 0, "    Desarmando M%d Botao %s - Tempo restante: %d segundos",
                         mod->id, nome_cor(mod->cor), mod->tempo_restante);
            }
            // Mostrar módulo em espera (máximo 1)
            if (t->qtd_fila > 0 && t->fila_modulos[0] >= 0 && t->fila_modulos[0] < g->qtd_modulos) {
                const Modulo *mod_fila = &g->modulos[t->fila_modulos[0]];
                mvprintw(linha++, 0, "    Em espera: M%d Botao %s",
                         mod_fila->id, nome_cor(mod_fila->cor));
            }
            if (cores_disponiveis) {
                attroff(COLOR_PAIR(3));
            }
        }
    }
    linha++;
    
    // Estado das Bancadas
    mvprintw(linha++, 0, "--- BANCADAS (%d total) ---", g->qtd_bancadas);
    for (int i = 0; i < g->qtd_bancadas; i++) {
        const Bancada *b = &g->bancadas[i];
        if (b->estado == BANCADA_LIVRE) {
            if (cores_disponiveis) {
                attron(COLOR_PAIR(2));
            }
            mvprintw(linha++, 0, "  Bancada %d: LIVRE", b->id);
            if (cores_disponiveis) {
                attroff(COLOR_PAIR(2));
            }
        } else {
            if (cores_disponiveis) {
                attron(COLOR_PAIR(3));
            }
            mvprintw(linha++, 0, "  Bancada %d: OCUPADA (Tedax %d)", b->id, b->tedax_ocupando);
            if (b->tedax_esperando >= 0) {
                mvprintw(linha++, 0, "    Esperando: Tedax %d", b->tedax_esperando);
            }
            if (cores_disponiveis) {
                attroff(COLOR_PAIR(3));
            }
        }
    }
    linha++;
    
    // Lista de módulos
    mvprintw(linha++, 0, "--- MODULOS (%d total) ---", g->qtd_modulos);
    
    // Primeiro, contar quantos módulos resolvidos estão dentro do tempo limite de 20s
    int resolvidos_visiveis_20s = 0;
    for (int i = 0; i < g->qtd_modulos; i++) {
        const Modulo *mod = &g->modulos[i];
        if (mod->estado == MOD_RESOLVIDO && mod->tempo_desde_resolvido >= 0 && mod->tempo_desde_resolvido < 20) {
            resolvidos_visiveis_20s++;
        }
    }
    
    // Determinar tempo limite para remoção (10s se tiver 8-9 resolvidos, 20s caso contrário)
    int tempo_limite_remocao = (resolvidos_visiveis_20s >= 8) ? 10 : 20;
    
    int pendentes = 0, em_execucao = 0, resolvidos = 0;
    int modulos_exibidos = 0;
    int modulos_nao_exibidos = 0;
    
    for (int i = 0; i < g->qtd_modulos; i++) {
        const Modulo *mod = &g->modulos[i];
        
        // Filtrar módulos resolvidos que já passaram do tempo limite
        int deve_exibir = 1;
        if (mod->estado == MOD_RESOLVIDO) {
            if (mod->tempo_desde_resolvido < 0 || mod->tempo_desde_resolvido >= tempo_limite_remocao) {
                deve_exibir = 0; // Não exibir módulos resolvidos antigos
                modulos_nao_exibidos++;
            }
        }
        
        if (!deve_exibir) {
            continue; // Pular módulos resolvidos antigos
        }
        
        switch (mod->estado) {
            case MOD_PENDENTE:
                pendentes++;
                break;
            case MOD_EM_EXECUCAO:
                em_execucao++;
                break;
            case MOD_RESOLVIDO:
                resolvidos++;
                break;
        }
        
        // Mostrar detalhes do módulo no formato M[Indice] [Categoria] [Cor]
        const char* categoria = "Botao"; // Por enquanto só temos módulos de botão
        const char* cor_nome = nome_cor(mod->cor);
        
        if (mod->estado == MOD_EM_EXECUCAO) {
            mvprintw(linha, 0, "  M%d %s %s - %s", 
                     mod->id, categoria, cor_nome, nome_estado_modulo(mod->estado));
        } else if (mod->estado == MOD_RESOLVIDO) {
            mvprintw(linha, 0, "  M%d %s %s - %s", 
                     mod->id, categoria, cor_nome, nome_estado_modulo(mod->estado));
        } else {
            mvprintw(linha, 0, "  M%d %s %s - %s - Execucao: %d sec", 
                     mod->id, categoria, cor_nome, nome_estado_modulo(mod->estado),
                     mod->tempo_total);
        }
        
        linha++;
        modulos_exibidos++;
        
        // Limitar quantidade de módulos exibidos para não ultrapassar a tela
        if (linha >= LINES - 8) {
            // Contar quantos módulos ainda não foram processados
            int restantes = 0;
            for (int j = i + 1; j < g->qtd_modulos; j++) {
                const Modulo *mod_rest = &g->modulos[j];
                if (mod_rest->estado != MOD_RESOLVIDO || 
                    (mod_rest->tempo_desde_resolvido >= 0 && mod_rest->tempo_desde_resolvido < tempo_limite_remocao)) {
                    restantes++;
                }
            }
            if (restantes > 0) {
                mvprintw(linha++, 0, "  ... (mais %d modulos)", restantes);
            }
            break;
        }
    }
    
    // Se houver módulos não exibidos (resolvidos antigos), informar
    if (modulos_nao_exibidos > 0) {
        linha++;
        mvprintw(linha++, 0, "  (%d resolvidos removidos)", modulos_nao_exibidos);
    }
    
    linha++;
    

    
    // Área de entrada
    mvprintw(linha++, 0, "Comando: [%s]", buffer_instrucao);
    linha++;
    
    // Mostrar mensagem de erro se houver
    if (g->mensagem_erro[0] != '\0') {
        if (cores_disponiveis) {
            attron(A_BOLD | COLOR_PAIR(3)); // Bold e amarelo/vermelho
        } else {
            attron(A_BOLD);
        }
        mvprintw(linha++, 0, "%s", g->mensagem_erro);
        if (cores_disponiveis) {
            attroff(A_BOLD | COLOR_PAIR(3));
        } else {
            attroff(A_BOLD);
        }
        linha++;
    }
    
    refresh();
}

// Mostra menu pós-jogo (vitória ou derrota)
// Retorna: 'q' ou 'Q' para sair, 'r' ou 'R' para voltar ao menu
int mostrar_menu_pos_jogo(int vitoria, int tempo_restante, int erros) {
    clear();
    int cores_disponiveis = has_colors();
    
    if (vitoria) {
        if (cores_disponiveis) {
            attron(A_BOLD | COLOR_PAIR(2));
        } else {
            attron(A_BOLD);
        }
        mvprintw(LINES / 2 - 1, COLS / 2 - 15, "================================");
        mvprintw(LINES / 2, COLS / 2 - 15, "    BOMBA DESARMADA!");
        mvprintw(LINES / 2 + 1, COLS / 2 - 15, "    VITORIA!");
        mvprintw(LINES / 2 + 2, COLS / 2 - 15, "================================");
        if (cores_disponiveis) {
            attroff(A_BOLD | COLOR_PAIR(2));
        } else {
            attroff(A_BOLD);
        }
        
        // Mostrar estatísticas de vitória
        mvprintw(LINES / 2 + 4, COLS / 2 - 15, "Tempo Restante: %d segundos", tempo_restante);
        mvprintw(LINES / 2 + 5, COLS / 2 - 15, "Erros: %d", erros);
    } else {
        if (cores_disponiveis) {
            attron(A_BOLD | COLOR_PAIR(3));
        } else {
            attron(A_BOLD);
        }
        mvprintw(LINES / 2 - 1, COLS / 2 - 15, "================================");
        mvprintw(LINES / 2, COLS / 2 - 15, "    BOMBA EXPLODIU!");
        mvprintw(LINES / 2 + 1, COLS / 2 - 15, "    DERROTA!");
        mvprintw(LINES / 2 + 2, COLS / 2 - 15, "================================");
        if (cores_disponiveis) {
            attroff(A_BOLD | COLOR_PAIR(3));
        } else {
            attroff(A_BOLD);
        }
    }
    
    // Mostrar opções de menu (abaixo das estatísticas se vitória, ou abaixo da mensagem se derrota)
    int linha_opcoes = vitoria ? LINES / 2 + 7 : LINES / 2 + 4;
    mvprintw(linha_opcoes, COLS / 2 - 15, "Pressione R para voltar ao Menu");
    mvprintw(linha_opcoes + 1, COLS / 2 - 15, "Pressione Q para Sair");
    refresh();
    
    nodelay(stdscr, FALSE);
    timeout(-1); // Bloquear até receber entrada
    int ch;
    while (1) {
        ch = getch();
        if (ch == 'q' || ch == 'Q') {
            return 'q';
        } else if (ch == 'r' || ch == 'R') {
            return 'r';
        }
    }
}

// Mostra mensagem de vitória (mantida para compatibilidade)
void mostrar_mensagem_vitoria(void) {
    mostrar_menu_pos_jogo(1, 0, 0);
}

// Mostra mensagem de derrota (mantida para compatibilidade)
void mostrar_mensagem_derrota(void) {
    mostrar_menu_pos_jogo(0, 0, 0);
}

// Variável global para estado da música (mantém estado entre chamadas)
static int musica_ligada_global = 0;

// Declaração externa da flag de áudio (definida em main.c)
extern int audio_disponivel_global;
extern int audio_disponivel_global; // Declarada em main.c

// Mostra menu principal e retorna o modo escolhido
// Retorna: 0 = Classico, 1-6 = Outros modos (em breve), -1 = Sair
int mostrar_menu_principal(void) {
    clear();
    int cores_disponiveis = has_colors();
    int selecao = 0; // 0 = Classico, 1-5 = Outros, 6 = Configs, 7 = Musica, 8 = Sair
    
    // Configurar para não bloquear na leitura
    nodelay(stdscr, FALSE);
    timeout(-1); // Bloquear até receber entrada
    
    while (1) {
        clear();
        
        // Título
        attron(A_BOLD);
        mvprintw(LINES / 2 - 10, COLS / 2 - 20, "========================================");
        mvprintw(LINES / 2 - 9, COLS / 2 - 20, "  KEEP SOLVING AND NOBODY EXPLODES");
        mvprintw(LINES / 2 - 7, COLS / 2 - 20, "========================================");
        attroff(A_BOLD);
        
        mvprintw(LINES / 2 - 5, COLS / 2 - 15, "Selecione o Modo de Jogo:");
        
        // Opções do menu (texto dinâmico para música)
        const char* texto_musica = musica_ligada_global ? "M. Desligar Musica" : "M. Ligar Musica";
        
        const char* opcoes[9] = {
            "1. Classico",
            "2. Especialistas [Em Breve]",
            "3. Sobrevivencia [Em Breve]",
            "4. Extras [Em Breve]",
            "5. Treino [Em Breve]",
            "6. Custom [Em Breve]",
            "C. Configs [Em Breve]",
            texto_musica,
            "Q. Sair"
        };
        
        // Desenhar opções
        for (int i = 0; i < 9; i++) {
            int y;
            if (i < 6) {
                // Opções 1-6: posições normais
                y = LINES / 2 - 3 + i;
            } else {
                // Opções C, M e Q: adicionar espaço em branco antes (linha extra)
                // i=6 (C. Configs) vai para posição que seria i=7
                // i=7 (M. Ligar/Desligar Musica) vai para posição que seria i=8
                // i=8 (Q. Sair) vai para posição que seria i=9
                y = LINES / 2 - 3 + i + 1;
            }
            int x = COLS / 2 - 20;
            
            if (i == selecao) {
                if (cores_disponiveis) {
                    attron(A_REVERSE | COLOR_PAIR(1));
                } else {
                    attron(A_REVERSE);
                }
            }
            
            mvprintw(y, x, "%s", opcoes[i]);
            
            if (i == selecao) {
                if (cores_disponiveis) {
                    attroff(A_REVERSE | COLOR_PAIR(1));
                } else {
                    attroff(A_REVERSE);
                }
            }
        }
        
        refresh();
        
        // Ler entrada (bloqueia até receber)
        int ch = getch();
        if (ch == KEY_UP || ch == 'w' || ch == 'W') {
            selecao = (selecao - 1 + 10) % 10;
        } else if (ch == KEY_DOWN || ch == 's' || ch == 'S') {
            selecao = (selecao + 1) % 10;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            if (selecao == 0) {
                // Classico - retorna 0
                return 0;
            } else if (selecao >= 1 && selecao <= 5) {
                // Modos em breve - não faz nada, apenas mostra que está em breve
                // Pode adicionar uma mensagem aqui se quiser
            } else if (selecao == 6) {
                // Configs em breve - não faz nada
            } else if (selecao == 7) {
                // Toggle música
                // Menu não é fase média, então volume normal
                definir_dificuldade_musica(0);
                if (!audio_disponivel_global) {
                    // Áudio não disponível - mostrar mensagem temporária
                    clear();
                    mvprintw(LINES / 2, COLS / 2 - 30, "Audio nao disponivel!");
                    mvprintw(LINES / 2 + 1, COLS / 2 - 35, "Instale: sudo apt-get install libsdl2-mixer-dev");
                    mvprintw(LINES / 2 + 3, COLS / 2 - 15, "Pressione qualquer tecla...");
                    refresh();
                    nodelay(stdscr, FALSE);
                    getch();
                    nodelay(stdscr, FALSE);
                    timeout(-1);
                } else if (musica_ligada_global) {
                    parar_musica();
                    musica_ligada_global = 0;
                    definir_musica_ligada(0); // Notificar audio.c que música foi desligada
                } else {
                    definir_musica_ligada(1); // Notificar audio.c que música foi ligada
                    if (tocar_musica("sounds/Menu.mp3")) {
                        musica_ligada_global = 1;
                    } else {
                        // Falha ao tocar música - mostrar mensagem
                        clear();
                        mvprintw(LINES / 2, COLS / 2 - 25, "Erro ao tocar musica!");
                        mvprintw(LINES / 2 + 1, COLS / 2 - 20, "Verifique: sounds/Menu.mp3");
                        mvprintw(LINES / 2 + 3, COLS / 2 - 15, "Pressione qualquer tecla...");
                        refresh();
                        nodelay(stdscr, FALSE);
                        getch();
                        nodelay(stdscr, FALSE);
                        timeout(-1);
                    }
                }
            } else if (selecao == 8) {
                // Sair
                return -1;
            }
        } else if (ch == 'q' || ch == 'Q') {
            return -1;
        } else if (ch == '1') {
            return 0; // Classico
        } else if (ch == 'm' || ch == 'M') {
            // Toggle música com tecla M
            if (!audio_disponivel_global) {
                // Áudio não disponível - mostrar mensagem temporária
                clear();
                mvprintw(LINES / 2, COLS / 2 - 30, "Audio nao disponivel!");
                mvprintw(LINES / 2 + 1, COLS / 2 - 35, "Instale: sudo apt-get install libsdl2-mixer-dev");
                mvprintw(LINES / 2 + 3, COLS / 2 - 15, "Pressione qualquer tecla...");
                refresh();
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, FALSE);
                timeout(-1);
            } else if (musica_ligada_global) {
                parar_musica();
                musica_ligada_global = 0;
                definir_musica_ligada(0); // Notificar audio.c que música foi desligada
            } else {
                // Menu não é fase média, então volume normal
                definir_dificuldade_musica(0);
                definir_musica_ligada(1); // Notificar audio.c que música foi ligada
                if (tocar_musica("sounds/Menu.mp3")) {
                    musica_ligada_global = 1;
                } else {
                    // Falha ao tocar música
                    clear();
                    mvprintw(LINES / 2, COLS / 2 - 25, "Erro ao tocar musica!");
                    mvprintw(LINES / 2 + 1, COLS / 2 - 20, "Verifique: sounds/Menu.mp3");
                    mvprintw(LINES / 2 + 3, COLS / 2 - 15, "Pressione qualquer tecla...");
                    refresh();
                    nodelay(stdscr, FALSE);
                    getch();
                    nodelay(stdscr, FALSE);
                    timeout(-1);
                }
            }
        }
    }
}

// Mostra menu de dificuldades e retorna a dificuldade escolhida (ou -1 para sair)
int mostrar_menu_dificuldades(void) {
    clear();
    int cores_disponiveis = has_colors();
    int selecao = 0; // 0 = Fácil, 1 = Médio, 2 = Difícil, 3 = Sair
    
    // Configurar para bloquear na leitura
    nodelay(stdscr, FALSE);
    timeout(-1); // Bloquear até receber entrada
    
    while (1) {
        clear();
        
        // Título
        attron(A_BOLD);
        mvprintw(LINES / 2 - 8, COLS / 2 - 20, "========================================");
        mvprintw(LINES / 2 - 7, COLS / 2 - 20, "  KEEP SOLVING AND NOBODY EXPLODES");
        mvprintw(LINES / 2 - 6, COLS / 2 - 20, "         VERSAO DE TREINO");
        mvprintw(LINES / 2 - 5, COLS / 2 - 20, "========================================");
        attroff(A_BOLD);
        
        mvprintw(LINES / 2 - 3, COLS / 2 - 15, "Selecione a Dificuldade:");
        
        // Opções do menu
        const char* opcoes[4] = {
            "FACIL",
            "MEDIO",
            "DIFICIL",
            "SAIR"
        };
        
        for (int i = 0; i < 4; i++) {
            if (selecao == i) {
                if (cores_disponiveis) {
                    attron(A_BOLD | COLOR_PAIR(2));
                } else {
                    attron(A_BOLD | A_REVERSE);
                }
                mvprintw(LINES / 2 + i, COLS / 2 - 20, "> %s", opcoes[i]);
                if (cores_disponiveis) {
                    attroff(A_BOLD | COLOR_PAIR(2));
                } else {
                    attroff(A_BOLD | A_REVERSE);
                }
            } else {
                mvprintw(LINES / 2 + i, COLS / 2 - 20, "  %s", opcoes[i]);
            }
        }
        
        mvprintw(LINES / 2 + 5, COLS / 2 - 15, "Use SETAS para navegar, ENTER para selecionar");
        
        refresh();
        
        // Ler entrada (bloqueia até receber)
        int ch = getch();
        
        if (ch == KEY_UP || ch == 'w' || ch == 'W') {
            selecao = (selecao - 1 + 4) % 4;
        } else if (ch == KEY_DOWN || ch == 's' || ch == 'S') {
            selecao = (selecao + 1) % 4;
        } else if (ch == '\n' || ch == '\r') {
            if (selecao == 3) {
                return -1; // Sair
            } else {
                return selecao; // Retorna DIFICULDADE_FACIL, DIFICULDADE_MEDIO ou DIFICULDADE_DIFICIL
            }
        } else if (ch == 'q' || ch == 'Q' || ch == 27) { // ESC ou Q
            return -1;
        }
    }
}

