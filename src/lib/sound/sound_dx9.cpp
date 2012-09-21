/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

 #include "../types/types.h"
 #include "sound.h"

#ifdef NIX_SOUND_DIRECTX9
#ifdef OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#pragma warning(disable : 4995)
#endif

#ifdef NIX_API_DIRECTX9
	#include <d3dx9.h>
	#ifdef NIX_SOUND_DIRECTX9
		#include <dsound.h>
		#include "_dsutil.h" // get rid of this crap!!!!!!!!!!!!!!!!!!!!!!!!
		#ifdef NIX_IDE_VCS
			#include <dshow.h>
		#endif
	#endif
#endif

#ifdef NIX_API_DIRECTX9
	// sound
	#ifdef NIX_SOUND_DIRECTX9
		LPDIRECTSOUND8 pDS=NULL;
		LPDIRECTSOUNDBUFFER pDSBPrimary=NULL;
		DSBUFFERDESC dsbd;
		LPDIRECTSOUND3DLISTENER pDSListener=NULL;
		DS3DLISTENER dsListenerParams;
		int NumSounds=0;
		vector SoundPos[NIX_MAX_SOUNDS],SoundVel[NIX_MAX_SOUNDS];
		float SoundMinDist[NIX_MAX_SOUNDS],SoundMaxDist[NIX_MAX_SOUNDS],SoundRate[NIX_MAX_SOUNDS],SoundSpeed[NIX_MAX_SOUNDS],SoundVolume[NIX_MAX_SOUNDS];
		DWORD SoundFrequency[NIX_MAX_SOUNDS];
		CSound* sound[NIX_MAX_SOUNDS];
		LPDIRECTSOUND3DBUFFER pDS3DBuffer[NIX_MAX_SOUNDS];
		DS3DBUFFER dsBufferParams[NIX_MAX_SOUNDS];

		#ifdef NIX_IDE_VCS
			IBaseFilter	*Music[NIX_MAX_SOUNDS];
			IPin *MusicPin[NIX_MAX_SOUNDS];
			IGraphBuilder *MusicGraphBuilder[NIX_MAX_SOUNDS];
			IMediaControl *MusicMediaControl[NIX_MAX_SOUNDS];
			IMediaSeeking *MusicMediaSeeking[NIX_MAX_SOUNDS];
			bool MusicRepeat[NIX_MAX_SOUNDS];
			int NumMusics=0;
		#endif
	#endif
	string DXErrorMsg(HRESULT h)
	{
		if (h==D3D_OK)								return("D3D_OK");
		if (h==D3DERR_CONFLICTINGRENDERSTATE)		return("D3DERR_CONFLICTINGRENDERSTATE");
		if (h==D3DERR_CONFLICTINGTEXTUREFILTER)		return("D3DERR_CONFLICTINGTEXTUREFILTER");
		if (h==D3DERR_CONFLICTINGTEXTUREPALETTE)	return("D3DERR_CONFLICTINGTEXTUREPALETTE");
		if (h==D3DERR_DEVICELOST)					return("D3DERR_DEVICELOST");
		if (h==D3DERR_DEVICENOTRESET)				return("D3DERR_DEVICENOTRESET");
		if (h==D3DERR_DRIVERINTERNALERROR)			return("D3DERR_DRIVERINTERNALERROR");
		if (h==D3DERR_INVALIDCALL)					return("D3DERR_INVALIDCALL");
		if (h==D3DERR_INVALIDDEVICE)				return("D3DERR_INVALIDDEVICE");
		if (h==D3DERR_MOREDATA)						return("D3DERR_MOREDATA");
		if (h==D3DERR_NOTAVAILABLE)					return("D3DERR_NOTAVAILABLE");
		if (h==D3DERR_NOTFOUND)						return("D3DERR_NOTFOUND");
		if (h==D3DERR_OUTOFVIDEOMEMORY)				return("D3DERR_OUTOFVIDEOMEMORY");
		if (h==D3DERR_TOOMANYOPERATIONS)			return("D3DERR_TOOMANYOPERATIONS");
		if (h==D3DERR_UNSUPPORTEDALPHAARG)			return("D3DERR_UNSUPPORTEDALPHAARG");
		if (h==D3DERR_UNSUPPORTEDALPHAOPERATION)	return("D3DERR_UNSUPPORTEDALPHAOPERATION");
		if (h==D3DERR_UNSUPPORTEDCOLORARG)			return("D3DERR_UNSUPPORTEDCOLORARG");
		if (h==D3DERR_UNSUPPORTEDCOLOROPERATION)	return("D3DERR_UNSUPPORTEDCOLOROPERATION");
		if (h==D3DERR_UNSUPPORTEDFACTORVALUE)		return("D3DERR_UNSUPPORTEDFACTORVALUE");
		if (h==D3DERR_UNSUPPORTEDTEXTUREFILTER)		return("D3DERR_UNSUPPORTEDTEXTUREFILTER");
		if (h==D3DERR_WRONGTEXTUREFORMAT)			return("D3DERR_WRONGTEXTUREFORMAT");
		if (h==E_FAIL)								return("E_FAIL");
		if (h==E_INVALIDARG)						return("E_INVALIDARG");
		//if (h==E_INVALIDCALL)						return("E_INVALIDCALL");
		if (h==E_OUTOFMEMORY)						return("E_OUTOFMEMORY");
		if (h==S_OK)								return("S_OK");
		return(string("unbekannter Fehler ",i2s(h)));
	}
