/*----------------------------------------------------------------------------*\
| Nix input                                                                    |
| -> user input (mouse/keyboard                                                |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
//#include "nix_common.h"

#ifdef OS_LINUX
	#include <X11/keysym.h>
	#include <gdk/gdkx.h>
#endif

vector NixMouse, NixMouseRel, NixMouseD, NixMouseDRel;


bool AllowWindowsKeyInput = false;

bool NixMouseStolen = false;
int NixStealMouseDx = 0;
int NixStealMouseDy = 0;

static Array<HuiEvent> NixInputEvent;
static vector _NixMouseDSum;
int NixKeyRep;

extern string NixControlID;
HuiInputData NixInputAccum;

void NixOnEvent()
{
	HuiEvent *e = HuiGetEvent();
	//msg_write(e->message);
	_NixMouseDSum += vector(e->dx, e->dy, e->dz);
	if (e->message == "hui:mouse-move"){
		NixInputAccum.x = e->mx;
		NixInputAccum.y = e->my;
		//msg_write(format("m:  %d  %d", e->mx, e->my));
		//msg_write(format("d:  %d  %d", e->dx, e->dy));
		if (NixMouseStolen){
			int x0 = MaxX / 2;
			int y0 = MaxY / 2;
			int dx = (e->mx - x0);
			int dy = (e->my - y0);
			if ((abs(dx) > 100) || (abs(dy) > 100)){
				//msg_write(format("S:  %d  %d", x0, y0));
				//msg_write("---shift");
				NixWindow->SetCursorPos(x0, y0);
			}
		}
	}else if (e->message == "hui:key-down"){
		NixInputAccum.key[e->key] = true;
	}else if (e->message == "hui:key-up"){
		NixInputAccum.key[e->key] = false;
	}
	NixInputEvent.add(*e);
}

HuiInputData NixInputDataCurrent, NixInputDataLast;

void NixInputInit()
{
	//NixWindow->AllowEvents("key,mouse,scroll,draw");

	NixInputDataCurrent.reset();
	NixInputDataLast.reset();

	NixWindow->EventX(NixControlID, "hui:mouse-move", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:mouse-wheel", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:key-down", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:key-up", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:left-button-down", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:left-button-up", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:middle-button-down", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:middle-button-up", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:right-button-down", &NixOnEvent);
	NixWindow->EventX(NixControlID, "hui:right-button-up", &NixOnEvent);
}

void NixStealMouse(bool steal)
{
	NixMouseStolen = steal;
	NixWindow->ShowCursor(!steal);

	NixWindow->SetCursorPos(MaxX / 2, MaxY / 2);
}

#if 0
// Eingaben vonm Fenster entgegennehmen
void NixGetInputFromWindow()
{
	if (!NixWindow)
		return;

	#ifdef HUI_API_WIN
		int i;
		POINT mpos;
		UINT message=NixWindow->CompleteWindowMessage.msg;		NixWindow->CompleteWindowMessage.msg=0;
		WPARAM wParam=NixWindow->CompleteWindowMessage.wparam;	NixWindow->CompleteWindowMessage.wparam=0;
		LPARAM lParam=NixWindow->CompleteWindowMessage.lparam;	NixWindow->CompleteWindowMessage.lparam=0;
		//NixWindow->InputData=NixInputDataCurrent;

		int mycd=0;
		RECT ToolBarRect;

		switch(message){
			case WM_MOUSEMOVE:
				// correction due to toolbar?
				if (NixWindow->tool_bar[HuiToolBarTop].Enabled){
					//SendMessage(NixWindow->tool_bar[HuiToolBarTop].hWnd,TB_AUTOSIZE,0,0);
					GetWindowRect(NixWindow->tool_bar[HuiToolBarTop].hWnd,&ToolBarRect);
					mycd=-ToolBarRect.bottom+ToolBarRect.top;
				}
				if (NixFullscreen){
					GetCursorPos(&mpos);
					NixWindow->InputData.dx+=(float)mpos.x-NixScreenWidth/2.0f;
					NixWindow->InputData.dy+=(float)mpos.y-NixScreenHeight/2.0f;
					SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
					// korrekte Mausposition
					/*NixWindow->InputData.mx=NixInputDataCurrent.mx+NixWindow->InputData.vx;
					NixWindow->InputData.my=NixInputDataCurrent.my+NixWindow->InputData.vy;*/
					// praktischere Mausposition
					NixWindow->InputData.x=NixInputDataCurrent.x+float(NixWindow->InputData.dx)/NixMouseMappingWidth*float(NixScreenWidth);
					NixWindow->InputData.y=NixInputDataCurrent.y+float(NixWindow->InputData.dy)/NixMouseMappingHeight*float(NixScreenHeight);
				}else{
					NixWindow->InputData.x=(float)LOWORD(lParam);
					NixWindow->InputData.y=(float)HIWORD(lParam)+mycd;
					if (NixWindow->InputData.x>32000)			NixWindow->InputData.x=0;
					if (NixWindow->InputData.y>32000)			NixWindow->InputData.y=0;
				}
				if (NixWindow->InputData.x<0)				NixWindow->InputData.x=0;
				if (NixWindow->InputData.y<0)				NixWindow->InputData.y=0;
				if (NixWindow->InputData.x>NixTargetWidth)	NixWindow->InputData.x=(float)NixTargetWidth;
				if (NixWindow->InputData.y>NixTargetHeight)	NixWindow->InputData.y=(float)NixTargetHeight;
				if (!NixFullscreen){
					NixWindow->InputData.dx=NixWindow->InputData.x-NixInputDataLast.x;
					NixWindow->InputData.dy=NixWindow->InputData.y-NixInputDataLast.y;
				}
				break;
			case WM_MOUSEWHEEL:
				NixWindow->InputData.dz+=(short)HIWORD(wParam);
				break;
			case WM_LBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.lb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_LBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.lb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.mb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.mb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.rb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.rb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_KEYDOWN:
				if (GetActiveWindow()==NixWindow->hWnd){
					AllowWindowsKeyInput=true;
					if (NixWindow->InputData.KeyBufferDepth>=HUI_MAX_KEYBUFFER_DEPTH-1){
						for (i=0;i<NixWindow->InputData.KeyBufferDepth-2;i++)
							NixWindow->InputData.KeyBuffer[i]=NixWindow->InputData.KeyBuffer[i+1];
						NixWindow->InputData.KeyBufferDepth--;
					}
					NixWindow->InputData.KeyBuffer[NixWindow->InputData.KeyBufferDepth]=HuiKeyID[wParam];
					NixWindow->InputData.KeyBufferDepth++;
				}
				break;
			/*case WM_KEYDOWN:
				key[wParam]=true;
				break;
			case WM_KEYUP:
				key[wParam]=false;
				break;*/
			/*case WM_SIZE:
				NixResize();
				break;*/
		}
		NixWindow->InputData.mw=(float)atan2(-NixWindow->InputData.dy-NixInputDataLast.dy,NixWindow->InputData.dx+NixInputDataLast.dx);
		if (NixWindow->InputData.mw<0)
			NixWindow->InputData.mw+=2*pi;

		if (GetActiveWindow()!=NixWindow->hWnd)
			AllowWindowsKeyInput=false;
	
		if (AllowWindowsKeyInput)
			for (i=0;i<256;i++)
				NixWindow->InputData.key[HuiKeyID[i]]=((GetAsyncKeyState(i)&(1<<15))!=0);
		else{
			for (i=0;i<256;i++)
				NixWindow->InputData.key[i]=false;
		}

		// Korrektur (manche Tasten belegen mehrere Array-Elemente) :-S
		if (NixGetKey(KEY_RALT))
			NixWindow->InputData.key[KEY_LCONTROL]=0;
	#endif
}

