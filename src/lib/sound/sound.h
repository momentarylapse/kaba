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
#include "../math/math.h"


extern Path SoundDir;

struct Sound
{
	bool Loop, Suicidal;
	vector Pos, Vel;
	float Volume, Speed;

	unsigned int al_source, al_buffer;

	Sound();
	~Sound();
	void _cdecl __delete__();
	void _cdecl Play(bool loop);
	void _cdecl Stop();
	void _cdecl Pause(bool pause);
	bool _cdecl IsPlaying();
	bool _cdecl Ended();
	void _cdecl SetData(const vector &pos, const vector &vel, float min_dist, float max_dist, float speed, float volume);
};

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

	bool stream(int buf);
};

struct Music
{
	float Volume, Speed;

	unsigned int al_source, al_buffer[2];
	sAudioStream stream;

	Music();
	~Music();
	void _cdecl __delete__();
	void _cdecl Play(bool loop);
	void _cdecl SetRate(float rate);
	void _cdecl Stop();
	void _cdecl Pause(bool pause);
	bool _cdecl IsPlaying();
	bool _cdecl Ended();

	void Iterate();
};

extern float VolumeMusic, VolumeSound;


void SoundInit();
void SoundExit();
void SoundCalcMove();
void _cdecl SoundSetListener(const vector &pos, const vector &ang, const vector &vel, float v_sound);
void SoundReset();
void SoundClearSmallCache();

// sound
Sound *_cdecl SoundLoad(const Path &filename);
Sound *_cdecl SoundEmit(const Path &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop);

// music
Music* _cdecl MusicLoad(const Path &filename);

// writing
void _cdecl SoundSaveFile(const Path &filename, const Array<float> &data_r, const Array<float> &data_l, int freq, int channels, int bits);

#endif
