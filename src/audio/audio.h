#ifndef AUDIO_H
#define AUDIO_H

// Inicializa o sistema de áudio
// Retorna 1 se sucesso, 0 se falhou
int inicializar_audio(void);

// Verifica se o áudio está disponível
int audio_disponivel(void);

// Define a dificuldade atual para ajuste de volume
void definir_dificuldade_musica(int e_fase_media);

// Define se a música está ligada ou desligada
void definir_musica_ligada(int ligada);

// Carrega e toca música de fundo (em loop)
int tocar_musica(const char* arquivo);

// Toca música uma vez e depois volta para Menu.mp3
int tocar_sound_effect(const char* arquivo);

// Para a música
void parar_musica(void);

// Verifica se a música está tocando
int musica_tocando(void);

// Finaliza o sistema de áudio
void finalizar_audio(void);

#endif // AUDIO_H

