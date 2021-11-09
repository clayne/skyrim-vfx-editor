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
#include "Composition.h"
#include "Visitor.h"
#include "CallWrapper.h"

#include "imgui_internal.h"

gui::Component::~Component()
{
	assert(!m_parent);
}

bool gui::Component::hasAncestor(IComponent* c) const
{
	return (m_parent && c) ? m_parent == c || m_parent->hasAncestor(c) : false;
}

void gui::Component::frame(FrameDrawer& fd)
{
	handleMouse(fd);
}

gui::Floats<2> gui::Component::getGlobalPosition() const
{
	return toGlobalSpace({ 0.0f, 0.0f });
}

gui::Floats<2> gui::Component::toGlobalSpace(const Floats<2>& p) const
{
	Floats<2> pp = toParentSpace(p);
	return m_parent ? m_parent->toGlobalSpace(pp) : pp;
}

gui::Floats<2> gui::Component::toParentSpace(const Floats<2>& p) const
{
	return m_translation + p * m_scale;
}

gui::Floats<2> gui::Component::fromGlobalSpace(const Floats<2>& p) const
{
	Floats<2> pp = m_parent ? m_parent->fromGlobalSpace(p) : p;
	return fromParentSpace(pp);
}

gui::Floats<2> gui::Component::fromParentSpace(const Floats<2>& p) const
{
	return (p - m_translation) / m_scale;
}

void gui::Component::accept(Visitor& v)
{
	v.visit(*this);
}

gui::IInvoker* gui::Component::getInvoker()
{
	return m_parent ? m_parent->getInvoker() : nullptr;
}

void gui::Component::handleMouse(FrameDrawer& fd)
{
	//I would much prefer to separate this from the frame event, 
	//and process each input to completion before considering the next.
	//Like this it's hard to control and overview who receives what input.
	if (m_mouseHandler) {

		ImGuiContext* g = ImGui::GetCurrentContext();
		assert(g);

		enum class Capturing
		{
			NONE,
			WE,
			CHILD,
			OTHER
		};

		Capturing who;
		if (Mouse::getCapture() == this)
			who = Capturing::WE;
		else if (Mouse::getCapture() == nullptr)
			//need to account for items that are still using imgui here (not sure if this will be enough)
			who = g->ActiveId ? Capturing::OTHER : Capturing::NONE;
		else if (Mouse::getCapture()->hasAncestor(this))
			who = Capturing::CHILD;
		else {
			who = Capturing::OTHER;
		}

		if (who == Capturing::OTHER) {
			//We should not recieve inputs, and should lose focus if we had it
			if (m_mouseFocus) {
				m_mouseFocus = false;
				m_mouseHandler->onMouseLeave();
			}
		}
		else if (who == Capturing::CHILD) {
			//We should not receive input, but should also not lose focus
		}
		else {
			//We should receive inputs

			ImGuiIO& io = ImGui::GetIO();

			bool capturing = who == Capturing::WE;

			ImGuiWindow* window = g->CurrentWindow;
			assert(window);

			static_assert(TO_PIXEL == static_cast<decltype(TO_PIXEL)>(&std::floor));
			gui::Floats<2> tlss = fd.toGlobal(m_translation).floor();
			gui::Floats<2> brss = fd.toGlobal(m_translation + m_size).floor();
			bool inside = false;
			if (g->HoveredWindow == window)
				//TODO: deal with popups
				inside = io.MousePos.x >= tlss[0] && io.MousePos.x <= brss[0] && io.MousePos.y >= tlss[1] && io.MousePos.y <= brss[0];

			if (capturing || inside)
			{
				//Moving
				if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)
					m_mouseHandler->onMouseMove(gui_type_conversion<Floats<2>>::from(io.MousePos));

				//Clicking (TODO: report handled events)
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					m_mouseHandler->onMouseDown(Mouse::Button::LEFT);
				else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
					m_mouseHandler->onMouseUp(Mouse::Button::LEFT);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
					m_mouseHandler->onMouseDown(Mouse::Button::MIDDLE);
				else if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
					m_mouseHandler->onMouseUp(Mouse::Button::MIDDLE);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					m_mouseHandler->onMouseDown(Mouse::Button::RIGHT);
				else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
					m_mouseHandler->onMouseUp(Mouse::Button::RIGHT);

				//Scrolling
				//ImGui::SetItemUsingMouseWheel();//prevents imgui from scrolling the window
				if (io.MouseWheel != 0.0f && !fd.isWheelHandled()) {
					if (m_mouseHandler->onMouseWheel(io.MouseWheel))
						fd.setWheelHandled();
				}
			}
			if (!capturing) {
				if (inside) {
					//Entering
					if (!m_mouseFocus) {
						m_mouseFocus = true;
						m_mouseHandler->onMouseEnter();
					}
				}
				else {
					//Leaving
					if (m_mouseFocus) {
						m_mouseFocus = false;
						m_mouseHandler->onMouseLeave();
					}
				}
			}
		}
	}
}

gui::IComponent* gui::Component::getRoot()
{
	return m_parent ? m_parent->getRoot() : this;
}

gui::Composite::~Composite()
{
	for (auto& c : m_children) {
		assert(c);
		c->setParent(nullptr);
	}
}

void gui::Composite::frame(FrameDrawer& fd)
{
	auto popper = fd.pushTransform(m_translation, m_scale);
	for (auto& c : m_children) {
		assert(c);
		c->frame(fd);
	}

	handleMouse(fd);
}

