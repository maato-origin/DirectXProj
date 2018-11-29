#include "controller.h"
#include "stdio.h"

using namespace DirectX;

void KeyInput::reset()
{
	for (int i = 0; i < KEY_ARRAY_NUM; ++i)
	{
		aKeyboard[i] = 0;
		aKeyboardOld[i] = 0;
	}
}

bool KeyInput::update(const MouseCapture& mouse)
{
	BYTE keyArray[KEY_ARRAY_NUM];
	if (!GetKeyboardState(keyArray))
	{
		for (int i = 0; i < KEY_ARRAY_NUM; ++i)
		{
			keyArray[i] = 0;
		}
		return false;
	}
	return update(mouse, keyArray);
}

bool KeyInput::update(const MouseCapture& mouse, BYTE keyArray[KEY_ARRAY_NUM])
{
	for (int i = 0; i < KEY_ARRAY_NUM; ++i)
	{
		aKeyboardOld[i] = aKeyboard[i];
	}
	for (int i = 0; i < KEY_ARRAY_NUM; ++i)
	{
		aKeyboard[i] = keyArray[i];
	}

	wheel = mouse.wheel;
	lastPt = cursorPt;
	cursorPt = mouse.cursorPt;
	if (keyDown(VK_LBUTTON)) lClickPt = cursorPt;
	if (keyDown(VK_MBUTTON)) mClickPt = cursorPt;
	if (keyDown(VK_RBUTTON)) rClickPt = cursorPt;

	return true;
}

bool KeyInput::keyDown(int vk) const
{
	if ((unsigned)vk >= KEY_ARRAY_NUM)
		return false;

	bool now = (aKeyboard[vk] & 0x80) != 0;
	bool old = (aKeyboardOld[vk] & 0x80) != 0;
	return now && !old;
}

bool KeyInput::keyOn(int vk) const
{
	if ((unsigned)vk >= KEY_ARRAY_NUM)
		return false;

	return (aKeyboard[vk] & 0x80) != 0;
}

bool KeyInput::keyUp(int vk) const
{
	if ((unsigned)vk >= KEY_ARRAY_NUM)
		return false;

	bool now = (aKeyboard[vk] & 0x80) != 0;
	bool old = (aKeyboardOld[vk] & 0x80) != 0;

	return !now && old;
}

DirectX::XMMATRIX CameraCtrl::getViewMatrix()
{
	XMVECTOR cameraPos = { 0,0,dist,1 };
	XMMATRIX cameraRot = XMMatrixRotationRollPitchYaw(rotV, rotH, 0);
	cameraPos = XMVector4Transform(cameraPos, cameraRot);
	XMVECTOR lookAt = at;
	if (bCharacterMode)
	{
		lookAt += vCharacterPos;
	}

	XMVECTOR eye = XMVectorAdd(cameraPos, lookAt);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	return XMMatrixLookAtLH(eye, lookAt, up);
}

void CameraCtrl::reset()
{
	at = iniAt;
	rotH = iniRotH;
	rotV = iniRotV;
	dist = iniDist;
	vCharacterPos = DirectX::XMVectorSet(0, 0, 0, 1);
	vCharacterRot = DirectX::XMVectorSet(0, 0, 0, 0);
}

namespace 
{
	//-ƒÎ`+ƒÎ
	FLOAT subAngle(FLOAT a0, FLOAT a1)
	{
		FLOAT sub = a0 - a1;
		if (sub > XM_PI)
			sub -= XM_2PI;
		if (sub < -XM_PI)
			sub += XM_2PI;

		return sub;
	}
	FLOAT angleLimit(FLOAT a)
	{
		int i = (int)(a / XM_2PI);
		a = a - FLOAT(i)*XM_2PI;
		if (a > XM_PI)
			a -= XM_2PI;
		if (a < -XM_PI)
			a += XM_2PI;

		return a;
	}
}

