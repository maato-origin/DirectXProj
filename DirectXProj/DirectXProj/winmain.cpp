
#include <Windows.h>
#include <crtdbg.h>
#include <d3d11.h>
#pragma comment(lib,"d3d11.lib")

#include <memory>

#include "graphics.h"
#include "controller.h"

HWND g_hWnd = NULL;
LARGE_INTEGER countTimer = { 0 };
MouseCapture MouseCap;

HRESULT initWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//メイン関数
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);	//引数未使用のWarning対策
	UNREFERENCED_PARAMETER(lpCmdLine);

	//メモリリークデバッグ
#ifndef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // !_DEBUG

	//ウィンドウ作成
	if (FAILED(initWindow(hInstance, nCmdShow)))
	{
		return 0;	//失敗
	}

	std::unique_ptr<Graphics> graphics(new Graphics);

	//DirectX初期化
	if (FAILED(graphics->init(g_hWnd)))
	{
		return 0;
	}

	//時間計測用カウンター
	LARGE_INTEGER countfreq;
	QueryPerformanceFrequency(&countfreq);	//カウンター周波数
	QueryPerformanceCounter(&countTimer);	//カウンター取得

	//メッセージループ
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			switch (msg.message)
			{
			case WM_MOUSEMOVE:
				MouseCap.cursorPt.x = LOWORD(msg.lParam);
				MouseCap.cursorPt.y = HIWORD(msg.lParam);
				break;
			case WM_MOUSEWHEEL:
				MouseCap.wheel = GET_WHEEL_DELTA_WPARAM(msg.wParam) / 120.0f;
				break;
			}
		}
		else
		{
			//更新処理
			graphics->update(MouseCap);
			//表示処理
			graphics->render();

			MouseCap.wheel = 0.0f;

			//60FPS固定
			const float fps = 60.0f;
			while (1)
			{
				LARGE_INTEGER count;
				QueryPerformanceCounter(&count);
				long long countTime = count.QuadPart - countTimer.QuadPart;	//経過時間(カウント)
				float time = float(double(countTime) / double(countfreq.QuadPart));		//経過時間(秒)
				if (time >= 1.0f / fps)
				{
					countTimer = count;
					break;
				}
				else
				{
					//待機時間
					float waitTime = 1.0f / fps - time;
					if (waitTime > 0.002f)
					{
						int waitMillitime = static_cast<int>(waitTime*1000.0f);
						Sleep(DWORD(waitMillitime - 1));
						//待ち時間が1ミリ秒以内になるまで待つ
					}
				}
			}
		}
	}

	return 1;
}

HRESULT initWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "DirectXGraphics";
	wcex.hIconSm = NULL;

	if (!RegisterClassEx(&wcex))
	{
		return E_FAIL;
	}

	//ウィンドウのクライアント領域を指定
	RECT rc = { 0,0,1280,720 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	g_hWnd = CreateWindow("DirectXGraphics", "DirectX11Graphics Engine", WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
							NULL, NULL, hInstance, NULL);
	if (!g_hWnd)
	{
		return E_FAIL;
	}

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}