void gui::Composite::accept(Visitor& v)
{
	v.visit(*this);
}

void gui::Composite::addChild(ComponentPtr&& c)
{
	if (c) {
		//ptr must not have a parent (we do not check for duplicate children, though)
		assert(!c->getParent());
		c->setParent(this);
		m_children.push_back(std::move(c));
	}
}

gui::ComponentPtr gui::Composite::removeChild(IComponent* c)
{
	ComponentPtr ret;

	auto pred = [&](const ComponentPtr& p) { return p.get() == c; };
	if (auto it = std::find_if(m_children.begin(), m_children.end(), pred); it != m_children.end()) {
		assert(c);
		c->setParent(nullptr);
		ret = std::move(*it);
		m_children.erase(it);
	}

	return ret;
}

void gui::Composite::clearChildren()
{
	for (auto&& child : m_children) {
		assert(child);
		child->setParent(nullptr);
	}
	m_children.clear();
}

gui::ComponentDecorator::ComponentDecorator(ComponentPtr&& c) : m_component{ std::move(c) }
{
	assert(m_component && !m_component->getParent());
	m_component->setParent(this);
}

gui::ComponentDecorator::~ComponentDecorator()
{
	assert(m_component && !m_parent);
	m_component->setParent(nullptr);
}

bool gui::ComponentDecorator::hasAncestor(IComponent* c) const
{
	return (m_parent && c) ? m_parent == c || m_parent->hasAncestor(c) : false;
}

void gui::ComponentDecorator::addChild(ComponentPtr&& c)
{
	assert(m_component);
	m_component->addChild(std::move(c));
}

gui::ComponentPtr gui::ComponentDecorator::removeChild(IComponent* c)
{
	assert(m_component);
	if (c == m_component.get() && m_parent)
		//this call should be coming from the component itself
		return m_parent->removeChild(this);
	else
		return m_component->removeChild(c);
}

void gui::ComponentDecorator::clearChildren()
{
	assert(m_component);
	m_component->clearChildren();
}

void gui::ComponentDecorator::frame(FrameDrawer& fd)
{
	assert(m_component);
	m_component->frame(fd);
}

gui::Floats<2> gui::ComponentDecorator::getTranslation() const
{
	assert(m_component);
	return m_component->getTranslation();
}

void gui::ComponentDecorator::setTranslation(const Floats<2>& t)
{
	assert(m_component);
	m_component->setTranslation(t);
}

void gui::ComponentDecorator::setTranslationX(float x)
{
	assert(m_component);
	m_component->setTranslationX(x);
}

void gui::ComponentDecorator::setTranslationY(float y)
{
	assert(m_component);
	m_component->setTranslationY(y);
}

void gui::ComponentDecorator::translate(const Floats<2>& t)
{
	assert(m_component);
	m_component->translate(t);
}

gui::Floats<2> gui::ComponentDecorator::getScale() const
{
	assert(m_component);
	return m_component->getScale();
}

void gui::ComponentDecorator::setScale(const Floats<2>& s)
{
	assert(m_component);
	m_component->setScale(s);
}

void gui::ComponentDecorator::setScaleX(float x)
{
	assert(m_component);
	m_component->setScaleX(x);
}

void gui::ComponentDecorator::setScaleY(float y)
{
	assert(m_component);
	m_component->setScaleY(y);
}

void gui::ComponentDecorator::scale(const Floats<2>& s)
{
	assert(m_component);
	m_component->scale(s);
}

gui::Floats<2> gui::ComponentDecorator::getGlobalPosition() const
{
	assert(false);//TODO
	return Floats<2>();
}

gui::Floats<2> gui::ComponentDecorator::toGlobalSpace(const Floats<2>& p) const
{
	assert(false);//TODO
	return Floats<2>();
}

gui::Floats<2> gui::ComponentDecorator::toParentSpace(const Floats<2>& p) const
{
	assert(false);//TODO
	return Floats<2>();
}

gui::Floats<2> gui::ComponentDecorator::fromGlobalSpace(const Floats<2>& p) const
{
	assert(false);//TODO
	return Floats<2>();
}

gui::Floats<2> gui::ComponentDecorator::fromParentSpace(const Floats<2>& p) const
{
	assert(false);//TODO
	return Floats<2>();
}

gui::Floats<2> gui::ComponentDecorator::getSize() const
{
	assert(m_component);
	return m_component->getSize();
}

void gui::ComponentDecorator::setSize(const Floats<2>& size)
{
	assert(m_component);
	m_component->setSize(size);
}

gui::Floats<2> gui::ComponentDecorator::getSizeHint() const
{
	assert(m_component);
	return m_component->getSizeHint();
}

void gui::ComponentDecorator::setSizeHint(const Floats<2>& hint)
{
	assert(m_component);
	m_component->setSizeHint(hint);
}

void gui::ComponentDecorator::accept(Visitor& v)
{
	assert(m_component);
	m_component->accept(v);
}

gui::IInvoker* gui::ComponentDecorator::getInvoker()
{
	return m_parent ? m_parent->getInvoker() : nullptr;
}

gui::IComponent* gui::ComponentDecorator::getRoot()
{
	return m_parent ? m_parent->getRoot() : this;
}

void gui::ComponentDecorator::setMouseHandler(MouseHandler* h)
{
	assert(m_component);
	m_component->setMouseHandler(h);
}
