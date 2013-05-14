/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

// TODO: cache small buffers...

 #include "../math/math.h"
 #include "sound.h"

#ifdef SOUND_ALLOW_OPENAL


#ifdef OS_WINDOWS
	#include <al.h>
	//#include <alut.h>
	#include <alc.h>
	//#pragma comment(lib,"alut.lib")
	#pragma comment(lib,"OpenAL32.lib")
	/*#pragma comment(lib,"libogg.lib")
	#pragma comment(lib,"libvorbis.lib")
	#pragma comment(lib,"libvorbisfile.lib")*/
#else
	#include <AL/al.h>
	//#include <AL/alut.h>
	#include <AL/alc.h>
#endif



struct sSmallAudio
{
	unsigned int al_buffer;
	string filename;
	int ref_count;
};
Array<sSmallAudio> SmallAudioCache;

sAudioFile load_sound_file(const string &filename);
sAudioStream load_sound_start(const string &filename);
void load_sound_step(sAudioStream *as);
void load_sound_end(sAudioStream *as);


ALCdevice *al_dev = NULL;
ALCcontext *al_context = NULL;

void SoundInit()
{
	msg_db_r("SoundInit", 0);

	al_dev = alcOpenDevice(NULL);
	if (al_dev){
		al_context = alcCreateContext(al_dev, NULL);
		if (al_context)
			alcMakeContextCurrent(al_context);
		else
			msg_error("could not create openal context");
	}else
		msg_error("could not open openal device");
	bool ok = (al_context);

	
	//bool ok = alutInit(NULL, 0);
	if (!ok)
		msg_error("sound init (openal)");

	msg_db_l(0);
}

void SoundExit()
{
	msg_db_r("SoundExit", 1);
	SoundReset();
	if (al_context)
		alcDestroyContext(al_context);
	al_context = NULL;
	if (al_dev)
		alcCloseDevice(al_dev);
	al_dev = NULL;
//	alutExit();
	msg_db_l(1);
}

int SoundLoad(const string &filename)
{
	msg_db_r("SoundLoad", 1);
	int id = -1;

	// cached?
	int cached = -1;
	for (int i=0;i<SmallAudioCache.num;i++)
		if (SmallAudioCache[i].filename == filename){
			SmallAudioCache[i].ref_count ++;
			cached = i;
			break;
		}

	// no -> load from file
	sAudioFile af;
	if (cached < 0)
		af = load_sound_file(SoundDir + filename);
	
	if (((af.channels == 1) && (af.buffer)) || (cached >= 0)){

		// get data structure
		sSound *s = NULL;
		for (int i=0;i<Sound.num;i++)
			if (!Sound[i].Used){
				s = &Sound[i];
				id = i;
				break;
			}
		if (!s){
			id = Sound.num;
			sSound ss;
			Sound.add(ss);
			s = &Sound.back();
		}
		
		s->Used = true;
		s->Suicidal = false;
		s->Pos = v_0;
		s->Vel = v_0;
		s->Volume = 1;
		s->Speed = 1;
		alGenSources(1, &s->al_source);
		if (cached >= 0){
			s->al_buffer = SmallAudioCache[cached].al_buffer;
		}else{

			// fill data into al-buffer
			alGenBuffers(1, &s->al_buffer);
			if (af.bits == 8)
				alBufferData(s->al_buffer, AL_FORMAT_MONO8, af.buffer, af.samples, af.freq);
			else if (af.bits == 16)
				alBufferData(s->al_buffer, AL_FORMAT_MONO16, af.buffer, af.samples * 2, af.freq);

			// put into small audio cache
			sSmallAudio sa;
			sa.filename = filename;
			sa.al_buffer = s->al_buffer;
			sa.ref_count = 1;
			SmallAudioCache.add(sa);
		}

		// set up al-source
		alSourcei (s->al_source, AL_BUFFER,   s->al_buffer);
		alSourcef (s->al_source, AL_PITCH,    s->Speed);
		alSourcef (s->al_source, AL_GAIN,     s->Volume * VolumeSound);
		alSource3f(s->al_source, AL_POSITION, s->Pos.x, s->Pos.y, s->Pos.z);
		alSource3f(s->al_source, AL_VELOCITY, s->Vel.x, s->Vel.y, s->Vel.z);
		alSourcei (s->al_source, AL_LOOPING,  false);
	}
	if ((af.buffer) && (cached < 0))
	    delete[](af.buffer);
	msg_db_l(1);
	return id;
}