#endif

void NixSoundInit()
{
#ifdef NIX_SOUND_DIRECTX9

	// initiate sound
	msg_write("-sound");
	DirectSoundCreate8(NULL,&pDS,NULL);
	pDS->SetCooperativeLevel(NixWindow->hWnd,DSSCL_PRIORITY);
	ZeroMemory(&dsbd,sizeof(DSBUFFERDESC));
	dsbd.dwSize			=sizeof(DSBUFFERDESC);
	dsbd.dwFlags		=DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes	=0;
	dsbd.lpwfxFormat	=NULL;
	pDS->CreateSoundBuffer(&dsbd,&pDSBPrimary,NULL);
	WAVEFORMATEX wfx;
	ZeroMemory(&wfx,sizeof(WAVEFORMATEX));
	wfx.wFormatTag		=(WORD)WAVE_FORMAT_PCM;
	wfx.nChannels		=(WORD)2;
	wfx.nSamplesPerSec	=(DWORD)22050;
	wfx.wBitsPerSample	=(WORD)16;
	wfx.nBlockAlign		=(WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec	=(DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);
	pDSBPrimary->SetFormat(&wfx);
	pDSBPrimary->QueryInterface(IID_IDirectSound3DListener,(void**)&pDSListener);
	pDSBPrimary->Release();	pDSBPrimary=NULL;

	dsListenerParams.dwSize=sizeof(DS3DLISTENER);
	pDSListener->GetAllParameters(&dsListenerParams);
	dsListenerParams.flDistanceFactor=0.01f;
	dsListenerParams.flDopplerFactor=1.0f;
	dsListenerParams.flRolloffFactor=1.0f;
    pDSListener->SetAllParameters( &dsListenerParams, DS3D_IMMEDIATE );

	for (int s=0;s<NIX_MAX_SOUNDS;s++)
		sound[s]=NULL;

#endif
}

int NixSoundLoad(const string &filename)
{
	#ifdef NIX_SOUND_DIRECTX9
		msg_write("loading Sound: " + filename);
		msg_right();

		int index=-1;
		for (int i=0;i<NumSounds;i++)
			if (!sound[i]){
				//msg_write("greife alten auf");
				index=i;
				break;
			}
		if (index<0){
			//msg_write("erstelle neuen");
			index=NumSounds;
			NumSounds++;
		}

		HRESULT hr;
		HRESULT hrRet = S_OK;
		LPDIRECTSOUNDBUFFER apDSBuffer;
		DWORD                dwDSBufferSize;
		CWaveFile*           pWaveFile      = NULL;

		char fn[512];
		strcpy(fn,string(HuiAppDirectory,filename));
		msg_write(SysFileName(fn));

		if (!file_test_existence(fn)){
			msg_error("file does not exist!");
			msg_left();
			return -1;
		}

		pWaveFile = new CWaveFile();

		pWaveFile->Open( (LPTSTR)sys_str_f(fn), NULL, WAVEFILE_READ );


		if ( pWaveFile->GetSize() == 0 ){
			delete(pWaveFile);
			msg_error("not readable");
			msg_left();
			return -1;
		}

		// Make the DirectSound buffer the same size as the wav file
		dwDSBufferSize = pWaveFile->GetSize();

		// Create the direct sound buffer, and only request the flags needed
		// since each requires some overhead and limits if the buffer can 
		// be hardware accelerated
		DSBUFFERDESC dsBufferDesc;
		ZeroMemory( &dsBufferDesc, sizeof(DSBUFFERDESC) );
		dsBufferDesc.dwSize				= sizeof(DSBUFFERDESC);
		//DWORD dwCreationFlags			= DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE | DSBCAPS_LOCSOFTWARE;
		DWORD dwCreationFlags			= DSBCAPS_CTRL3D | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME;
		dsBufferDesc.dwFlags			= dwCreationFlags;
		dsBufferDesc.dwBufferBytes		= dwDSBufferSize;
		dsBufferDesc.guid3DAlgorithm	= DS3DALG_HRTF_LIGHT; // DS3DALG_NO_VIRTUALIZATION, DS3DALG_HRTF_FULL, DS3DALG_HRTF_LIGHT
		dsBufferDesc.lpwfxFormat		= pWaveFile->m_pwfx;
    
		// DirectSound is only guarenteed to play PCM data.  Other
		// formats may or may not work depending the sound card driver.
		hr = pDS->CreateSoundBuffer( &dsBufferDesc, &apDSBuffer, NULL );

		// Be sure to return this error code if it occurs so the
		// callers knows this happened.
		if( hr == DS_NO_VIRTUALIZATION )
	        hrRet = DS_NO_VIRTUALIZATION;

	    if (FAILED(hr)){
			// DSERR_BUFFERTOOSMALL will be returned if the buffer is
			// less than DSBSIZE_FX_MIN and the buffer is created
			// with DSBCAPS_CTRLFX.
        
			// It might also fail if hardware buffer mixing was requested
			// on a device that doesn't support it.
			msg_error("CreateSoundBuffer");
			msg_error(string("CreateSoundBuffer: ",DXErrorMsg(hr)));
			msg_left();
			return -1;
		}

    
		// create the sound
		sound[index] = new CSound( &apDSBuffer, dwDSBufferSize, 1, pWaveFile, dwCreationFlags );

	    // get the 3D buffer from the secondary buffer
		hr=sound[index]->m_apDSBuffer[0]->QueryInterface(IID_IDirectSound3DBuffer,(VOID**)&pDS3DBuffer[index]);
	    if (FAILED(hr)){
			msg_error(string("Get3DBufferInterface: ",DXErrorMsg(hr)));
			msg_left();
			return -1;
		}

		// Get the 3D buffer parameters
		dsBufferParams[index].dwSize = sizeof(DS3DBUFFER);
		pDS3DBuffer[index]->GetAllParameters( &dsBufferParams[index] );

		// Set new 3D buffer parameters
		dsBufferParams[index].dwMode = DS3DMODE_HEADRELATIVE;
		pDS3DBuffer[index]->SetAllParameters( &dsBufferParams[index], DS3D_IMMEDIATE );

		DSBCAPS dsbcaps;
		ZeroMemory( &dsbcaps, sizeof(DSBCAPS) );
		dsbcaps.dwSize = sizeof(DSBCAPS);

		SoundPos[index]=SoundVel[index]=v0;
		SoundMinDist[index]=1;
		SoundMaxDist[index]=2;
		SoundRate[index]=1;
		SoundSpeed[index]=1;
		SoundVolume[index]=1;
		sound[index]->m_apDSBuffer[0]->GetFrequency(&SoundFrequency[index]);
		sound[index]->m_apDSBuffer[0]->SetVolume(DSBVOLUME_MAX);

		msg_ok();
		msg_left();
		return index;
	#else
		msg_todo("SoundLoad without DirectX9");
	#endif
	return -1;
}

bool SoundUsable(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if ((index<0)||(index>=NumSounds))
			return false;
		if (sound[index])
			return true;
		return false;
	#endif
	return false;
}

void NixSoundDelete(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		pDS3DBuffer[index]->Release();
		pDS3DBuffer[index]=NULL;
		sound[index]->~CSound();
		sound[index]=NULL;
	#endif
}

void NixSoundPlay(int index,bool repeat)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		//msg_write(string("Play ",i2s(index)));
		//nw->SafeMessage("Play");
		float vol=float(pow(SoundVolume[index],0.15f));
		sound[index]->Play( 0, repeat?DSBPLAY_LOOPING:0 );
		//nw->SafeMessage("/Play");
		//msg_write("  -ok");
	#endif
}

