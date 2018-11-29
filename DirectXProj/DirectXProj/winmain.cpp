
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

//���C���֐�
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);	//�������g�p��Warning�΍�
	UNREFERENCED_PARAMETER(lpCmdLine);

	//���������[�N�f�o�b�O
#ifndef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // !_DEBUG

	//�E�B���h�E�쐬
	if (FAILED(initWindow(hInstance, nCmdShow)))
	{
		return 0;	//���s
	}

	std::unique_ptr<Graphics> graphics(new Graphics);

	//DirectX������
	if (FAILED(graphics->init(g_hWnd)))
	{
		return 0;
	}

	//���Ԍv���p�J�E���^�[
	LARGE_INTEGER countfreq;
	QueryPerformanceFrequency(&countfreq);	//�J�E���^�[���g��
	QueryPerformanceCounter(&countTimer);	//�J�E���^�[�擾

	//���b�Z�[�W���[�v
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
			//�X�V����
			graphics->update(MouseCap);
			//�\������
			graphics->render();

			MouseCap.wheel = 0.0f;

			//60FPS�Œ�
			const float fps = 60.0f;
			while (1)
			{
				LARGE_INTEGER count;
				QueryPerformanceCounter(&count);
				long long countTime = count.QuadPart - countTimer.QuadPart;	//�o�ߎ���(�J�E���g)
				float time = float(double(countTime) / double(countfreq.QuadPart));		//�o�ߎ���(�b)
				if (time >= 1.0f / fps)
				{
					countTimer = count;
					break;
				}
				else
				{
					//�ҋ@����
					float waitTime = 1.0f / fps - time;
					if (waitTime > 0.002f)
					{
						int waitMillitime = static_cast<int>(waitTime*1000.0f);
						Sleep(DWORD(waitMillitime - 1));
						//�҂����Ԃ�1�~���b�ȓ��ɂȂ�܂ő҂�
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

	//�E�B���h�E�̃N���C�A���g�̈���w��
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