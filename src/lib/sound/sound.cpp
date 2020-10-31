/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

 #include "../math/math.h"
 #include "sound.h"


#ifdef SOUND_ALLOW_OGG
	#include <vorbis/codec.h>
	#include <vorbis/vorbisfile.h>
	#include <vorbis/vorbisenc.h>
#endif

Path SoundDir;

Array<Sound*> Sounds;
Array<Music*> Musics;

float VolumeMusic = 1.0f, VolumeSound = 1.0f;

void SoundCalcMove()
{
	for (int i=Sounds.num-1;i>=0;i--)
			if (Sounds[i]->Suicidal)
				if (Sounds[i]->Ended())
					delete(Sounds[i]);
	for (int i=0;i<Musics.num;i++)
		Musics[i]->Iterate();
}

void SoundReset()
{
	for (int i=Sounds.num-1;i>=0;i--)
		delete(Sounds[i]);
	Sounds.clear();
	for (int i=Musics.num-1;i>=0;i--)
		delete(Musics[i]);
	Musics.clear();
	SoundClearSmallCache();
}

sAudioFile EmptyAudioFile = {0, 0, 0, 0, NULL};
sAudioStream EmptyAudioStream = {0, 0, 0, 0, NULL, 0, NULL, 0, 0};

sAudioFile load_wave_file(const Path &filename);
sAudioFile load_ogg_file(const Path &filename);
sAudioStream load_ogg_start(const Path &filename);
void load_ogg_step(sAudioStream *as);
void load_ogg_end(sAudioStream *as);

enum
{
	AudioStreamWave,
	AudioStreamOgg,
	AudioStreamFlac,
};

sAudioFile load_sound_file(const Path &filename)
{
	msg_write("loading sound: " + filename.str());
	string ext = filename.extension();
	if (ext == "wav")
		return load_wave_file(filename);
#ifdef SOUND_ALLOW_OGG
	else if (ext == "ogg")
		return load_ogg_file(filename);
#endif
	return EmptyAudioFile;
}

sAudioStream load_sound_start(const Path &filename)
{
	string ext = filename.extension();
	/*if (ext == "wav")
		return load_wave_start(filename);
	else*/ if (ext == "ogg")
		return load_ogg_start(filename);
	return EmptyAudioStream;
}

void load_sound_step(sAudioStream *as)
{
	/*if (as->type == AudioStreamWave)
		load_ogg_step(as);
	else*/ if (as->type == AudioStreamOgg)
		load_ogg_step(as);
}

void load_sound_end(sAudioStream *as)
{
	/*if (as->type == AudioStreamWave)
		load_ogg_end(as);
	else*/ if (as->type == AudioStreamOgg)
		load_ogg_end(as);
}


sAudioFile load_wave_file(const Path &filename)
{
	sAudioFile r;
	r.buffer = NULL;
//	ProgressStatus(_("lade wave"), 0);
	File *f = FileOpen(filename);
	char *data = new char[f->get_size()];
	char header[44];
	f->read_buffer(header, 44);
	if ((header[0] != 'R') or (header[1] != 'I') or (header[2] != 'F') or (header[3] != 'F')){
		msg_error("wave file does not start with \"RIFF\"");
		return r;
	}
	/*if (*(int*)&header[4] != f->GetSize())
		msg_write("wave file gives wrong size");
		// sometimes 0x2400ff7f*/
	if ((header[8] != 'W') or (header[9] != 'A') or (header[10] != 'V') or (header[11] != 'E') or (header[12] != 'f') or (header[13] != 'm') or (header[14] != 't') or (header[15] != ' ')){
		msg_error("\"WAVEfmt \" expected in wave file");
		return r;
	}
	if ((*(int*)&header[16] != 16) or (*(short*)&header[20] != 1)){
		msg_write("wave file does not have format 16/1");
		return r;
	}
	r.channels = *(short*)&header[22];
	r.freq = *(int*)&header[24];
	int block_align = *(short*)&header[32];
	r.bits = *(short*)&header[34];
	int byte_per_sample = (r.bits / 8) * r.channels;
	if ((header[36] != 'd') or (header[37] != 'a') or (header[38] != 't') or (header[39] != 'a')){
		msg_error("\"data\" expected in wave file");
		return r;
	}
	int size = *(int*)&header[40];
	if ((size > f->get_size() - 44) or (size < 0)){
		msg_write("wave file gives wrong data size");
		size = f->get_size() - 44;
	}
	r.samples = size / byte_per_sample;
//	ProgressStatus(_("lade wave"), 0.1f);

	int read = 0;
	int nn = 0;
	while (read < size){
		int toread = 65536;
		if (toread > size - read)
			toread = size - read;
		int rr = f->read_buffer(&data[read], toread);
		nn ++;
/*		if (nn > 16){
			ProgressStatus(_("lade wave"), perc_read + dperc_read * (float)read / (float)size);
			nn = 0;
		}*/
		if (rr > 0)
			read += rr;
		else{
			msg_error("could not read in wave file...");
			break;
		}
	}

	FileClose(f);
	r.buffer = data;
	
	return r;
}


#ifdef SOUND_ALLOW_OGG

char ogg_buffer[4096];