void NixSoundStop(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		sound[index]->Stop();
	#endif
}

void NixSoundSetPause(int index,bool pause)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		//sound[index]->;
	#endif
}

bool NixSoundIsPlaying(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return false;
		DWORD dwStatus=0;
		sound[index]->m_apDSBuffer[0]->GetStatus(&dwStatus);
		return (( dwStatus&DSBSTATUS_PLAYING )!=0);
	#endif
	return false;
}

bool NixSoundEnded(int index)
{
	return false;
}

void NixSoundTestRepeat()
{
}

vector ListenerVel;
matrix ListenerInvMatrix;

void NixSoundSetData(int index,const vector &pos,const vector &vel,float min_dist,float max_dist,float speed,float volume,bool set_now)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		//msg_write(string("SetData ",i2s(index)));
		SoundPos[index]=pos;
		SoundVel[index]=vel;
		SoundMinDist[index]=min_dist;
		SoundMaxDist[index]=max_dist;
		SoundSpeed[index]=speed;
		SoundVolume[index]=volume;

		if (set_now){
			// Frequenz
			int f=int(float(SoundFrequency[index])*SoundSpeed[index]*SoundRate[index]);
			sound[index]->m_apDSBuffer[0]->SetFrequency(f);

			vector dPos,dVel;
			VecTransform(dPos,ListenerInvMatrix,SoundPos[index]);
			dVel=SoundVel[index]-ListenerVel;
			VecNormalTransform(dVel,ListenerInvMatrix,dVel);
			memcpy( &dsBufferParams[index].vPosition, &dPos, sizeof(D3DVECTOR) );
			memcpy( &dsBufferParams[index].vVelocity, &dVel, sizeof(D3DVECTOR) );
			dsBufferParams[index].flMinDistance=SoundMinDist[index];
			dsBufferParams[index].flMaxDistance=SoundMaxDist[index];
			pDS3DBuffer[index]->SetAllParameters( &dsBufferParams[index], DS3D_IMMEDIATE );
			// Lautst�ke
			float vol=float(pow(SoundVolume[index],0.15f));
			bool should_play=((VecLength(dPos)<SoundMaxDist[index])&&(SoundSpeed[index]<10.0f)&&(SoundSpeed[index]>0.001f));
			if (should_play)
				sound[index]->m_apDSBuffer[0]->SetVolume( long(DSBVOLUME_MAX*vol + DSBVOLUME_MIN*(1-vol)) );
			else
				sound[index]->m_apDSBuffer[0]->SetVolume(DSBVOLUME_MIN);
		}
	#endif
}

