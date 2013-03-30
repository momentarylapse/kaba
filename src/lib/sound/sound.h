/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _SOUND_EXISTS_
#define _SOUND_EXISTS_

#include "../config.h"
#include "../file/file.h"


extern string SoundDir;

struct sSound
{
	bool Used;
	bool Loop, Suicidal;
	vector Pos, Vel;
	float Volume, Speed;

	unsigned al_source, al_buffer;
};
extern Array<sSound> Sound;

struct sAudioFile
{
	int channels, bits, samples, freq;
	char *buffer;
};

enum
{
	StreamStateError,
	StreamStateReady,
	StreamStateEnd
};

struct sAudioStream
{
	int channels, bits, samples, freq;
	char *buffer;
	int buf_samples;
	void *vf;
	int type, state;
};

struct sMusic
{
	bool Used;
	float Volume, Speed;

	unsigned al_source, al_buffer[2];
	sAudioStream stream;
};
extern Array<sMusic> Music;

extern float VolumeMusic, VolumeSound;


void SoundInit();
void SoundExit();
void SoundCalcMove();
void SoundSetListener(const vector &pos, const vector &ang, const vector &vel, float v_sound);
void SoundReset();
void SoundClearSmallCache();

// sound
bool SoundUsable(int index);
int SoundLoad(const string &filename);
void SoundEmit(const string &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop);
void SoundDelete(int index);
void SoundPlay(int index, bool repeat);
void SoundStop(int index);
void SoundPause(int index, bool pause);
bool SoundIsPlaying(int index);
bool SoundEnded(int index);
void SoundSetData(int index, const vector &pos, const vector &vel, float min_dist, float max_dist, float speed, float volume);

// music
bool MusicUsable(int index);
int _cdecl MusicLoad(const string &filename);
void _cdecl MusicDelete(int index);
void _cdecl MusicPlay(int index,bool repeat);
void _cdecl MusicSetRate(int index,float rate);
void _cdecl MusicStop(int index);
void _cdecl MusicPause(int index,bool pause);
bool _cdecl MusicIsPlaying(int index);
bool _cdecl MusicEnded(int index);

// writing
void SoundSaveFile(const string &filename, const Array<float> &data_r, const Array<float> &data_l, int freq, int channels, int bits);

#endif
