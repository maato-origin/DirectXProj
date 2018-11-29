#pragma once

#include <Windows.h>
#include <DirectXMath.h>

struct MouseCapture
{
	struct POINT
	{
		POINT(int a = 0,int b=0): x(a), y(b){}
		int x, y;
	};

	FLOAT wheel = 0;
	POINT cursorPt = POINT(-10000, -10000);
};

//キーボード入力
struct KeyInput
{
	typedef MouseCapture::POINT POINT;
	static const int KEY_ARRAY_NUM = 256;
	BYTE aKeyboard[KEY_ARRAY_NUM];
	BYTE aKeyboardOld[KEY_ARRAY_NUM];

	FLOAT wheel = 0.0f;
	POINT cursorPt = POINT(-10000, -10000);	//現在位置
	POINT lastPt = POINT(-10000, -10000);	//一つ前の位置
	POINT lClickPt = POINT(-10000, -10000);	//Lクリック位置
	POINT mClickPt = POINT(-10000, -10000);	//Mクリック位置
	POINT rClickPt = POINT(-10000, -10000);	//Rクリック位置

	KeyInput()
	{
		reset();
	}

	void reset();
	bool update(const MouseCapture& mouse);
	bool update(const MouseCapture& mouse, BYTE keyArray[KEY_ARRAY_NUM]);
	bool keyDown(int vk) const;
	bool keyOn(int vk) const;
	bool keyUp(int vk) const;
};

//カメラ操作
struct CameraCtrl
{
	DirectX::XMVECTOR at = DirectX::XMVectorSet(0, 5, 0, 0);
	FLOAT rotH = DirectX::XM_PI;
	FLOAT rotV = -0.5f;
	FLOAT dist = 50.0f;

	FLOAT speedRotH = 1.5f;
	FLOAT speedRotV = 1.0f;
	FLOAT limitRotV = 89.0f / 180.0f*DirectX::XM_PI;
	FLOAT distMin = 0.5f;
	FLOAT distMax = 200.0f;
	FLOAT speedDist = 20.0f;
	FLOAT speedMove = 20.0f;
	FLOAT speedMouseRotation = 0.01f;
	FLOAT speedWheelMove = 3.0f;
	FLOAT speedMouseSlide = 0.15f;

	DirectX::XMVECTOR iniAt = DirectX::XMVectorSet(0, 5, 0, 0);
	FLOAT iniRotH = DirectX::XM_PI;
	FLOAT iniRotV = -0.5f;
	FLOAT iniDist = 50.0f;

	bool bCharacterMode = false;
	DirectX::XMVECTOR vCharacterPos = DirectX::XMVectorSet(0, 0, 0, 1);
	DirectX::XMVECTOR vCharacterRot = DirectX::XMVectorSet(0, 0, 0, 0);

	void reset();
	void ctrl(const KeyInput& key, FLOAT elapsed_time_sec);

	DirectX::XMMATRIX getViewMatrix();
};