// setzt auch alle anderen Aenderungen der Sounds erst in Kraft
void NixSoundSetListener(const vector &pos,const vector &ang,const vector &vel,float meters_per_unit)
{
	#ifdef NIX_SOUND_DIRECTX9
		int i;
		//msg_write("Listener");
		matrix a,b;
		MatrixTranslation(a,pos);
		MatrixRotation(b,ang);
		MatrixMultiply(a,a,b);
		MatrixInverse(ListenerInvMatrix,a);
		ListenerVel=vel;
		dsListenerParams.flDistanceFactor=meters_per_unit;
		dsListenerParams.flDopplerFactor=1.5f;
		dsListenerParams.flRolloffFactor=1.5f;
		/*memcpy( &dsListenerParams.vPosition, &pos, sizeof(D3DVECTOR) );
		memcpy( &dsListenerParams.vVelocity, &vel, sizeof(D3DVECTOR) );*/

		//msg_write("  -AllParameters");
		//nw->SafeMessage("Listener");
		pDSListener->SetAllParameters( &dsListenerParams, DS3D_DEFERRED );
		//nw->SafeMessage("/Listener");
		//msg_write("  -Sounds");

		// die Daten der Sounds erstellen
		for (i=0;i<NumSounds;i++){
			if (!SoundUsable(i))
				continue;
			//nw->SafeMessage(i2s(i));
			/*if (!SoundIsPlaying(i))
				continue;*/

			//msg_write(i);

			//nw->SafeMessage("Frequency");
			vector dPos,dVel;
			int f=int(float(SoundFrequency[i])*SoundSpeed[i]*SoundRate[i]);
			sound[i]->m_apDSBuffer[0]->SetFrequency(f);

			VecTransform(dPos,ListenerInvMatrix,SoundPos[i]);
			dVel=SoundVel[i]-ListenerVel;
			VecNormalTransform(dVel,ListenerInvMatrix,dVel);
			memcpy( &dsBufferParams[i].vPosition, &dPos, sizeof(D3DVECTOR) );
			memcpy( &dsBufferParams[i].vVelocity, &dVel, sizeof(D3DVECTOR) );
			dsBufferParams[i].flMinDistance=SoundMinDist[i];
			dsBufferParams[i].flMaxDistance=SoundMaxDist[i];

			//msg_write("    -ap");
			//nw->SafeMessage("SetAllParameters");
			pDS3DBuffer[i]->SetAllParameters( &dsBufferParams[i], DS3D_DEFERRED );
			//pDS3DBuffer[i]->SetAllParameters( &dsBufferParams[i], DS3D_IMMEDIATE );
			//msg_write("    -V");

			float vol=float(pow(SoundVolume[i],0.15f));
			bool should_play=((VecLength(dPos)<SoundMaxDist[i])&&(SoundSpeed[i]<10.0f)&&(SoundSpeed[i]>0.001f));
			//nw->SafeMessage("Volume");
			if (should_play)
				sound[i]->m_apDSBuffer[0]->SetVolume( long(DSBVOLUME_MAX*vol + DSBVOLUME_MIN*(1-vol)) );
			else
				sound[i]->m_apDSBuffer[0]->SetVolume(DSBVOLUME_MIN);
			//msg_write("    -ok");
		}

		// alles ausfuehren
		//msg_write("  -Commit");
		//nw->SafeMessage("Commit");
		pDSListener->CommitDeferredSettings();

		for (i=0;i<NumSounds;i++)
			SoundVel[i]=v0;
		//msg_write("  -ok");
		//nw->SafeMessage("/Listener");
	#endif
}

