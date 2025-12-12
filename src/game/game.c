#include "game.h"
#include "../ui/ui.h"
#include "../fases/fases.h"
#include "../modulos/modulos.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <ncurses.h>

void inicializar_jogo(GameState *g, Dificuldade dificuldade, int num_tedax, int num_bancadas) {
    const ConfigFase *config = obter_config_fase(dificuldade);
    
    if (num_tedax < 1) num_tedax = config->num_tedax;
    if (num_tedax > 5) num_tedax = 5;
    if (num_bancadas < 1) num_bancadas = config->num_bancadas;
    if (num_bancadas > 5) num_bancadas = 5;
    
    g->dificuldade = dificuldade;
    g->tempo_total_partida = config->tempo_total_partida;
    g->tempo_restante = g->tempo_total_partida;
    g->qtd_modulos = 0;
    g->proximo_id_modulo = 1;
    g->modulos_necessarios = config->modulos_necessarios;
    g->intervalo_geracao = config->intervalo_geracao;
    g->max_modulos = config->modulos_necessarios;
    
    g->qtd_tedax = num_tedax;
    for (int i = 0; i < num_tedax; i++) {
        g->tedax[i].id = i + 1;
        g->tedax[i].estado = TEDAX_LIVRE;
        g->tedax[i].modulo_atual = -1;
        g->tedax[i].bancada_atual = -1;
        g->tedax[i].qtd_fila = 0;
        g->tedax[i].fila_modulos[0] = -1;
    }
    
    g->qtd_bancadas = num_bancadas;
    for (int i = 0; i < num_bancadas; i++) {
        g->bancadas[i].id = i + 1;
        g->bancadas[i].estado = BANCADA_LIVRE;
        g->bancadas[i].tedax_ocupando = -1;
        g->bancadas[i].tedax_esperando = -1;
    }
    
    srand(time(NULL));
    
    g->ticks_desde_ultimo_modulo = 0;
    g->jogo_rodando = 1;
    g->jogo_terminou = 0;
    g->mensagem_erro[0] = '\0';
    g->erros_cometidos = 0;
    
    pthread_mutex_init(&g->mutex_jogo, NULL);
    pthread_cond_init(&g->cond_modulo_disponivel, NULL);
    pthread_cond_init(&g->cond_bancada_disponivel, NULL);
    pthread_cond_init(&g->cond_tela_atualizada, NULL);
    
    pthread_mutex_lock(&g->mutex_jogo);
    for (int i = 0; i < config->modulos_iniciais; i++) {
        gerar_novo_modulo(g);
    }
    pthread_mutex_unlock(&g->mutex_jogo);
}

void finalizar_jogo(GameState *g) {
    g->jogo_rodando = 0;
    pthread_mutex_destroy(&g->mutex_jogo);
    pthread_cond_destroy(&g->cond_modulo_disponivel);
    pthread_cond_destroy(&g->cond_bancada_disponivel);
    pthread_cond_destroy(&g->cond_tela_atualizada);
}

void gerar_novo_modulo(GameState *g) {
    if (g->qtd_modulos >= 100) {
        return;
    }
    
    Modulo *novo = &g->modulos[g->qtd_modulos];
    novo->id = g->proximo_id_modulo++;
    
    const ConfigFase *config = obter_config_fase(g->dificuldade);
    novo->tempo_total = config->tempo_minimo_execucao + (rand() % (config->tempo_variacao_execucao + 1));
    novo->tempo_restante = novo->tempo_total;
    
    // Escolher tipo de módulo aleatoriamente com pesos por dificuldade
    // fácil: 40% fios, 40% botão, 20% hash
    // médio: 40% fios, 30% botão, 30% hash
    // difícil: 40% fios, 20% botão, 40% hash
    int tipo_aleatorio = rand() % 100;
    switch (g->dificuldade) {
        case DIFICULDADE_FACIL:
            if (tipo_aleatorio < 40) {
                gerar_modulo_fios(novo, g->dificuldade);
            } else if (tipo_aleatorio < 80) {
                gerar_modulo_botao(novo, g->dificuldade);
            } else {
                gerar_modulo_senha(novo, g->dificuldade);
            }
            break;
        case DIFICULDADE_MEDIO:
            if (tipo_aleatorio < 40) {
                gerar_modulo_fios(novo, g->dificuldade);
            } else if (tipo_aleatorio < 70) {
                gerar_modulo_botao(novo, g->dificuldade);
            } else {
                gerar_modulo_senha(novo, g->dificuldade);
            }
            break;
        case DIFICULDADE_DIFICIL:
            if (tipo_aleatorio < 40) {
                gerar_modulo_fios(novo, g->dificuldade);
            } else if (tipo_aleatorio < 60) {
                gerar_modulo_botao(novo, g->dificuldade);
            } else {
                gerar_modulo_senha(novo, g->dificuldade);
            }
            break;
        default:
            gerar_modulo_botao(novo, g->dificuldade);
            break;
    }
    
    novo->instrucao_digitada[0] = '\0';
    novo->estado = MOD_PENDENTE;
    novo->tempo_desde_resolvido = -1;
    
    g->qtd_modulos++;
    pthread_cond_broadcast(&g->cond_modulo_disponivel);
}

