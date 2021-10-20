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
#include <memory>
#include "IInvoker.h"
#include "gui_types.h"

namespace gui
{
	class Visitor;

	//We need some object to track certain state as we traverse the composition. Things like clip regions, current transforms.
	//Come to think of it, this object might actually be the same thing as what we have so far called a GUIEngine. It's purpose
	//is to translate our composition into vertex buffers.
	//In any way, sending (an interface to) such an object to frame seems like the way to go.
	class FrameDrawer 
	{

	};

	class IComponent
	{
	public:
		virtual ~IComponent() {}

		virtual IComponent* getParent() const = 0;
		//Not for public use! Parenting should be done via add/removeChild.
		virtual void setParent(IComponent*) = 0;

		virtual void addChild(std::unique_ptr<IComponent>&&) = 0;
		virtual std::unique_ptr<IComponent> removeChild(IComponent*) = 0;
		virtual void clearChildren() = 0;

		virtual void frame(FrameDrawer&) = 0;

		//Actual position of component. Decided at runtime.
		virtual Floats<2> getPosition() const = 0;
		virtual void setPosition(const Floats<2>&) = 0;
		virtual Floats<2> getGlobalPosition() const = 0;
		//if we allow scaling of components, we need to pass the local pos to get a global
		//virtual Floats<2> getGlobalPosition(const Floats<2>& local) const = 0;

		//Actual size of component. Decided at runtime.
		virtual Floats<2> getSize() const = 0;
		virtual void setSize(const Floats<2>&) = 0;//maybe this should not be public, if it is a calculated value

		//Hint at the desired size of this component. Used by layout operators.
		virtual Floats<2> getSizeHint() const = 0;
		virtual void setSizeHint(const Floats<2>&) = 0;
		//(This size system isn't fully implemented yet. I'm not convinced by it.)

		virtual void accept(Visitor&) = 0;

		virtual IInvoker* getInvoker() = 0;
		virtual IComponent* getRoot() = 0;
	};

	typedef std::unique_ptr<IComponent> ComponentPtr;
}