int NixMusicLoad(const string &filename)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		msg_write("loading music: " + SysFileName(filename));
		msg_right();
		int h=_open(sys_str_f(filename),0);
		_close(h);
		if (h<0){
			msg_error("Musik-Datei nicht gefunden");
			msg_left();
			return -1;
		}
		//NumMusics=0;

		/*msg_write("a");
		WCHAR		wFileName[MAX_PATH];
		msg_write("b");
		CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC,IID_IGraphBuilder,reinterpret_cast<void **>(&MusicGraphBuilder[NumSounds]));
		msg_write("c");
		MusicGraphBuilder[NumMusics]->QueryInterface(IID_IMediaControl,reinterpret_cast<void **>(&MusicMediaControl[NumMusics]));
		msg_write("d");
		MusicGraphBuilder[NumMusics]->QueryInterface(IID_IMediaSeeking,reinterpret_cast<void **>(&MusicMediaSeeking[NumMusics]));
		msg_write("e");
		Music[NumMusics]=NULL;
		//msg_write("f");
		MusicPin[NumMusics]=NULL;
		//msg_write("g");
	#ifndef UNICODE
		MultiByteToWideChar(CP_ACP,0,filename,-1,wFileName,MAX_PATH);
	#else
		lstrcpy(wFileName,filename);
	#endif
	//	msg_write("h");
		MusicGraphBuilder[NumMusics]->AddSourceFilter(wFileName,NULL,&Music[NumMusics]);
	//	msg_write("i");
		Music[NumMusics]->FindPin(L"Output",&MusicPin[NumMusics]);*/


		//msg_write("a");
		WCHAR		wFileName[MAX_PATH];
		//msg_write("b");
		if (NumMusics>0){
			MusicMediaControl[0]->Stop();
		//msg_write("b1");
			Music[NumMusics-1]->Release();
		//msg_write("b2");
			MusicPin[NumMusics-1]->Release();
		//msg_write("b3");
			MusicMediaControl[0]->Release();
		//msg_write("b4");
			MusicMediaSeeking[0]->Release();
		//msg_write("b5");
			MusicGraphBuilder[0]->Release();
		//msg_write("b6");
		NumMusics=0;
		}

		// allgemeines... (nur einmal)
		if (NumMusics<1){
			CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC,IID_IGraphBuilder,reinterpret_cast<void **>(&MusicGraphBuilder[0]));
			//msg_write("c");
			MusicGraphBuilder[0]->QueryInterface(IID_IMediaControl,reinterpret_cast<void **>(&MusicMediaControl[0]));
			//msg_write("d");
			MusicGraphBuilder[0]->QueryInterface(IID_IMediaSeeking,reinterpret_cast<void **>(&MusicMediaSeeking[0]));
			//msg_write("e");
		}else{
			MusicMediaControl[0]->Stop();
			IEnumFilters *pFilterEnum = NULL;
			msg_write(DXErrorMsg(MusicGraphBuilder[0]->EnumFilters(&pFilterEnum)));
			// Allocate space, then pull out all of the
			IBaseFilter *pFilter;
			msg_write(DXErrorMsg(pFilterEnum->Reset()));
			unsigned long nf=0;
			msg_write(DXErrorMsg(pFilterEnum->Next(1, &pFilter, &nf)));
			msg_write(nf);
			msg_write(DXErrorMsg(MusicGraphBuilder[0]->RemoveFilter(pFilter)));
		}


		MultiByteToWideChar(CP_ACP,0,filename,-1,wFileName,MAX_PATH);
	//	msg_write("h");
		MusicGraphBuilder[0]->AddSourceFilter(wFileName,NULL,&Music[NumMusics]);
	//	msg_write("i");
		Music[NumMusics]->FindPin(L"Output",&MusicPin[NumMusics]);



	//	msg_write("j");
		NumMusics++;
		msg_ok();
		msg_left();
		return NumMusics-1;
	}