bool allow_mb=false;
int gdk_mx=0,gdk_my=0;
#endif

// Eingaben behandeln
void NixUpdateInput()
{
	NixInputDataLast = NixInputDataCurrent;
	NixKeyRep = -2;
	foreach(HuiEvent &e, NixInputEvent){
		if (e.message == "hui:key-down")
			NixKeyRep = e.key;
	}

	if (NixMouseStolen){
		// copy (but keep x,y)
		float mx0 = NixInputDataCurrent.x;
		float my0 = NixInputDataCurrent.y;
		NixInputDataCurrent = NixWindow->input;
		NixInputDataCurrent.x = mx0;
		NixInputDataCurrent.y = my0;

		NixInputDataCurrent.dx = _NixMouseDSum.x;
		NixInputDataCurrent.dy = _NixMouseDSum.y;
		NixInputDataCurrent.dz = _NixMouseDSum.z;
		NixInputDataCurrent.x += _NixMouseDSum.x;
		NixInputDataCurrent.y += _NixMouseDSum.y;
		NixInputDataCurrent.x = clampf(NixInputDataCurrent.x, 0, NixTargetWidth - 1);
		NixInputDataCurrent.y = clampf(NixInputDataCurrent.y, 0, NixTargetHeight - 1);
		//msg_write(format("%f  %f", NixInputDataCurrent.x, NixWindow->input.x));
		memcpy(NixInputDataCurrent.key, NixInputAccum.key, sizeof(NixInputAccum.key));
	}else
		NixInputDataCurrent = NixInputAccum;

	NixInputEvent.clear();
	NixWindow->input.dx = NixWindow->input.dy = 0;
	
	
	NixMouse = vector(NixInputDataCurrent.x, NixInputDataCurrent.y, 0);
	NixMouseRel = vector((float)NixInputDataCurrent.x / (float)NixTargetWidth, (float)NixInputDataCurrent.y / (float)NixTargetHeight, 0);
	NixMouseD = vector(NixInputDataCurrent.dx, NixInputDataCurrent.dy, NixInputDataCurrent.dz);
	NixMouseDRel = vector((float)NixInputDataCurrent.dx / (float)NixTargetWidth, (float)NixInputDataCurrent.dy / (float)NixTargetHeight, NixInputDataCurrent.dz);

	_NixMouseDSum = v_0;
}