void SoundEmit(const string &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop)
{
	int id = SoundLoad(filename);
	if (id >= 0){
		sSound *s = &Sound[id];
		s->Suicidal = true;
		SoundSetData(id, pos, v_0, min_dist, max_dist, speed, volume);
		SoundPlay(id, loop);
	}
}

void SoundDelete(int index)
{
	if (!SoundUsable(index))
		return;
	SoundStop(index);
	Sound[index].Used = false;
	for (int i=0;i<SmallAudioCache.num;i++)
		if (Sound[index].al_buffer == SmallAudioCache[i].al_buffer)
			SmallAudioCache[i].ref_count --;
	//alDeleteBuffers(1, &Sound[index].al_buffer);
	alDeleteSources(1, &Sound[index].al_source);
}

void SoundClearSmallCache()
{
	for (int i=0;i<SmallAudioCache.num;i++)
		alDeleteBuffers(1, &SmallAudioCache[i].al_buffer);
	SmallAudioCache.clear();
}

void SoundPlay(int index, bool repeat)
{
	if (!SoundUsable(index))
		return;
	alSourcei (Sound[index].al_source, AL_LOOPING, repeat);
	alSourcePlay(Sound[index].al_source);
}

void SoundStop(int index)
{
	if (!SoundUsable(index))
		return;
	alSourceStop(Sound[index].al_source);
}

void SoundPause(int index, bool pause)
{
	if (!SoundUsable(index))
		return;
	int state;
	alGetSourcei(Sound[index].al_source, AL_SOURCE_STATE, &state);
	if ((pause) && (state == AL_PLAYING))
		alSourcePause(Sound[index].al_source);
	else if ((!pause) && (state == AL_PAUSED))
		alSourcePlay(Sound[index].al_source);
}