int contar_modulos_resolvidos(const GameState *g) {
    int resolvidos = 0;
    for (int i = 0; i < g->qtd_modulos; i++) {
        if (g->modulos[i].estado == MOD_RESOLVIDO) {
            resolvidos++;
        }
    }
    return resolvidos;
}

int tem_modulos_pendentes(const GameState *g) {
    for (int i = 0; i < g->qtd_modulos; i++) {
        if (g->modulos[i].estado == MOD_PENDENTE) {
            return 1;
        }
    }
    return 0;
}

int todos_modulos_resolvidos(const GameState *g) {
    int resolvidos = contar_modulos_resolvidos(g);
    return (resolvidos >= g->modulos_necessarios);
}

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

void* thread_mural(void* arg) {
    GameState *g = (GameState*)arg;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 200000000L;
    
    while (g->jogo_rodando && !g->jogo_terminou) {
        pthread_mutex_lock(&g->mutex_jogo);
        
        if (g->qtd_modulos < g->max_modulos) {
            g->ticks_desde_ultimo_modulo++;
            
            if (g->ticks_desde_ultimo_modulo >= g->intervalo_geracao) {
                gerar_novo_modulo(g);
                g->ticks_desde_ultimo_modulo = 0;
            }
        }
        
        if (!tem_modulos_pendentes(g) && g->qtd_modulos < g->max_modulos) {
            gerar_novo_modulo(g);
        }
        
        pthread_mutex_unlock(&g->mutex_jogo);
        
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

void* thread_exibicao(void* arg) {
    GameState *g = (GameState*)arg;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 200000000L;
    
    extern char buffer_instrucao_global[64];
    
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
    
    while (g->jogo_rodando && !g->jogo_terminou) {
        pthread_mutex_lock(&g->mutex_jogo);
        desenhar_tela(g, buffer_instrucao_global);
        pthread_mutex_unlock(&g->mutex_jogo);
        nanosleep(&ts, NULL);
    }
    
    
    return NULL;
}

void* thread_tedax(void* arg) {
    typedef struct {
        GameState *g;
        int tedax_id;
    } TedaxArgs;
    
    TedaxArgs *args = (TedaxArgs*)arg;
    GameState *g = args->g;
    int tedax_id = args->tedax_id;
    Tedax *tedax = &g->tedax[tedax_id];
    
    struct timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    
    while (g->jogo_rodando && !g->jogo_terminou) {
        pthread_mutex_lock(&g->mutex_jogo);
        
        for (int i = 0; i < g->qtd_modulos; i++) {
            if (g->modulos[i].estado == MOD_RESOLVIDO && g->modulos[i].tempo_desde_resolvido >= 0) {
                g->modulos[i].tempo_desde_resolvido++;
            }
        }
        
        // Verifica se tedax em espera pode ocupar bancada liberada
        if (tedax->estado == TEDAX_ESPERANDO && tedax->bancada_atual >= 0) {
            int bancada_idx = tedax->bancada_atual;
            if (g->bancadas[bancada_idx].estado == BANCADA_LIVRE && 
                g->bancadas[bancada_idx].tedax_esperando == tedax->id) {
                g->bancadas[bancada_idx].estado = BANCADA_OCUPADA;
                g->bancadas[bancada_idx].tedax_ocupando = tedax->id;
                g->bancadas[bancada_idx].tedax_esperando = -1;
                tedax->estado = TEDAX_OCUPADO;
                
                if (tedax->modulo_atual >= 0) {
                    Modulo *mod_esperando = &g->modulos[tedax->modulo_atual];
                    if (mod_esperando->estado != MOD_EM_EXECUCAO) {
                        mod_esperando->estado = MOD_EM_EXECUCAO;
                    }
                    if (mod_esperando->tempo_restante <= 0) {
                        mod_esperando->tempo_restante = mod_esperando->tempo_total;
                    }
                }
            } else if (g->bancadas[bancada_idx].estado == BANCADA_OCUPADA && 
                       g->bancadas[bancada_idx].tedax_ocupando != tedax->id &&
                       g->bancadas[bancada_idx].tedax_esperando != tedax->id) {
                // Outro tedax ocupou: procurar outra bancada livre
                int bancada_encontrada = 0;
                for (int i = 0; i < g->qtd_bancadas; i++) {
                    if (g->bancadas[i].estado == BANCADA_LIVRE) {
                        if (bancada_idx >= 0 && bancada_idx < g->qtd_bancadas) {
                            if (g->bancadas[bancada_idx].tedax_esperando == tedax->id) {
                                g->bancadas[bancada_idx].tedax_esperando = -1;
                            }
                        }
                        tedax->bancada_atual = i;
                        g->bancadas[i].estado = BANCADA_OCUPADA;
                        g->bancadas[i].tedax_ocupando = tedax->id;
                        g->bancadas[i].tedax_esperando = -1;
                        tedax->estado = TEDAX_OCUPADO;
                        bancada_encontrada = 1;
                        
                        if (tedax->modulo_atual >= 0) {
                            Modulo *mod_esperando = &g->modulos[tedax->modulo_atual];
                            if (mod_esperando->estado != MOD_EM_EXECUCAO) {
                                mod_esperando->estado = MOD_EM_EXECUCAO;
                            }
                            if (mod_esperando->tempo_restante <= 0) {
                                mod_esperando->tempo_restante = mod_esperando->tempo_total;
                            }
                        }
                        break;
                    }
                }
                
                if (!bancada_encontrada) {
                    // Nenhuma livre: manter em espera na primeira disponível
                    int nova_bancada_idx = 0;
                    if (bancada_idx != nova_bancada_idx) {
                        if (bancada_idx >= 0 && bancada_idx < g->qtd_bancadas) {
                            if (g->bancadas[bancada_idx].tedax_esperando == tedax->id) {
                                g->bancadas[bancada_idx].tedax_esperando = -1;
                            }
                        }
                        if (g->bancadas[nova_bancada_idx].tedax_esperando < 0) {
                            tedax->bancada_atual = nova_bancada_idx;
                            g->bancadas[nova_bancada_idx].tedax_esperando = tedax->id;
                        }
                    }
                }
            }
        }
        
        // Processa módulo atribuído ao tedax
        if (tedax->estado == TEDAX_OCUPADO && tedax->modulo_atual >= 0) {
            Modulo *mod = &g->modulos[tedax->modulo_atual];
            
            if (mod->estado == MOD_RESOLVIDO) {
                if (tedax->bancada_atual >= 0) {
                    g->bancadas[tedax->bancada_atual].estado = BANCADA_LIVRE;
                    g->bancadas[tedax->bancada_atual].tedax_ocupando = -1;
                }
                tedax->estado = TEDAX_LIVRE;
                tedax->modulo_atual = -1;
                tedax->bancada_atual = -1;
                pthread_mutex_unlock(&g->mutex_jogo);
                nanosleep(&ts, NULL);
                continue;
            }
            
            if (mod->estado != MOD_EM_EXECUCAO) {
                mod->estado = MOD_EM_EXECUCAO;
                if (mod->tempo_restante <= 0) {
                    mod->tempo_restante = mod->tempo_total;
                }
            }
            
            if (strlen(mod->instrucao_digitada) == 0) {
                pthread_mutex_unlock(&g->mutex_jogo);
                nanosleep(&ts, NULL);
                continue;
            }
            
            mod->tempo_restante--;
            
            if (mod->tempo_restante <= 0) {
                if (validar_instrucao_modulo(mod, mod->instrucao_digitada)) {
                    mod->estado = MOD_RESOLVIDO;
                    mod->tempo_desde_resolvido = 0;
                } else {
                    mod->estado = MOD_PENDENTE;
                    mod->tempo_restante = mod->tempo_total;
                    mod->instrucao_digitada[0] = '\0';
                    mod->tempo_desde_resolvido = -1;
                    g->erros_cometidos++;
                }
                
                if (tedax->bancada_atual >= 0) {
                    int bancada_idx = tedax->bancada_atual;
                    g->bancadas[bancada_idx].estado = BANCADA_LIVRE;
                    g->bancadas[bancada_idx].tedax_ocupando = -1;
                    
                    // Atribui bancada liberada ao tedax em espera (evita condição de corrida)
                    if (g->bancadas[bancada_idx].estado == BANCADA_LIVRE && 
                        g->bancadas[bancada_idx].tedax_esperando >= 0) {
                        int tedax_esperando_id = g->bancadas[bancada_idx].tedax_esperando;
                        for (int i = 0; i < g->qtd_tedax; i++) {
                            if (g->tedax[i].id == tedax_esperando_id && 
                                g->tedax[i].estado == TEDAX_ESPERANDO &&
                                g->tedax[i].bancada_atual == bancada_idx) {
                                if (g->bancadas[bancada_idx].estado == BANCADA_LIVRE) {
                                    g->bancadas[bancada_idx].estado = BANCADA_OCUPADA;
                                    g->bancadas[bancada_idx].tedax_ocupando = g->tedax[i].id;
                                    g->bancadas[bancada_idx].tedax_esperando = -1;
                                    g->tedax[i].estado = TEDAX_OCUPADO;
                                    
                                    if (g->tedax[i].modulo_atual >= 0) {
                                        Modulo *mod_esperando = &g->modulos[g->tedax[i].modulo_atual];
                                        if (mod_esperando->estado != MOD_EM_EXECUCAO) {
                                            mod_esperando->estado = MOD_EM_EXECUCAO;
                                        }
                                        if (mod_esperando->tempo_restante <= 0) {
                                            mod_esperando->tempo_restante = mod_esperando->tempo_total;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                    
                    pthread_cond_broadcast(&g->cond_bancada_disponivel);
                }
                
                // Processa próximo módulo da fila (máximo 1)
                if (tedax->qtd_fila > 0) {
                    int proximo_modulo_idx = tedax->fila_modulos[0];
                    if (proximo_modulo_idx >= 0 && proximo_modulo_idx < g->qtd_modulos) {
                        Modulo *prox_mod = &g->modulos[proximo_modulo_idx];
                        if (prox_mod->estado == MOD_RESOLVIDO) {
                            tedax->fila_modulos[0] = -1;
                            tedax->qtd_fila = 0;
                        } else {
                            tedax->fila_modulos[0] = -1;
                            tedax->qtd_fila = 0;
                            if (tedax->bancada_atual >= 0 && 
                                g->bancadas[tedax->bancada_atual].estado == BANCADA_LIVRE) {
                                tedax->modulo_atual = proximo_modulo_idx;
                                prox_mod->estado = MOD_EM_EXECUCAO;
                                prox_mod->tempo_restante = prox_mod->tempo_total;
                                g->bancadas[tedax->bancada_atual].estado = BANCADA_OCUPADA;
                                g->bancadas[tedax->bancada_atual].tedax_ocupando = tedax->id;
                            } else {
                                int bancada_encontrada = 0;
                                for (int i = 0; i < g->qtd_bancadas; i++) {
                                    if (g->bancadas[i].estado == BANCADA_LIVRE) {
                                        tedax->modulo_atual = proximo_modulo_idx;
                                        tedax->bancada_atual = i;
                                        prox_mod->estado = MOD_EM_EXECUCAO;
                                        prox_mod->tempo_restante = prox_mod->tempo_total;
                                        g->bancadas[i].estado = BANCADA_OCUPADA;
                                        g->bancadas[i].tedax_ocupando = tedax->id;
                                        bancada_encontrada = 1;
                                        break;
                                    }
                                }
                                
                                if (!bancada_encontrada) {
                                    if (g->qtd_bancadas > 0) {
                                        int bancada_idx = 0;
                                        tedax->modulo_atual = proximo_modulo_idx;
                                        tedax->bancada_atual = bancada_idx;
                                        tedax->estado = TEDAX_ESPERANDO;
                                        prox_mod->estado = MOD_EM_EXECUCAO;
                                        prox_mod->tempo_restante = prox_mod->tempo_total;
                                        g->bancadas[bancada_idx].tedax_esperando = tedax->id;
                                    } else {
                                        prox_mod->estado = MOD_PENDENTE;
                                        tedax->modulo_atual = -1;
                                        tedax->bancada_atual = -1;
                                        tedax->estado = TEDAX_LIVRE;
                                    }
                                }
                            }
                        }
                    } else {
                        tedax->fila_modulos[0] = -1;
                        tedax->qtd_fila = 0;
                    }
                } else {
                    tedax->estado = TEDAX_LIVRE;
                    tedax->modulo_atual = -1;
                    tedax->bancada_atual = -1;
                }
                
                if (!tem_modulos_pendentes(g) && g->qtd_modulos < g->max_modulos) {
                    gerar_novo_modulo(g);
                }
                
                pthread_cond_broadcast(&g->cond_modulo_disponivel);
            }
        }
        
        pthread_mutex_unlock(&g->mutex_jogo);
        nanosleep(&ts, NULL);
    }
    
    free(args);
    return NULL;
}

// Processa comando formato T1B1M1:ppp
int processar_comando(const char* buffer, GameState *g, 
                      int *tedax_idx, int *bancada_idx, int *modulo_idx, 
                      char *instrucao) {
    *tedax_idx = -1;
    *bancada_idx = -1;
    *modulo_idx = -1;
    instrucao[0] = '\0';
    
    const char *separador = strchr(buffer, ':');
    
    if (!separador) {
        // Sem ':' - verifica se tem T/B/M (formato inválido)
        int tem_comando = 0;
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] == 'T' || buffer[i] == 't' || 
                buffer[i] == 'B' || buffer[i] == 'b' || 
                buffer[i] == 'M' || buffer[i] == 'm') {
                tem_comando = 1;
                break;
            }
        }
        
        if (tem_comando) {
            return 0;
        }
        
        strncpy(instrucao, buffer, 15);
        instrucao[15] = '\0';
        return 1;
    }
    
    strncpy(instrucao, separador + 1, 15);
    instrucao[15] = '\0';
    
    int len_comando = separador - buffer;
    if (len_comando == 0) {
        return 1;
    }
    
    char comando[32];
    strncpy(comando, buffer, len_comando);
    comando[len_comando] = '\0';
    
    int i = 0;
    while (i < len_comando) {
        if (comando[i] == 'T' || comando[i] == 't') {
            i++;
            int num = 0;
            while (i < len_comando && comando[i] >= '0' && comando[i] <= '9') {
                num = num * 10 + (comando[i] - '0');
                i++;
            }
            if (num >= 1 && num <= g->qtd_tedax) {
                *tedax_idx = num - 1;
            } else {
                return 0;
            }
        } else if (comando[i] == 'B' || comando[i] == 'b') {
            i++;
            int num = 0;
            while (i < len_comando && comando[i] >= '0' && comando[i] <= '9') {
                num = num * 10 + (comando[i] - '0');
                i++;
            }
            if (num >= 1 && num <= g->qtd_bancadas) {
                *bancada_idx = num - 1;
            } else {
                return 0;
            }
        } else if (comando[i] == 'M' || comando[i] == 'm') {
            i++;
            int num = 0;
            while (i < len_comando && comando[i] >= '0' && comando[i] <= '9') {
                num = num * 10 + (comando[i] - '0');
                i++;
            }
            int encontrado = 0;
            for (int j = 0; j < g->qtd_modulos; j++) {
                if (g->modulos[j].id == num) {
                    *modulo_idx = j;
                    encontrado = 1;
                    break;
                }
            }
            if (!encontrado) {
                return 0;
            }
        } else {
            i++;
        }
    }
    
    return 1;
}

void* thread_coordenador(void* arg) {
    GameState *g = (GameState*)arg;
    extern char buffer_instrucao_global[64];
    int buffer_len = 0;
    
    while (g->jogo_rodando && !g->jogo_terminou) {
        int ch = getch();
        
        if (ch == ERR) {
        } else if (ch == 'q' || ch == 'Q') {
            pthread_mutex_lock(&g->mutex_jogo);
            g->jogo_rodando = 0;
            pthread_mutex_unlock(&g->mutex_jogo);
            break;
        } else {
            pthread_mutex_lock(&g->mutex_jogo);
            
            if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (buffer_len > 0) {
                    buffer_len--;
                    buffer_instrucao_global[buffer_len] = '\0';
                }
            }
            else if (ch == '\n' || ch == '\r') {
                g->mensagem_erro[0] = '\0';
                
                int tedax_idx = -1, bancada_idx = -1, modulo_idx = -1;
                char instrucao[16] = "";
                
                if (processar_comando(buffer_instrucao_global, g, &tedax_idx, &bancada_idx, &modulo_idx, instrucao)) {
                    // Aplica regras de default para valores não especificados
                    if (tedax_idx == -1) {
                        for (int i = 0; i < g->qtd_tedax; i++) {
                            if (g->tedax[i].estado == TEDAX_LIVRE) {
                                tedax_idx = i;
                                break;
                            }
                        }
                    }
                    
                    if (bancada_idx == -1) {
                        for (int i = 0; i < g->qtd_bancadas; i++) {
                            if (g->bancadas[i].estado == BANCADA_LIVRE) {
                                bancada_idx = i;
                                break;
                            }
                        }
                        if (bancada_idx == -1 && g->qtd_bancadas > 0) {
                            bancada_idx = 0;
                        }
                    }
                    
                    if (modulo_idx == -1) {
                        for (int i = 0; i < g->qtd_modulos; i++) {
                            if (g->modulos[i].estado == MOD_PENDENTE) {
                                modulo_idx = i;
                                break;
                            }
                        }
                    }
                    
                    int valido = 1;
                    
                    if (tedax_idx < 0 || tedax_idx >= g->qtd_tedax) {
                        valido = 0;
                    }
                    
                    if (bancada_idx < 0 || bancada_idx >= g->qtd_bancadas) {
                        valido = 0;
                    }
                    
                    if (modulo_idx < 0 || modulo_idx >= g->qtd_modulos) {
                        valido = 0;
                    } else if (g->modulos[modulo_idx].estado != MOD_PENDENTE) {
                        valido = 0;
                        if (g->modulos[modulo_idx].estado == MOD_RESOLVIDO) {
                            valido = 0;
                        }
                    }
                    
                    if (valido && strlen(instrucao) > 0) {
                        Modulo *mod = &g->modulos[modulo_idx];
                        Tedax *t = &g->tedax[tedax_idx];
                        
                        strncpy(mod->instrucao_digitada, instrucao, 15);
                        mod->instrucao_digitada[15] = '\0';
                        
                        if (t->estado == TEDAX_OCUPADO) {
                            // Adiciona à fila (máximo 1)
                            if (t->qtd_fila > 0) {
                                strncpy(g->mensagem_erro, "Tedax ja tem modulo em espera", 63);
                                g->mensagem_erro[63] = '\0';
                            } else {
                                t->fila_modulos[0] = modulo_idx;
                                t->qtd_fila = 1;
                                mod->estado = MOD_PENDENTE;
                                mod->tempo_restante = mod->tempo_total;
                            }
                        } else {
                            // Limpa espera anterior se tedax estava esperando
                            if (t->estado == TEDAX_ESPERANDO && t->bancada_atual >= 0) {
                                if (g->bancadas[t->bancada_atual].tedax_esperando == t->id) {
                                    g->bancadas[t->bancada_atual].tedax_esperando = -1;
                                }
                                
                                if (t->modulo_atual >= 0 && t->modulo_atual < g->qtd_modulos) {
                                    Modulo *mod_anterior = &g->modulos[t->modulo_atual];
                                    if (mod_anterior->estado == MOD_EM_EXECUCAO) {
                                        mod_anterior->estado = MOD_PENDENTE;
                                        mod_anterior->tempo_restante = mod_anterior->tempo_total;
                                        mod_anterior->instrucao_digitada[0] = '\0';
                                        mod_anterior->tempo_desde_resolvido = -1;
                                    }
                                }
                            }
                            
                            mod->estado = MOD_EM_EXECUCAO;
                            mod->tempo_restante = mod->tempo_total;
                            
                            if (g->bancadas[bancada_idx].estado == BANCADA_LIVRE) {
                                t->estado = TEDAX_OCUPADO;
                                t->modulo_atual = modulo_idx;
                                t->bancada_atual = bancada_idx;
                                
                                g->bancadas[bancada_idx].estado = BANCADA_OCUPADA;
                                g->bancadas[bancada_idx].tedax_ocupando = t->id;
                                g->bancadas[bancada_idx].tedax_esperando = -1;
                            } else {
                                // Bancada ocupada: procura outra livre ou entra em espera
                                if (g->bancadas[bancada_idx].tedax_esperando >= 0) {
                                    int bancada_encontrada = 0;
                                    for (int i = 0; i < g->qtd_bancadas; i++) {
                                        if (g->bancadas[i].estado == BANCADA_LIVRE && 
                                            g->bancadas[i].tedax_esperando < 0) {
                                            t->estado = TEDAX_OCUPADO;
                                            t->modulo_atual = modulo_idx;
                                            t->bancada_atual = i;
                                            g->bancadas[i].estado = BANCADA_OCUPADA;
                                            g->bancadas[i].tedax_ocupando = t->id;
                                            g->bancadas[i].tedax_esperando = -1;
                                            bancada_encontrada = 1;
                                            break;
                                        }
                                    }
                                    
                                    if (!bancada_encontrada) {
                                        int bancada_espera = -1;
                                        for (int i = 0; i < g->qtd_bancadas; i++) {
                                            if (g->bancadas[i].tedax_esperando < 0) {
                                                bancada_espera = i;
                                                break;
                                            }
                                        }
                                        
                                        if (bancada_espera >= 0) {
                                            t->estado = TEDAX_ESPERANDO;
                                            t->modulo_atual = modulo_idx;
                                            t->bancada_atual = bancada_espera;
                                            g->bancadas[bancada_espera].tedax_esperando = t->id;
                                        } else {
                                            t->estado = TEDAX_ESPERANDO;
                                            t->modulo_atual = modulo_idx;
                                            t->bancada_atual = bancada_idx;
                                        }
                                    }
                                } else {
                                    t->estado = TEDAX_ESPERANDO;
                                    t->modulo_atual = modulo_idx;
                                    t->bancada_atual = bancada_idx;
                                    g->bancadas[bancada_idx].tedax_esperando = t->id;
                                }
                            }
                        }
                        
                        buffer_len = 0;
                        buffer_instrucao_global[0] = '\0';
                    } else {
                        strncpy(g->mensagem_erro, "Entrada Invalida", 63);
                        g->mensagem_erro[63] = '\0';
                        buffer_len = 0;
                        buffer_instrucao_global[0] = '\0';
                    }
                }
                else {
                    strncpy(g->mensagem_erro, "Entrada Invalida", 63);
                    g->mensagem_erro[63] = '\0';
                    buffer_len = 0;
                    buffer_instrucao_global[0] = '\0';
                }
            }
            else if (ch >= 32 && ch <= 126) {
                if (buffer_len < 63) {
                    buffer_instrucao_global[buffer_len] = (char)ch;
                    buffer_len++;
                    buffer_instrucao_global[buffer_len] = '\0';
                }
            }
            
            pthread_mutex_unlock(&g->mutex_jogo);
        }
        
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 50000000L;
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

