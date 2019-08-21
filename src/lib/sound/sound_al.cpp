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

extern Array<Sound*> Sounds;
extern Array<Music*> Musics;


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
}

void SoundExit()
{
	SoundReset();
	if (al_context)
		alcDestroyContext(al_context);
	al_context = NULL;
	if (al_dev)
		alcCloseDevice(al_dev);
	al_dev = NULL;
//	alutExit();
}

Sound *SoundLoad(const string &filename)
{
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

	Sound *s = new Sound;
	
	if (((af.channels == 1) && (af.buffer)) || (cached >= 0)){
		
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
	return s;
}


Sound *SoundEmit(const string &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop)
{
	Sound *s = SoundLoad(filename);
	s->Suicidal = true;
	s->SetData(pos, v_0, min_dist, max_dist, speed, volume);
	s->Play(loop);
	return s;
}

Sound::Sound()
{
	Suicidal = false;
	Pos = v_0;
	Vel = v_0;
	Volume = 1;
	Speed = 1;
	al_source = 0;
	al_buffer = 0;
	Sounds.add(this);
}

Sound::~Sound()
{
	Stop();
	for (int i=0;i<SmallAudioCache.num;i++)
		if (al_buffer == SmallAudioCache[i].al_buffer)
			SmallAudioCache[i].ref_count --;
	//alDeleteBuffers(1, &al_buffer);
	alDeleteSources(1, &al_source);
	for (int i=0;i<Sounds.num;i++)
		if (Sounds[i] == this)
			Sounds.erase(i);
}

void Sound::__delete__()
{
	this->~Sound();
}

void SoundClearSmallCache()
{
	for (int i=0;i<SmallAudioCache.num;i++)
		alDeleteBuffers(1, &SmallAudioCache[i].al_buffer);
	SmallAudioCache.clear();
}

void Sound::Play(bool loop)
{
	alSourcei(al_source, AL_LOOPING, loop);
	alSourcePlay(al_source);
}

void Sound::Stop()
{
	alSourceStop(al_source);
}

void Sound::Pause(bool pause)
{
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	if ((pause) && (state == AL_PLAYING))
		alSourcePause(al_source);
	else if ((!pause) && (state == AL_PAUSED))
		alSourcePlay(al_source);
}

bool Sound::IsPlaying()
{
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool Sound::Ended()
{
	return !IsPlaying(); // TODO... (paused...)
}

void Sound::SetData(const vector &pos, const vector &vel, float min_dist, float max_dist, float speed, float volume)
{
	Pos = pos;
	Vel = vel;
	Volume = volume;
	Speed = speed;
	alSourcef (al_source, AL_PITCH,    Speed);
	alSourcef (al_source, AL_GAIN,     Volume * VolumeSound);
	alSource3f(al_source, AL_POSITION, Pos.x, Pos.y, Pos.z);
	alSource3f(al_source, AL_VELOCITY, Vel.x, Vel.y, Vel.z);
	//alSourcei (al_source, AL_LOOPING,  false);
	alSourcef (al_source, AL_REFERENCE_DISTANCE, min_dist);
	alSourcef (al_source, AL_MAX_DISTANCE, max_dist);
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
	rot = matrix::rotation(ang);
	up = rot.transform_normal(vector::EY);
	ListenerOri[3] = up.x;
	ListenerOri[4] = up.y;
	ListenerOri[5] = up.z;
	alListener3f(AL_POSITION,    pos.x, pos.y, pos.z);
	alListener3f(AL_VELOCITY,    vel.x, vel.y, vel.z);
	alListenerfv(AL_ORIENTATION, ListenerOri);
	alSpeedOfSound(v_sound);
}

bool sAudioStream::stream(int buf)
{
	if (state != StreamStateReady)
		return false;
	load_sound_step(this);
	if (channels == 2){
		if (bits == 8)
			alBufferData(buf, AL_FORMAT_STEREO8, buffer, buf_samples * 2, freq);
		else if (bits == 16)
			alBufferData(buf, AL_FORMAT_STEREO16, buffer, buf_samples * 4, freq);
	}else{
		if (bits == 8)
			alBufferData(buf, AL_FORMAT_MONO8, buffer, buf_samples, freq);
		else if (bits == 16)
			alBufferData(buf, AL_FORMAT_MONO16, buffer, buf_samples * 2, freq);
	}
	return true;
}

Music *MusicLoad(const string &filename)
{
	msg_write(SoundDir + filename);
	int id = -1;
	sAudioStream as = load_sound_start(SoundDir + filename);

	Music *m = new Music();

	if (as.state == StreamStateReady){

		alGenSources(1, &m->al_source);
		alGenBuffers(2, m->al_buffer);
		m->stream = as;

		// start streaming
		int num_buffers = 0;
		if (as.stream(m->al_buffer[0]))
			num_buffers ++;
		if (as.stream(m->al_buffer[1]))
			num_buffers ++;
		alSourceQueueBuffers(m->al_source, num_buffers, m->al_buffer);


		alSourcef(m->al_source, AL_PITCH,           m->Speed);
		alSourcef(m->al_source, AL_GAIN,            m->Volume * VolumeMusic);
		alSourcei(m->al_source, AL_LOOPING,         false);
		alSourcei(m->al_source, AL_SOURCE_RELATIVE, AL_TRUE);
	}
	return m;
}

Music::Music()
{
	Volume = 1;
	Speed = 1;
	al_source = 0;
	al_buffer[0] = 0;
	al_buffer[1] = 0;
	Musics.add(this);
}

Music::~Music()
{
	Stop();
	alSourceUnqueueBuffers(al_source, 2, al_buffer);
	load_sound_end(&stream);
	alDeleteBuffers(2, al_buffer);
	alDeleteSources(1, &al_source);
	for (int i=0;i<Musics.num;i++)
		if (Musics[i] == this)
			Musics.erase(i);
}

void Music::__delete__()
{
	this->~Music();
}

void Music::Play(bool loop)
{
	//alSourcei   (al_source, AL_LOOPING, loop);
	alSourcePlay(al_source);
	alSourcef(al_source, AL_GAIN, Volume * VolumeMusic);
}

void Music::SetRate(float rate)
{
}

void Music::Stop()
{
	alSourceStop(al_source);
}

void Music::Pause(bool pause)
{
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	if ((pause) && (state == AL_PLAYING))
		alSourcePause(al_source);
	else if ((!pause) && (state == AL_PAUSED))
		alSourcePlay(al_source);
}

bool Music::IsPlaying()
{
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool Music::Ended()
{
	return !IsPlaying();
}

void Music::Iterate()
{
	alSourcef(al_source, AL_GAIN, Volume * VolumeMusic);
	int processed;
	alGetSourcei(al_source, AL_BUFFERS_PROCESSED, &processed);
	while(processed --){
		ALuint buf;
		alSourceUnqueueBuffers(al_source, 1, &buf);
		if (stream.stream(buf))
			alSourceQueueBuffers(al_source, 1, &buf);
	}
}

#else

void SoundInit(){}
void SoundExit(){}
Sound* SoundLoad(const string &filename){ return NULL; }
Sound* SoundEmit(const string &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop){ return NULL; }
Sound::Sound(){}
Sound::~Sound(){}
void Sound::__delete__(){}
void SoundClearSmallCache(){}
void Sound::Play(bool repeat){}
void Sound::Stop(){}
void Sound::Pause(bool pause){}
bool Sound::IsPlaying(){ return false; }
bool Sound::Ended(){ return false; }
void Sound::SetData(const vector &pos, const vector &vel, float min_dist, float max_dist, float speed, float volume){}
void SoundSetListener(const vector &pos, const vector &ang, const vector &vel, float v_sound){}
Music *MusicLoad(const string &filename){ return NULL; }
Music::~Music(){}
void Music::Play(bool repeat){}
void Music::SetRate(float rate){}
void Music::Stop(){}
void Music::Pause(bool pause){}
bool Music::IsPlaying(){ return false; }
bool Music::Ended(){ return false; }
void Music::Iterate(){}

#endif

