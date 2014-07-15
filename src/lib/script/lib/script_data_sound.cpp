#include "../../file/file.h"
#include "../script.h"
#include "../../config.h"
#include "script_data_common.h"


#ifdef _X_USE_SOUND_
	#include "../../sound/sound.h"
#endif


namespace Script{

#ifdef _X_USE_SOUND_
	#define sound_p(p)		(void*)p
#else
	typedef int Sound;
	typedef int Music;
	#define sound_p(p)		NULL
#endif

extern Type *TypeFloatList;

void SIAddPackageSound()
{
	add_package("sound", false);

	Type *TypeSound = add_type("Sound",		sizeof(Sound));
	Type *TypeSoundP = add_type_p("Sound*", TypeSound);
	Type *TypeMusic = add_type("Music",		sizeof(Music));
	Type *TypeMusicP = add_type_p("Music*", TypeMusic);

	add_class(TypeSound);
		class_add_func("set",							TypeVoid,	sound_p(mf(&Sound::SetData)));
			func_add_param("pos",		TypeVector);
			func_add_param("vel",		TypeVector);
			func_add_param("r_min",		TypeFloat32);
			func_add_param("r_max",		TypeFloat32);
			func_add_param("speed",		TypeFloat32);
			func_add_param("volume",	TypeFloat32);
		class_add_func("play",							TypeVoid,	sound_p(mf(&Sound::Play)));
			func_add_param("loop",		TypeBool);
		class_add_func("stop",							TypeVoid,	sound_p(mf(&Sound::Stop)));
		class_add_func("pause",							TypeVoid,	sound_p(mf(&Sound::Pause)));
			func_add_param("pause",		TypeBool);
		class_add_func("__delete__",									TypeVoid,	sound_p(mf(&Sound::__delete__)));

	add_class(TypeMusic);
		class_add_func("play",									TypeVoid,	sound_p(mf(&Music::Play)));
			func_add_param("loop",		TypeBool);
		class_add_func("stop",									TypeVoid,	sound_p(mf(&Music::Stop)));
		class_add_func("pause",									TypeVoid,	sound_p(mf(&Music::Pause)));
			func_add_param("pause",		TypeBool);
		class_add_func("setRate",							TypeVoid,	sound_p(mf(&Music::SetRate)));
			func_add_param("rate",		TypeFloat32);
	
	// sound
	add_func("EmitSound",									TypeSoundP,	sound_p(&SoundEmit));
		func_add_param("filename",	TypeString);
		func_add_param("pos",		TypeVector);
		func_add_param("r_min",		TypeFloat32);
		func_add_param("r_max",		TypeFloat32);
		func_add_param("speed",		TypeFloat32);
		func_add_param("volume",	TypeFloat32);
		func_add_param("loop",		TypeBool);
	add_func("LoadSound",									TypeSoundP,	sound_p(&SoundLoad));
		func_add_param("filename",	TypeString);
	// music
	add_func("LoadMusic",									TypeMusicP,	sound_p(&MusicLoad));
		func_add_param("filename",		TypeString);
	// sound creation
	add_func("SaveSound",		TypeVoid, 		sound_p(&SoundSaveFile));
		func_add_param("filename",		TypeString);
		func_add_param("buf_r",			TypeFloatList);
		func_add_param("buf_l",			TypeFloatList);
		func_add_param("freq",			TypeInt);
		func_add_param("channels",		TypeInt);
		func_add_param("bits",			TypeInt);
	
	add_ext_var("VolumeMusic",		TypeFloat32,		sound_p(&VolumeMusic));
	add_ext_var("VolumeSounds",		TypeFloat32,		sound_p(&VolumeSound));
}

};