bool SoundIsPlaying(int index)
{
	if (!SoundUsable(index))
		return false;
	int state;
	alGetSourcei(Sound[index].al_source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool SoundEnded(int index)
{
	return !SoundIsPlaying(index); // TODO... (paused...)
}

void SoundSetData(int index, const vector &pos, const vector &vel, float min_dist, float max_dist, float speed, float volume)
{
	if (SoundUsable(index)){
		sSound *s = &Sound[index];
		s->Pos = pos;
		s->Vel = vel;
		s->Volume = volume;
		s->Speed = speed;
		alSourcef (s->al_source, AL_PITCH,    s->Speed);
		alSourcef (s->al_source, AL_GAIN,     s->Volume * VolumeSound);
		alSource3f(s->al_source, AL_POSITION, s->Pos.x, s->Pos.y, s->Pos.z);
		alSource3f(s->al_source, AL_VELOCITY, s->Vel.x, s->Vel.y, s->Vel.z);
		//alSourcei (s->al_source, AL_LOOPING,  false);
		alSourcef (s->al_source, AL_REFERENCE_DISTANCE, min_dist);
		alSourcef (s->al_source, AL_MAX_DISTANCE, max_dist);
	}
}

void SoundSetListener(const vector &pos, const vector &ang, const vector &vel, float v_sound)
{
	ALfloat ListenerOri[6];
	vector dir = ang.ang2dir();
	ListenerOri[0] = dir.x;
	ListenerOri[1] = dir.y;
	ListenerOri[2] = dir.z;
	vector up;
	matrix rot;
	MatrixRotation(rot, ang);
	up = rot.transform_normal(e_y);
	ListenerOri[3] = up.x;
	ListenerOri[4] = up.y;
	ListenerOri[5] = up.z;
	alListener3f(AL_POSITION,    pos.x, pos.y, pos.z);
	alListener3f(AL_VELOCITY,    vel.x, vel.y, vel.z);
	alListenerfv(AL_ORIENTATION, ListenerOri);
	alSpeedOfSound(v_sound);
}

bool stream(int buf, sAudioStream *as)
{
	msg_db_r("stream", 1);
	if (as->state != StreamStateReady){
		msg_db_l(1);
		return false;
	}
	load_sound_step(as);
	if (as->channels == 2){
		if (as->bits == 8)
			alBufferData(buf, AL_FORMAT_STEREO8, as->buffer, as->buf_samples * 2, as->freq);
		else if (as->bits == 16)
			alBufferData(buf, AL_FORMAT_STEREO16, as->buffer, as->buf_samples * 4, as->freq);
	}else{
		if (as->bits == 8)
			alBufferData(buf, AL_FORMAT_MONO8, as->buffer, as->buf_samples, as->freq);
		else if (as->bits == 16)
			alBufferData(buf, AL_FORMAT_MONO16, as->buffer, as->buf_samples * 2, as->freq);
	}
	msg_db_l(1);
	return true;
}

int MusicLoad(const string &filename)
{
	msg_db_r("load music", 0);
	int channels, bits, samples, freq;
	msg_write(SoundDir + filename);
	int id = -1;
	sAudioStream as = load_sound_start(SoundDir + filename);

	if (as.state == StreamStateReady){

		sMusic *m = NULL;
		for (int i=0;i<Music.num;i++)
			if (!Music[i].Used){
				m = &Music[i];
				id = i;
				break;
			}
		if (!m){
			id = Music.num;
			sMusic mm;
			Music.add(mm);
			m = &Music.back();
		}
		m->Used = true;
		m->Volume = 1;
		m->Speed = 1;
		alGenSources(1, &m->al_source);
		alGenBuffers(2, m->al_buffer);
		m->stream = as;

		// start streaming
		int num_buffers = 0;
		if (stream(m->al_buffer[0], &as))
			num_buffers ++;
		if (stream(m->al_buffer[1], &as))
			num_buffers ++;
		alSourceQueueBuffers(m->al_source, num_buffers, m->al_buffer);


		alSourcef(m->al_source, AL_PITCH,           m->Speed);
		alSourcef(m->al_source, AL_GAIN,            m->Volume * VolumeMusic);
		alSourcei(m->al_source, AL_LOOPING,         false);
		alSourcei(m->al_source, AL_SOURCE_RELATIVE, AL_TRUE);
	}
	msg_db_l(0);
	return id;
}

void MusicDelete(int index)
{
	if (!MusicUsable(index))
		return;
	MusicStop(index);
	alSourceUnqueueBuffers(Music[index].al_source, 2, Music[index].al_buffer);
	load_sound_end(&Music[index].stream);
	alDeleteBuffers(2, Music[index].al_buffer);
	alDeleteSources(1, &Music[index].al_source);
	Music[index].Used = false;
}

void MusicPlay(int index, bool repeat)
{
	if (!MusicUsable(index))
		return;
	sMusic *m = &Music[index];
	//alSourcei   (m->al_source, AL_LOOPING, repeat);
	alSourcePlay(m->al_source);
	alSourcef(m->al_source, AL_GAIN, m->Volume * VolumeMusic);
}

void MusicSetRate(int index,float rate)
{
}

void MusicStop(int index)
{
	if (!MusicUsable(index))
		return;
	alSourceStop(Music[index].al_source);
}

void MusicPause(int index,bool pause)
{
	if (!MusicUsable(index))
		return;
	int state;
	alGetSourcei(Music[index].al_source, AL_SOURCE_STATE, &state);
	if ((pause) && (state == AL_PLAYING))
		alSourcePause(Music[index].al_source);
	else if ((!pause) && (state == AL_PAUSED))
		alSourcePlay(Music[index].al_source);
}

bool MusicIsPlaying(int index)
{
	if (!MusicUsable(index))
		return false;
	int state;
	alGetSourcei(Music[index].al_source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool MusicEnded(int index)
{
	return !MusicIsPlaying(index);
}

void MusicStep(int index)
{
	//if (!MusicIsPlaying(index))
	if (!MusicUsable(index))
		return;
	msg_db_r("MusicStep", 1);
	sMusic *m = &Music[index];
	alSourcef(m->al_source, AL_GAIN, m->Volume * VolumeMusic);
	int processed;
	alGetSourcei(m->al_source, AL_BUFFERS_PROCESSED, &processed);
	while(processed --){
		ALuint buf;
		alSourceUnqueueBuffers(m->al_source, 1, &buf);
		if (stream(buf, &m->stream))
			alSourceQueueBuffers(m->al_source, 1, &buf);
	}
	msg_db_l(1);
}

#endif