#endif
#endif
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		return -1;
	}
#endif
	return -1;
}

#ifdef OS_WINDOWS
	VOID CALLBACK MyMusicTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
	{
		NixMusicTestRepeat();
	}
#endif

void NixMusicPlay(int index,bool repeat)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		msg_write("NixMusicPlay");
		/*MusicMediaControl[index]->Stop();
		MusicGraphBuilder[index]->Render(MusicPin[index]);
		LONGLONG llPos=0; // ~sec/10.000.000
		MusicMediaSeeking[index]->SetPositions(&llPos,AM_SEEKING_AbsolutePositioning,&llPos,AM_SEEKING_NoPositioning);
		MusicMediaControl[index]->Run();
		MusicRepeat[index]=repeat;*/

		MusicMediaControl[0]->Stop();
		MusicGraphBuilder[0]->Render(MusicPin[index]);
		LONGLONG llPos=0; // ~sec/10.000.000
		MusicMediaSeeking[0]->SetPositions(&llPos,AM_SEEKING_AbsolutePositioning,&llPos,AM_SEEKING_NoPositioning);
		MusicMediaControl[0]->Run();
		MusicRepeat[index]=repeat;

		SetTimer(NixWindow->hWnd,666,1000,&MyMusicTimerProc);
	}
#endif
#endif
#endif
}

void NixMusicSetRate(int index,float rate)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		//MusicMediaSeeking[index]->SetRate((double)rate);
		MusicMediaSeeking[0]->SetRate((double)rate);
	}
#endif
#endif
#endif
}

void NixMusicStop(int index)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		msg_write("NixMusicStop");
		//MusicMediaControl[index]->Stop();
		MusicMediaControl[0]->Stop();
		MusicRepeat[index]=false;
	}
#endif
#endif
#endif
}

void NixMusicSetPause(int index,bool pause)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		/*if (pause)
			MusicMediaControl[index]->Pause();
		else{
			MusicMediaControl[index]->Stop();
			MusicMediaControl[index]->Run();
		}*/
		if (pause)
			MusicMediaControl[0]->Pause();
		else{
			MusicMediaControl[0]->Stop();
			MusicMediaControl[0]->Run();
		}
	}
#endif
#endif
#endif
}

bool NixMusicIsPlaying(int index)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return false;
		FILTER_STATE fs;
		Music[index]->GetState(0,&fs);
		if (fs==State_Running)
			return true;
	}
#endif
#endif
#endif
	return false;
}

bool NixMusicEnded(int index)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return false;
		LONGLONG Dur,Pos;
		/*MusicMediaSeeking[index]->GetDuration(&Dur);
		MusicMediaSeeking[index]->GetCurrentPosition(&Pos);*/
		MusicMediaSeeking[0]->GetDuration(&Dur);
		MusicMediaSeeking[0]->GetCurrentPosition(&Pos);
		if (Pos>=Dur)
			return true;
	}
#endif
#endif
#endif
	return false;
}

void NixMusicTestRepeat()
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		/*FILTER_STATE fs;
		for (int m=0;m<NumMusics;m++){
			Music[m]->GetState(0,&fs);
			LONGLONG Dur,Pos;
			MusicMediaSeeking[m]->GetDuration(&Dur);
			MusicMediaSeeking[m]->GetCurrentPosition(&Pos);
			if ((Pos>=Dur)&&(MusicRepeat[m]))
				NixMusicPlay(m,true);
		}
		SetTimer(_HuiWindow_->hWnd,666,1000,&MyMusicTimerProc);*/
	}
#endif
#endif
}

#endif

