#ifndef AUDIO_H
#define AUDIO_H

int inicializar_audio(void);
int audio_disponivel(void);
void definir_dificuldade_musica(int e_fase_media);
void definir_musica_ligada(int ligada);
int tocar_musica(const char* arquivo);
int tocar_sound_effect(const char* arquivo);
void parar_musica(void);
int musica_tocando(void);
void finalizar_audio(void);

#endif