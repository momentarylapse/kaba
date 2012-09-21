/*----------------------------------------------------------------------------*\
| Nix input                                                                    |
| -> user input (mouse/keyboard)                                               |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_INPUT_EXISTS_
#define _NIX_INPUT_EXISTS_

void NixInputInit();

void _cdecl NixStealMouse(bool steal);
void _cdecl NixUpdateInput();
void _cdecl NixResetInput();

// keys
bool _cdecl NixGetKey(int key);
bool _cdecl NixGetKeyDown(int key);
bool _cdecl NixGetKeyDownRep(int key);
bool _cdecl NixGetKeyUp(int key);
string _cdecl NixGetKeyChar(int key);

// mouse
float _cdecl NixGetMDir();
void _cdecl NixResetCursor();
bool _cdecl NixGetButton(int but);
bool _cdecl NixGetButtonDown(int but);
bool _cdecl NixGetButtonUp(int but);

extern vector NixMouse, NixMouseRel, NixMouseD, NixMouseDRel;


#endif
