#pragma once

#include "VirtualMotion.h"
#include "CADController.h"
#include "MacroPlugin.h"

class MayaController : public CADController, public MacroPlugin
{
public:
	MayaController(void);
	virtual ~MayaController(void);

	BOOL Initialize(LPCSTR szBuffer, char* termination);
	virtual void Execute(HWND hWnd, LPCSTR szCommand, double deltaX, double deltaY);
	void ModKeyUp(void);

private:
	virtual void TumbleExecute(int deltaX, int deltaY);
	virtual void TrackExecute(int deltaX, int deltaY);
	virtual void DollyExecute(int deltaX, int deltaY);

	BOOL InitializeModifierKeys(PCSTR szModifierKeys);
	BOOL GetTargetChildWnd(void);
	//BOOL CheckTargetState(void);
	void AdjustCursorPos(void);
	void ModKeyDown(void);
	BOOL IsModKeysDown(void);

	HWND m_hTargetTopWnd;
	HWND m_hMouseInputWnd;
	HWND m_hKeyInputWnd;

	VMMouseMessage m_mouseMessage;
	POINT m_currentPos;
	BOOL m_ctrl;
	BOOL m_alt;
	BOOL m_shift;
	BOOL m_bUsePostMessageToSendKey;
	BOOL m_bUsePostMessageToMouseDrag;
	BOOL m_bSyskeyDown;
	int m_DisplayWidth;
	int m_DisplayHeight;
	double m_fTumbleRate;
	double m_fTrackRate;
	double m_fDollyRate;
};

