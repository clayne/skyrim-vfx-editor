//Copyright 2021 Jonas Gernandt
//
//This file is part of SVFX Editor, a program for creating visual effects
//in the NetImmerse format.
//
//SVFX Editor is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//SVFX Editor is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with SVFX Editor. If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include "gui_types.h"

namespace gui
{
	class IComponent;

	//I think this is better (as a general idea) than to go via the FrameDrawer/GUIEngine.
	//Needs some work, though.
	class Keyboard
	{
	public:
		enum class Key : int
		{
			SHIFT		= 0x10,
			CTRL		= 0x11,
			ALT			= 0x12,
		};
	public:
		static bool isDown(Key key);
	};

	class Mouse
	{
	public:
		enum class Button
		{
			NONE,
			LEFT,
			MIDDLE,
			RIGHT,
			EXTRA1,
			EXTRA2,
		};

	public:
		static bool isButtonDown(Button btn) { return false; }
		static Floats<2> getPosition();
		static void setPosition(const Floats<2>& pos) {}
		static float getWheelDelta() { return 0.0f; }

		static IComponent* getCapture() { return s_capturing; }
		static void setCapture(IComponent* c) { s_capturing = c; }

	private:
		static IComponent* s_capturing;
	};

	class MouseHandler
	{
	public:
		virtual ~MouseHandler() = default;

		//Let true return indicate that the event was handled
		virtual bool onMouseDown(Mouse::Button) { return false; }
		virtual bool onMouseUp(Mouse::Button) { return false; }
		virtual bool onMouseWheel(float) { return false; }

		virtual void onMouseMove(const Floats<2>& pos) {}
		virtual void onMouseEnter() {}
		virtual void onMouseLeave() {}

	};


	int guiToImGuiButton(Mouse::Button button);
}