void NixResetInput()
{
	NixUpdateInput();
	NixInputDataCurrent.reset();
	NixInputDataCurrent.x = NixTargetWidth / 2;
	NixInputDataCurrent.y = NixTargetHeight / 2;
	NixInputDataLast = NixInputDataCurrent;
}



void NixResetCursor()
{
#if 0
	if (NixFullscreen){
		#ifdef OS_WINDOWS
			SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
		#endif
		NixWindow->InputData.x=NixInputDataCurrent.x=NixScreenWidth/2.0f;
		NixWindow->InputData.y=NixInputDataCurrent.y=NixScreenHeight/2.0f;
	}
#endif
}

bool NixGetButton(int but)
{
	if (but == 0)
		return NixInputDataCurrent.lb;
	if (but == 1)
		return NixInputDataCurrent.mb;
	if (but == 2)
		return NixInputDataCurrent.rb;
	return false;
}

bool NixGetButtonDown(int but)
{
	if (but == 0)
		return NixInputDataCurrent.lb && !NixInputDataLast.lb;
	if (but == 1)
		return NixInputDataCurrent.mb && !NixInputDataLast.mb;
	if (but == 2)
		return NixInputDataCurrent.rb && !NixInputDataLast.rb;
	return false;
}

bool NixGetButtonUp(int but)
{
	if (but == 0)
		return !NixInputDataCurrent.lb && NixInputDataLast.lb;
	if (but == 1)
		return !NixInputDataCurrent.mb && NixInputDataLast.mb;
	if (but == 2)
		return !NixInputDataCurrent.rb && NixInputDataLast.rb;
	return false;
}

bool NixGetKey(int key)
{
	return NixWindow->GetKey(key);
}

bool NixGetKeyDown(int key)
{
	if (key == KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if (NixInputDataCurrent.key[i] && !NixInputDataLast.key[i])
				return true;
		return false;
	}else
		return NixInputDataCurrent.key[key] && !NixInputDataLast.key[key];
}

int NixGetKeyDownRep()
{
	return NixKeyRep;
}

bool NixGetKeyUp(int key)
{
	if (key == KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if (!NixInputDataCurrent.key[i] && NixInputDataLast.key[i])
				return true;
		return false;
	}else
		return !NixInputDataCurrent.key[key] && NixInputDataLast.key[key];
}

string NixGetKeyChar(int key)
{
	return HuiGetKeyChar(key);
}


