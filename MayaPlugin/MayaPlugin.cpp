// MayaPlugin.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "MayaPlugin.h"
#include "MayaController.h"
#include "Miscellaneous.h"
#include "I4C3DCommon.h"
#include <WinSock2.h>
#include <ShellAPI.h>

#define MAX_LOADSTRING	100
#define TIMER_ID		1

static const int BUFFER_SIZE = 256;
static const PCSTR COMMAND_INIT	= "init";
static const PCSTR COMMAND_EXIT	= "exit";

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名
static USHORT g_uPort = 0;

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

static int AnalyzeMessage(LPCSTR szMessage, LPSTR szCommand, SIZE_T size, double* pDeltaX, double* pDeltaY, char cTermination);

static SOCKET InitializeController(HWND hWnd, USHORT uPort);
static void UnInitializeController(SOCKET socketHandler);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MAYAPLUGIN, szWindowClass, MAX_LOADSTRING);

	if (!ExecuteOnce(szTitle)) {
		return EXIT_SUCCESS;
	}

	MyRegisterClass(hInstance);

	//int argc = 0;
	//LPTSTR *argv = NULL;
	//argv = CommandLineToArgvW(GetCommandLine(), &argc);
	//if (argc != 2) {
	//	MessageBox(NULL, _T("[ERROR] 引数が足りません[例: MayaPlugin.exe 10001]。<MayaPlugin>"), szTitle, MB_OK | MB_ICONERROR);
	//}
	//g_uPort = static_cast<USHORT>(_wtoi(argv[1]));
	//OutputDebugString(argv[1]);
	//LocalFree(argv);
	g_uPort = 10002;

	static WSAData wsaData;
	WORD wVersion;
	int nResult;

	wVersion = MAKEWORD(2,2);
	nResult = WSAStartup(wVersion, &wsaData);
	if (nResult != 0) {
		MessageBox(NULL, _T("[ERROR] Initialize Winsock."), szTitle, MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (wsaData.wVersion != wVersion) {
		MessageBox(NULL, _T("[ERROR] Winsock バージョン."), szTitle, MB_OK | MB_ICONERROR);
		WSACleanup();
		return FALSE;
	}

#if _DEBUG || DEBUG
	LogInitialize(Log_Debug);
#else
	LogInitialize(Log_Error);
#endif

	// アプリケーションの初期化を実行します:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAYAPLUGIN));

	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();
	CleanupMutex();
	return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
//  コメント:
//
//    この関数および使い方は、'RegisterClassEx' 関数が追加された
//    Windows 95 より前の Win32 システムと互換させる場合にのみ必要です。
//    アプリケーションが、関連付けられた
//    正しい形式の小さいアイコンを取得できるようにするには、
//    この関数を呼び出してください。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAYAPLUGIN));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDI_MAYAPLUGIN);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	const int timer_interval = 100;
	static DWORD counter = 0;
	static BOOL doCount = FALSE;

	static MayaController controller;
	static SOCKET socketHandler = INVALID_SOCKET;
	char buffer[BUFFER_SIZE*4] = {0};
	char szCommand[32] = {0};
	double deltaX = 0;
	double deltaY = 0;
	static char cTermination = '?';

	int nBytes = 0;
	TCHAR szError[BUFFER_SIZE] = {0};

	switch (message)
	{
	case WM_CREATE:
		socketHandler = InitializeController(hWnd, g_uPort);
		SetTimer(hWnd, TIMER_ID, timer_interval, NULL);
		break;

	case MY_WINSOCKSELECT:
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			nBytes = recv(socketHandler, buffer, _countof(buffer), 0);
			if (nBytes == SOCKET_ERROR) {
				_stprintf_s(szError, _countof(szError), _T("recv() : %d <MayaPlugin>"), WSAGetLastError());
				LogDebugMessage(Log_Error, szError);
				//ReportError(szError);
				break;

			}

			int scanCount = AnalyzeMessage(buffer, szCommand, _countof(szCommand), &deltaX, &deltaY, cTermination);
			if (scanCount == 3) {
				counter = 0;
				controller.Execute(szCommand, deltaX, deltaY);
				Sleep(1);
				doCount = TRUE;

			} else if (scanCount == 1) {
				if (_strcmpi(szCommand, COMMAND_INIT) == 0) {
					if (!controller.Initialize(buffer, &cTermination)) {
						_stprintf_s(szError, _countof(szError), _T("Mayaコントローラの初期化に失敗しています。"));
						ReportError(szError);
					}
				} else if (_strcmpi(szCommand, COMMAND_EXIT) == 0) {
					OutputDebugString(_T("exit\n"));
					DestroyWindow(hWnd);
				}
			}
		}
		break;

	case WM_TIMER:
		if (doCount) {
			counter += timer_interval;
			if (300 < counter) {
				controller.ModKeyUp();
				doCount = FALSE;
			}
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;

	case MY_I4C3DREBOOT:
		UnInitializeController(socketHandler);
		socketHandler = InitializeController(hWnd, g_uPort);
		break;

	case MY_I4C3DDESTROY:
	case WM_DESTROY:
		UnInitializeController(socketHandler);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

SOCKET InitializeController(HWND hWnd, USHORT uPort)
{
	SOCKET socketHandler;
	SOCKADDR_IN address;
	TCHAR szError[BUFFER_SIZE];
	int nResult = 0;

	socketHandler = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		ReportError(szError);
		LogDebugMessage(Log_Error, szError);
		return INVALID_SOCKET;
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(uPort);
	address.sin_addr.S_un.S_addr = INADDR_ANY;

	nResult = bind(socketHandler, (const SOCKADDR*)&address, sizeof(address));
	if (nResult == SOCKET_ERROR) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] bind() : %d"), WSAGetLastError());
		ReportError(szError);
		LogDebugMessage(Log_Error, szError);
		closesocket(socketHandler);
		return INVALID_SOCKET;
	}

	if (WSAAsyncSelect(socketHandler, hWnd, MY_WINSOCKSELECT, FD_READ) == SOCKET_ERROR) {
		TCHAR* szError = _T("ソケットイベント通知設定に失敗しました。<MayaPlugin::InitializeController>");
		ReportError(szError);
		LogDebugMessage(Log_Error, szError);
	}

	return socketHandler;
}

void UnInitializeController(SOCKET socketHandler)
{
	closesocket(socketHandler);
}

int AnalyzeMessage(LPCSTR szMessage, LPSTR szCommand, SIZE_T size, double* pDeltaX, double* pDeltaY, char cTermination)
{
	static char szFormat[BUFFER_SIZE/4] = {0};
	int scanCount = 0;
	double deltaX = 0., deltaY = 0.;
	if (szFormat[0] == '\0') {
		sprintf_s(szFormat, sizeof(szFormat), "%%s %%lf %%lf%c", cTermination);
	}
	
	scanCount = sscanf_s(szMessage, szFormat, szCommand, size, &deltaX, &deltaY);
	if (3 <= scanCount) {
		*pDeltaX = deltaX;
		*pDeltaY = deltaY;
	}
	return scanCount;
}
