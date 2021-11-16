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

#include "pch.h"
#include "NiObject.h"
#include "File.h"

std::pair<std::shared_ptr<nif::NiObject>, std::shared_ptr<Niflib::NiObject>> 
make_NiObject(nif::File&, const Niflib::Ref<Niflib::NiObject>& native)
{
	struct Creation
	{
		nif::NiObject object;
		Niflib::Ref<Niflib::NiObject> native;
	};

	auto ptr = std::make_shared<Creation>();

	return std::make_pair(
		std::shared_ptr<nif::NiObject>(ptr, &ptr->object),
		std::shared_ptr<Niflib::NiObject>(ptr, ptr->native));
}

static nif::File::FactoryFcn factory = nif::File::pushType(std::hash<const Niflib::Type*>{}(&Niflib::NiObject::TYPE), &make_NiObject);

#ifdef _DEBUG
int g_currentNiObjects = 0;
#endif

nif::NiObject::NiObject()
{
#ifdef _DEBUG
	g_currentNiObjects++;
#endif
}

nif::NiObject::~NiObject()
{
#ifdef _DEBUG
	g_currentNiObjects--;
#endif
}