void CameraCtrl::ctrl(const KeyInput& key_input, FLOAT elapsed_time_sec)
{
	static bool last_move_camera = false;
	bool move_camera = false;

	if (key_input.keyOn(VK_RBUTTON))
	{
		move_camera = true;
		FLOAT dx = static_cast<FLOAT>(key_input.cursorPt.x - key_input.lastPt.x);
		FLOAT dy = static_cast<FLOAT>(key_input.cursorPt.y - key_input.lastPt.y);
		rotH += speedMouseRotation * dx;
		rotV += speedMouseRotation * dy;
		if (rotH > XM_PI)
			rotH -= XM_2PI;
		if (rotH < -XM_PI)
			rotH += XM_2PI;
		if (rotV > limitRotV)
			rotV = limitRotV;
		if (rotV < -limitRotV)
			rotV = -limitRotV;
	}
	if (key_input.wheel != 0.0f)
	{
		move_camera = true;
		dist -= speedWheelMove * key_input.wheel;
		if (dist < distMin)
			dist = distMin;
		if (dist > distMax)
			dist = distMax;
	}
	if (key_input.keyOn(VK_MBUTTON))
	{
		move_camera = true;
		XMMATRIX view = getViewMatrix();
		view = XMMatrixInverse(nullptr, view);
		FLOAT dx = static_cast<FLOAT>(key_input.cursorPt.x - key_input.lastPt.x);
		FLOAT dy = static_cast<FLOAT>(key_input.cursorPt.y - key_input.lastPt.y);
		at -= view.r[0] * dx*speedMouseSlide;
		at += view.r[1] * dy*speedMouseSlide;
	}
	if (key_input.keyOn(VK_RIGHT))
	{
		move_camera = true;
		rotH += speedRotH * elapsed_time_sec;
		if (rotH > XM_PI)
			rotH -= XM_2PI;
	}
	if (key_input.keyOn(VK_LEFT))
	{
		move_camera = true;
		rotH -= speedRotH * elapsed_time_sec;
		if (rotH < -XM_PI)
			rotH += XM_2PI;
	}
	if (key_input.keyOn(VK_DOWN))
	{
		move_camera = true;
		rotV += speedRotV * elapsed_time_sec;
		if (rotV > limitRotV)
			rotV = limitRotV;
	}
	if (key_input.keyOn(VK_UP))
	{
		move_camera = true;
		rotV -= speedRotV * elapsed_time_sec;
		if (rotV < -limitRotV)
			rotV = -limitRotV;
	}
	if (key_input.keyOn(VK_PRIOR))		//page up
	{
		move_camera = true;
		dist -= speedDist * elapsed_time_sec;
		if (dist < distMin)
			dist = distMin;
	}
	if (key_input.keyOn(VK_NEXT))		//page down
	{
		move_camera = true;
		dist += speedDist * elapsed_time_sec;
		if (dist > distMax)
			dist = distMax;
	}

	XMVECTOR move = { 0,0,0,0 };
	if (key_input.keyOn('W'))
		move = XMVectorSetZ(move, -1.0f);
	if (key_input.keyOn('S'))
		move = XMVectorSetZ(move, 1.0f);
	if (key_input.keyOn('D'))
		move = XMVectorSetX(move, -1.0f);
	if (key_input.keyOn('A'))
		move = XMVectorSetX(move, 1.0f);
	if (key_input.keyOn('C'))
		move = XMVectorSetY(move, -1.0f);
	if (key_input.keyOn('E'))
		move = XMVectorSetY(move, 1.0f);

	if (XMVectorGetX(XMVector3Length(move)) > 0.0f)
	{
		move_camera = true;
		move = XMVector3Normalize(move);
		move = XMVectorScale(move, speedMove*elapsed_time_sec);
		move = XMVector3Transform(move, XMMatrixRotationY(rotH));
		if (key_input.keyOn(VK_LSHIFT))
			move *= 2.0f;

		if (bCharacterMode)
		{
			vCharacterPos += move;

			FLOAT move_angle = atan2f(-XMVectorGetX(move), -XMVectorGetZ(move));
			FLOAT now_angle = XMVectorGetY(vCharacterRot);
			FLOAT sub_angle = subAngle(move_angle, now_angle);
			const FLOAT rot_speed = 0.25f;
			if (sub_angle > rot_speed)
				sub_angle = rot_speed;
			if (sub_angle < rot_speed)
				sub_angle = -rot_speed;
			now_angle = angleLimit(now_angle + sub_angle);

			vCharacterRot = XMVectorSet(0, now_angle, 0, 0);
		}
		else
		{
			at = XMVectorAdd(at, move);
		}
	}
#if 0
	if (!move_camera && last_move_camera)
	{
		//‘€ìI—¹
		char camera[256];
		sprintf_s(camera, "\tINI_ROT %f, %f;\n\tINI_LOOKAT %f, %f, %f;\n\tINI_DIST %f;\n//-----\n",
			rotH*180.0f / XM_PI, rotV*180.0f / XM_PI,
			XMVectorGetX(at), XMVectorGetY(at), XMVectorGetZ(at),
			dist);
		OutputDebugString(camera);
	}
	last_move_camera = move_camera;
#endif
}