sAudioFile load_ogg_file(const string &filename)
{
	sAudioFile r = EmptyAudioFile;
	OggVorbis_File vf;
	
	if (ov_fopen((char*)filename.c_str(), &vf)){
		msg_error("ogg: ov_fopen failed");
		return r;
	}
	vorbis_info *vi = ov_info(&vf, -1);
	r.bits = 16;
	if (vi){
		r.channels = vi->channels;
		r.freq = vi->rate;
	}
	int bytes_per_sample = (r.bits / 8) * r.channels;
	r.samples = (int)ov_pcm_total(&vf, -1);
	char *data=new char[r.samples * bytes_per_sample + 4096];
	int current_section;
	int read = 0;
	while(true){
		int toread = 4096;
		int rr = ov_read(&vf, &data[read], toread, 0, 2, 1, &current_section); // 0,2,1 = little endian, 16bit, signed
		if (rr == 0)
			break;
		else if (rr < 0){
			msg_error("ogg: ov_read failed");
			break;
		}else{
			read += rr;
		}
	}
	ov_clear(&vf);
	r.samples = read / bytes_per_sample;
	r.buffer = data;
	return r;
}

sAudioStream load_ogg_start(const string &filename)
{
	sAudioStream r;
	r.type = AudioStreamOgg;
	r.vf = new OggVorbis_File;
	r.state = StreamStateReady;
	r.buffer = NULL;
	r.buf_samples = 0;
	
	if (ov_fopen((char*)filename.c_str(), (OggVorbis_File*)r.vf)){
		r.state = StreamStateError;
		msg_error("ogg: ov_fopen failed");
		return r;
	}
	vorbis_info *vi = ov_info((OggVorbis_File*)r.vf, -1);
	r.bits = 16;
	if (vi){
		r.channels = vi->channels;
		r.freq = vi->rate;
	}
	r.samples = (int)ov_pcm_total((OggVorbis_File*)r.vf, -1);
	r.buffer = new char[65536 * 4 + 1024];
	return r;
}

void load_ogg_step(sAudioStream *as)
{
	if (as->state != StreamStateReady)
		return;
	int current_section;
	int bytes_per_sample = (as->bits / 8) * as->channels;
	int wanted = 65536 * bytes_per_sample;
	
	int read = 0;
	while(read < wanted){
		int toread = min(wanted - read, 4096);
		int rr = ov_read((OggVorbis_File*)as->vf, &as->buffer[read], toread, 0, 2, 1, &current_section); // 0,2,1 = little endian, 16bit, signed
		if (rr == 0){
			as->state = StreamStateEnd;
			break;
		}else if (rr < 0){
			as->state = StreamStateError;
			msg_error("ogg: ov_read failed");
			break;
		}else{
			read += rr;
		}
	}
	as->buf_samples = read / bytes_per_sample;
}

void load_ogg_end(sAudioStream *as)
{
	ov_clear((OggVorbis_File*)as->vf);
	if (as->vf)
		delete((OggVorbis_File*)as->vf);
	if (as->buffer)
		delete[](as->buffer);
}

#else

sAudioFile load_ogg_file(const Path &filename){ sAudioFile r; return r; }
sAudioStream load_ogg_start(const Path &filename){ sAudioStream r; return r; }
void load_ogg_step(sAudioStream *as){}
void load_ogg_end(sAudioStream *as){}

#endif

void save_wave_file(const Path &filename, const Array<float> &data_r, const Array<float> &data_l, int freq, int channels, int bits)
{
//	channels = 1;
	bits = 16;
	
	int bytes_per_sample = (bits / 8) * channels;
	int samples = min(data_r.num, data_l.num);
	
	File *f = FileCreate(filename);
	f->write_buffer("RIFF", 4);
	f->write_int(44 + bytes_per_sample * samples); // file size (bytes)
	f->write_buffer("WAVEfmt ", 8);
	f->write_int(16); // fmt size (bytes)
	f->write_word(1); // version
	f->write_word(channels);
	f->write_int(freq);
	f->write_int(freq * bytes_per_sample); // bytes per second
	f->write_word(bytes_per_sample); // byte align
	f->write_word(bits);
	f->write_buffer("data", 4);
	f->write_int(samples * bytes_per_sample); // data size (bytes)
	if (channels == 1){
		for (int i=0;i<samples;i++){
			float br = clamp(data_r[i], -1.0f, 1.0f);
			short sr = (int)(br * 32767.0f);
			int aa = sr;
			f->write_word(aa);
		}
	}else if (channels == 2){
		for (int i=0;i<samples;i++){
			float br = clamp(data_r[i], -1.0f, 1.0f);
			short sr = (int)(br * 32767.0f);
			float bl = clamp(data_l[i], -1.0f, 1.0f);
			short sl = (int)(bl * 32767.0f);
			unsigned int aa = (unsigned int)sr + (((unsigned int)sl) << 16);
			f->write_int(aa);
		}
	}else
		msg_error("save_wave_file... channels != 1,2");
	FileClose(f);
}

void SoundSaveFile(const Path &filename, const Array<float> &data_r, const Array<float> &data_l, int freq, int channels, int bits)
{
	string ext = filename.extension();
	if (ext == "wav")
		save_wave_file(filename, data_r, data_l, freq, channels, bits);
	else
		msg_error("unhandled file extension: " + ext);
}
