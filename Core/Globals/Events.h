#pragma once
#include "KeyCodes.h"

class EventArgs
{
public:
	EventArgs() {}
};

class KeyEvent : public EventArgs
{
public:
	enum KeyState
	{
		Released = 0,
		Pressed = 1
	};

	typedef EventArgs base;

	KeyEvent(KeyCode::Key key, unsigned int c, KeyState state, bool ctrl, bool shift, bool alt)
		: Key(key)
		, Char(c)
		, State(state)
		, Ctrl(ctrl)
		, Shift(shift)
		, Alt(alt)
	{}

	KeyCode::Key Key;
	unsigned int Char;
	KeyState State;
	bool Ctrl;
	bool Shift;
	bool Alt;
};

class MouseMotionEvent : public EventArgs
{
public:
	typedef EventArgs base;

	MouseMotionEvent(bool leftMouseButton, bool middleMouseButton, bool rightMouseButton, bool ctrl, bool shift, int x, int y)
		: LeftMouseButton(leftMouseButton)
		, MiddleMouseButton(middleMouseButton)
		, RightMouseButton(rightMouseButton)
		, Ctrl(ctrl)
		, Shift(shift)
		, X(x)
		, Y(y)
		, RelX(0)
		, RelY(0)
	{}

	bool LeftMouseButton;
	bool MiddleMouseButton;
	bool RightMouseButton;
	bool Ctrl;
	bool Shift;

	int X;
	int Y;
	int RelX;
	int RelY;
};

class MouseButtonEvent : public EventArgs
{
public:
	enum MouseButton
	{
		None = 0,
		Left = 1,
		Right = 2,
		Middle = 3
	};

	enum ButtonState
	{
		Released = 0,
		Pressed = 1
	};


	typedef EventArgs base;

	MouseButtonEvent(MouseButton buttonID, ButtonState state, bool leftMouseButton, bool middleMouseButton, bool rightMouseButton, bool ctrl, bool shift, int x, int y)
		: Button(buttonID)
		, State(state)
		, LeftMouseButton(leftMouseButton)
		, MiddleMouseButton(middleMouseButton)
		, RightMouseButton(rightMouseButton)
		, Ctrl(ctrl)
		, Shift(shift)
		, X(x)
		, Y(y)
	{}

	MouseButton Button;
	ButtonState State;
	bool LeftMouseButton;
	bool MiddleMouseButton;
	bool RightMouseButton;
	bool Ctrl;
	bool Shift;

	int X;
	int Y;
};

class MouseWheelEvent : EventArgs
{
public:
	typedef EventArgs base;

	MouseWheelEvent(float wheelDelta, bool leftMouseButton, bool middleMouseButton, bool rightMouseButton, bool ctrl, bool shift, int x, int y)
		: WheelDelta(wheelDelta)
		, LeftMouseButton(leftMouseButton)
		, MiddleMouseButton(middleMouseButton)
		, RightMouseButton(rightMouseButton)
		, Ctrl(ctrl)
		, Shift(shift)
		, X(x)
		, Y(y)
	{}

	float WheelDelta;
	bool LeftMouseButton;
	bool MiddleMouseButton;
	bool RightMouseButton;
	bool Ctrl;
	bool Shift;

	int X;
	int Y;
};

class UpdateEvent : public EventArgs
{
public:
	typedef EventArgs base;
	UpdateEvent(double deltaTime, double totalTime)
		: ElapsedTime(deltaTime)
		, TotalTime(totalTime)
	{}

	double ElapsedTime;
	double TotalTime;
};

class RenderEvent : public EventArgs
{
public:
	typedef EventArgs base;
	RenderEvent(double deltaTime, double totalTime)
		: ElapsedTime(deltaTime)
		, TotalTime(totalTime)
	{}

	double ElapsedTime;
	double TotalTime;
};