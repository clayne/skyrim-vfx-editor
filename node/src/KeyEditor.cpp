#include "pch.h"
#include "KeyEditor.h"
#include "CompositionActions.h"
#include "widget_types.h"

constexpr float DRAG_THRESHOLD = 5.0f;

constexpr float SCALE_BASE = 1.1f;
constexpr float SCALE_SENSITIVITY = 0.1f;

using namespace nif;

using PosProperties = std::array<ni_ptr<Property<float>>, 2>;

class MoveOp final : public gui::ICommand
{
	std::vector<PosProperties> m_targets;
	std::vector<Eigen::Vector2f> m_to;
	std::vector<Eigen::Vector2f> m_from;

public:
	MoveOp(std::vector<PosProperties>&& targets,
		std::vector<Eigen::Vector2f>&& to, std::vector<Eigen::Vector2f>&& from) :
		m_targets{ targets }, m_to{ to }, m_from{ from }
	{}

	virtual void execute() override 
	{
		assert(m_targets.size() == m_to.size() && m_from.size() == m_to.size());
		for (size_t i = 0; i < m_targets.size(); i++) {
			m_targets[i][0]->set(m_to[i][0]);
			m_targets[i][1]->set(m_to[i][1]);
		}
	}
	virtual void reverse() override 
	{
		for (size_t i = 0; i < m_targets.size(); i++) {
			m_targets[i][0]->set(m_from[i][0]);
			m_targets[i][1]->set(m_from[i][1]);
		}
	}
	virtual bool reversible() const override 
	{ 
		return m_to != m_from;
	}
};

//Locate a component whose center is in the vicinity of the cursor
class ClickSelector final : public gui::DescendingVisitor
{
public:
	//expects a point in global space
	ClickSelector(const gui::Floats<2>& globalPoint)
	{
		m_rect << globalPoint - 5.0f, globalPoint + 5.0f;
		m_rect = m_rect.floor();
	}

	static_assert(TO_PIXEL == static_cast<decltype(TO_PIXEL)>(&std::floor));

	virtual void visit(gui::Composite& c) override
	{
		if (m_current.empty()) {
			m_current.push({});
			m_current.top() << c.fromGlobalSpace(m_rect.head(2)), c.fromGlobalSpace(m_rect.tail(2));
		}
		else {
			const gui::Floats<4>& prev = m_current.top();
			m_current.push({});
			m_current.top() << c.fromParentSpace(prev.head(2)), c.fromParentSpace(prev.tail(2));
		}

		gui::Floats<2> size = c.getSize();
		gui::Floats<2> pos = 0.5f * gui::Floats<2>(m_current.top()[2] + m_current.top()[0], m_current.top()[3] + m_current.top()[1]).abs();
		if ((size[0] == 0.0f || size[1] == 0.0f) || //Treat 0 size as infinite. Makes sense?
			(pos[0] >= 0.0f && pos[0] <= size[0] && pos[1] >= 0.0f && pos[1] <= size[1])) {
			for (auto&& child : c.getChildren()) {
				child->accept(*this);
				if (result != nullptr)
					break;
			}
		}
		m_current.pop();
	}
	virtual void visit(gui::Component& c) override
	{
		gui::Floats<2> pos = c.getTranslation();
		const gui::Floats<4>& rect = m_current.empty() ? m_rect : m_current.top();

		float minx;
		float maxx;
		if (rect[0] < rect[2]) {
			minx = rect[0];
			maxx = rect[2];
		}
		else {
			minx = rect[2];
			maxx = rect[0];
		}
		float miny;
		float maxy;
		if (rect[1] < rect[3]) {
			miny = rect[1];
			maxy = rect[3];
		}
		else {
			miny = rect[3];
			maxy = rect[1];
		}
		if (maxx >= pos[0] && minx <= pos[0] && maxy >= pos[1] && miny <= pos[1])
			result = &c;
	}

	gui::IComponent* result{ nullptr };
private:
	std::stack<gui::Floats<4>> m_current;
	gui::Floats<4> m_rect;
};

//Locate all the components whose centres are inside the rectangle
class BoxSelector final : public gui::DescendingVisitor
{
public:
	BoxSelector(const gui::Floats<4>& rect) {}

	virtual void visit(gui::Composite& c) override {}
	virtual void visit(gui::Component& c) override {}

	std::vector<gui::IComponent*> found;
};

node::FloatKeyEditor::FloatKeyEditor(
	ni_ptr<Property<KeyType>>&& keyType,
	ni_ptr<List<Key<float>>>&& keys,
	ni_ptr<Property<float>>&& tStart,
	ni_ptr<Property<float>>&& tStop) :
	m_keyType{ keyType }, m_keys{ keys }
{
	assert(m_keyType && m_keys && tStart && tStop);

	setSize({ 640.0f, 480.0f });

	//We need:
	//plot area
	//input handler for plot area (specific mechanic on generic component inserted above plot area?)
	//handles for each key
	//a curve that interpolates the data
	//widget to select interpolation type
	//controls for start/stop time

	auto item = newChild<gui::Item>();
	item->setSize({ 200.0f, -1.0f });
	item->newChild<gui::Text>("Interpolation");
	using selector_type = gui::Selector<KeyType, ni_ptr<Property<KeyType>>>;
	auto selector = item->newChild<selector_type>(m_keyType, std::string(),
		selector_type::ItemList{
			{ KEY_CONSTANT, "Constant" },
			{ KEY_LINEAR, "Linear" },
			{ KEY_QUADRATIC, "Quadratic" } });

	m_plot = newChild<gui::Plot>();

	//The initial curve is created by the callback from keyType.
	m_keyType->addListener(*this);
	onSet(m_keyType->get());

	//determine limits
	gui::Floats<2> xlims = { tStart->get(), tStop->get() };
	if (float diff = xlims[1] - xlims[0]; diff > 0.0f) {
		xlims[0] -= 0.25f * diff;
		xlims[1] += 0.25f * diff;
	}
	else if (diff == 0.0f) {
		//start time == stop time
		//this may be valid (but stupid)
		xlims[0] -= 0.5f;
		xlims[1] += 0.5f;
	}
	else {
		//stop time < start time (error)
		//it's not really our business to fix this, we just need to deal with it
		float tmp = xlims[0];
		xlims[0] = xlims[1] - 0.25f * diff;
		xlims[1] = tmp + 0.25f * diff;
	}
	m_plot->getPlotArea().setXLimits(xlims);

	if (m_curve) {
		gui::Floats<2> ylims = m_curve->getBounds();
		if (float diff = ylims[1] - ylims[0]; diff > 0.0f) {
			ylims[0] -= 0.25f * diff;
			ylims[1] += 0.25f * diff;
		}
		else {
			assert(diff == 0.0f);
			ylims[0] -= 0.5f;
			ylims[1] += 0.5f;
		}
		m_plot->getPlotArea().setYLimits(ylims);
	}

	updateAxisUnits();
	m_plot->getPlotArea().setMouseHandler(this);
}

node::FloatKeyEditor::~FloatKeyEditor()
{
	//Exiting the program while we are open will make this invalid.
	//The property is destroyed before we are. 
	//We need to refactor NodeBase to solve this.
	m_keyType->removeListener(*this);
}

void node::FloatKeyEditor::onClose()
{
	asyncInvoke<gui::RemoveChild>(this, getParent(), false);
}

void node::FloatKeyEditor::onSet(const nif::KeyType& type)
{
	assert(m_plot);
	//Remove existing curve, clear selection
	if (m_curve) {
		//Any reason to unselect first?
		m_selection.clear();

		m_plot->getPlotArea().getAxes().removeChild(m_curve);
		m_curve = nullptr;
	}

	//Then we need to make a new of the appropriate type and add that one instead.
	//The new curve will need to know about the keys property.
	std::unique_ptr<Interpolant> curve;
	switch (type) {
	case KEY_CONSTANT:
		curve = std::make_unique<ConstantInterpolant>();
		break;
	case KEY_LINEAR:
		curve = std::make_unique<LinearInterpolant>(m_keys);
		break;
	case KEY_QUADRATIC:
		curve = std::make_unique<QuadraticInterpolant>(m_keys);
		break;
	}
	if (curve) {
		auto tmp = curve.get();//addChild may throw. Don't assign until we know it will survive.
		m_plot->getPlotArea().getAxes().addChild(std::move(curve));
		m_curve = tmp;
	}
}

bool node::FloatKeyEditor::onMouseDown(gui::Mouse::Button button)
{
	assert(m_plot);

	if (m_currentOp != Op::NONE)
		return true;
	else if (button == gui::Mouse::Button::LEFT) {
		//locate the clicked object
		ClickSelector v(gui::Mouse::getPosition());
		m_plot->getPlotArea().accept(v);

		//I'm not sure how much type info we want to have (or require).
		//Let's do a downcast for now
		KeyHandle* object = dynamic_cast<KeyHandle*>(v.result);

		if (object) {
			if (gui::Keyboard::isDown(gui::Keyboard::Key::SHIFT)) {
				if (auto res = m_selection.insert(object); !res.second) {
					//shift+clicked selected object
					//remove from selection
					if (m_activeItem == res.first)
						m_activeItem = m_selection.end();
					m_selection.erase(res.first);
					object->setSelected(false);
				}
				else {
					//shift+clicked non-selected object
					//add to selection
					object->setSelected(true);
					m_activeItem = res.first;
				}
			}
			else {
				if (auto it = m_selection.find(object); it != m_selection.end()) {
					//clicked selected object
					//start dragging selection (if threshold is passed)

					//if we release before reaching the drag threshold, should clear
					//other selected objects
					m_activeItem = it;
				}
				else {
					//clicked non-selected object
					//set selection and start dragging (if threshold is passed)
					for (auto&& o : m_selection)
						o->setSelected(false);
					m_selection.clear();
					m_activeItem = m_selection.insert(object).first;
					object->setSelected(true);
				}
				//start dragging
				gui::Mouse::setCapture(&m_plot->getPlotArea());
				m_clickPoint = gui::Mouse::getPosition();
				m_dragThresholdPassed = false;
				m_currentOp = Op::DRAG;
			}
		}
		//else clear selection? Or not?
		return true;
	}
	else if (button == gui::Mouse::Button::MIDDLE) {
		assert(gui::Mouse::getCapture() == nullptr);
		m_clickPoint = m_plot->getPlotArea().fromGlobalSpace(gui::Mouse::getPosition());
		gui::Mouse::setCapture(&m_plot->getPlotArea());
		if (gui::Keyboard::isDown(gui::Keyboard::Key::CTRL)) {
			m_currentOp = Op::ZOOM;
			m_startS = m_plot->getPlotArea().getAxes().getScale();
			m_startT = m_plot->getPlotArea().getAxes().getTranslation();
		}
		else {
			m_currentOp = Op::PAN;
			m_startT = m_plot->getPlotArea().getAxes().getTranslation();
		}

		return true;
	}
	else
		return false;
}

bool node::FloatKeyEditor::onMouseUp(gui::Mouse::Button button)
{
	bool result = false;

	if (button == gui::Mouse::Button::LEFT) {
		if (result = m_currentOp == Op::DRAG) {
			if (!m_dragThresholdPassed) {
				//they released before reaching the drag threshold, should clear
				//other selected objects
				Selection newSelection;
				m_activeItem = newSelection.insert(m_selection.extract(m_activeItem)).position;
				for (auto&& obj : m_selection) {
					assert(obj);
					obj->setSelected(false);
				}
				m_selection = std::move(newSelection);
			}
			else {
				//send final move op

				//Collect the properties from each selected key
				std::vector<PosProperties> props(m_initialState.size());
				//collect current values
				std::vector<Eigen::Vector2f> to(m_initialState.size());
				//collect initial state
				std::vector<Eigen::Vector2f> from(m_initialState.size());

				int i = 0;
				for (auto&& item : m_initialState) {
					props[i] = { item.first->getXProperty(), item.first->getYProperty() };
					to[i] = item.first->getTranslation();
					from[i] = item.second;
					i++;
				}

				asyncInvoke<MoveOp>(std::move(props), std::move(to), std::move(from));
			}

			m_currentOp = Op::NONE;
			assert(m_plot && gui::Mouse::getCapture() == &m_plot->getPlotArea());
			gui::Mouse::setCapture(nullptr);
			m_initialState.clear();
		}
	}
	else if (button == gui::Mouse::Button::MIDDLE) {
		if (result = (m_currentOp == Op::PAN || m_currentOp == Op::ZOOM)) {
			assert(m_plot && gui::Mouse::getCapture() == &m_plot->getPlotArea());
			gui::Mouse::setCapture(nullptr);
			m_currentOp = Op::NONE;
		}
	}
	
	return result;
}

bool node::FloatKeyEditor::onMouseWheel(float delta)
{
	assert(m_plot);

	float scaleFactor = std::pow(SCALE_BASE, delta);

	gui::Floats<2> pivot = m_plot->getPlotArea().fromGlobalSpace(gui::Mouse::getPosition());
	gui::Floats<2> T = m_plot->getPlotArea().getAxes().getTranslation();
	m_plot->getPlotArea().getAxes().setTranslation(pivot - (pivot - T) * scaleFactor);
	m_plot->getPlotArea().getAxes().scale({ scaleFactor, scaleFactor });

	updateAxisUnits();

	return true;
}

void node::FloatKeyEditor::onMouseMove(const gui::Floats<2>& pos)
{
	switch (m_currentOp) {
	case Op::DRAG:
		drag(pos);
		break;
	case Op::PAN:
		pan(pos);
		break;
	case Op::ZOOM:
		zoom(pos);
		break;
	}
}

void node::FloatKeyEditor::drag(const gui::Floats<2>& pos)
{
	assert(!m_selection.empty());//should be checked before starting a move op

	if (!m_dragThresholdPassed &&
		(std::abs(pos[0] - m_clickPoint[0]) >= DRAG_THRESHOLD ||
		std::abs(pos[1] - m_clickPoint[1]) >= DRAG_THRESHOLD))
	{
		m_dragThresholdPassed = true;

		//save initial state
		assert(m_initialState.empty());
		m_initialState.reserve(m_selection.size());
		for (KeyHandle* obj : m_selection) {
			assert(obj);
			m_initialState.push_back({ obj, obj->getTranslation() });
		}
	}

	if (m_dragThresholdPassed) {
		gui::Floats<2> delta = pos - m_clickPoint;

		for (auto&& item : m_initialState) {
			if (IComponent* parent = item.first->getParent()) {
				//Do we forego our previous design of sending everything to the Invoker?
				//If we do any synchronous access to the data, the Property themselves should guard.
				//Then again, we don't know if a listener wants to make changes to the graph in response.
				//Let's forego it, but make sure we can easily find everywhere we have done so:
				static_assert(FOREGO_ASYNC);

				gui::Floats<2> global = parent->toGlobalSpace(item.second);
				item.first->setTranslation(parent->fromGlobalSpace(global + delta));
			}
		}
	}
}

void node::FloatKeyEditor::pan(const gui::Floats<2>& pos)
{
	assert(m_plot);
	gui::Floats<2> delta = m_plot->getPlotArea().fromGlobalSpace(pos) - m_clickPoint;
	m_plot->getPlotArea().getAxes().setTranslation(m_startT + delta);
}

void node::FloatKeyEditor::zoom(const gui::Floats<2>& pos)
{
	assert(m_plot);
	gui::Floats<2> delta = m_plot->getPlotArea().fromGlobalSpace(pos) - m_clickPoint;
	gui::Floats<2> factor;
	factor[0] = std::pow(SCALE_BASE, delta[0] * SCALE_SENSITIVITY);
	factor[1] = std::pow(SCALE_BASE, -delta[1] * SCALE_SENSITIVITY);

	m_plot->getPlotArea().getAxes().setTranslation(m_clickPoint - (m_clickPoint - m_startT) * factor);
	m_plot->getPlotArea().getAxes().setScale(m_startS * factor);

	updateAxisUnits();
}

void node::FloatKeyEditor::updateAxisUnits()
{
	assert(m_plot);

	//major unit should be 1, 2 or 5 to some power of 10
	gui::Floats<2> major;
	gui::Floats<2> xlims = m_plot->getPlotArea().getXLimits();
	gui::Floats<2> ylims = m_plot->getPlotArea().getYLimits();
	gui::Floats<2> estimate{ 0.5f * std::abs(xlims[1] - xlims[0]), 0.5f * std::abs(ylims[1] - ylims[0]) };
	//gui::Floats<2> estimate = 300.0f / m_area.getAxes().getScale().abs();//problematic before the first frame
	gui::Floats<2> power = estimate.log10().floor();
	gui::Floats<2> oom = { std::pow(10.0f, power[0]), std::pow(10.0f, power[1]) };
	if (estimate[0] >= 5.0f * oom[0])
		major[0] = 5.0f * oom[0];
	else if (estimate[0] >= 2.0f * oom[0])
		major[0] = 2.0f * oom[0];
	else
		major[0] = oom[0];

	if (estimate[1] >= 5.0f * oom[1])
		major[1] = 5.0f * oom[1];
	else if (estimate[1] >= 2.0f * oom[1])
		major[1] = 2.0f * oom[1];
	else
		major[1] = oom[1];

	//minor unit should be major / 4 (2?)
	gui::Floats<2> minor = major / 4.0f;

	m_plot->getPlotArea().getAxes().setMajorUnits(major);
	m_plot->getPlotArea().getAxes().setMinorUnits(minor);
}


node::FloatKeyEditor::KeyHandle::KeyHandle(ni_ptr<Key<float>>&& key, ni_ptr<Key<float>>&& next) :
	m_timeLsnr{ &m_translation[0] },
	m_valueLsnr{ &m_translation[1] },
	m_key{ std::move(key) }, 
	m_next{ std::move(next) }
{
	assert(m_key);
	m_key->time.addListener(m_timeLsnr);
	m_key->value.addListener(m_valueLsnr);
	m_translation = { m_key->time.get(), m_key->value.get() };
}

node::FloatKeyEditor::KeyHandle::~KeyHandle()
{
	m_key->time.removeListener(m_timeLsnr);
	m_key->value.removeListener(m_valueLsnr);
}

void node::FloatKeyEditor::KeyHandle::frame(gui::FrameDrawer& fd)
{
	if (m_dirty) {
		//Recalculate our interpolation (if we were quadratic)
		m_dirty = false;
	}

	//We want to scale with PlotArea, not Axes. Not so easy right now. 
	//We'll just not scale at all.
	if (m_next)
		fd.line(
			fd.toGlobal(m_translation), 
			fd.toGlobal({ m_next->time.get(), m_next->value.get() }), 
			{ 1.0f, 0.0f, 0.0f, 1.0f }, 
			3.0f,
			true);
	fd.circle(fd.toGlobal(m_translation), 3.0f, m_selected ? nif::COL_WHITE : nif::COL_BLACK, true);
}

void node::FloatKeyEditor::KeyHandle::setTranslation(const gui::Floats<2>& t)
{
	assert(m_key);
	m_key->time.set(t[0]);
	m_key->value.set(t[1]);
}

ni_ptr<Property<float>> node::FloatKeyEditor::KeyHandle::getXProperty() const
{
	return make_ni_ptr(m_key, &Key<float>::time);
}

ni_ptr<Property<float>> node::FloatKeyEditor::KeyHandle::getYProperty() const
{
	return make_ni_ptr(m_key, &Key<float>::value);
}


node::FloatKeyEditor::LinearInterpolant::LinearInterpolant(const ni_ptr<List<Key<float>>>& keys) :
	m_keys{ keys }
{
	assert(m_keys);
	m_keys->addListener(*this);

	if (m_keys->size() > 0) {

		auto next = ++m_keys->begin();
		for (auto&& key : *m_keys) {

			ni_ptr<Key<float>> next_key;
			if (next != m_keys->end()) {
				next_key = make_ni_ptr(m_keys, &(*next));
				++next;
			}
			newChild<KeyHandle>(make_ni_ptr(m_keys, &key), std::move(next_key));
		}
	}
}

node::FloatKeyEditor::LinearInterpolant::~LinearInterpolant()
{
	m_keys->removeListener(*this);
}

gui::Floats<2> node::FloatKeyEditor::LinearInterpolant::getBounds() const
{
	gui::Floats<2> result = { 0.0f, 0.0f };
	for (auto&& key : *m_keys) {
		if (key.time.get() < result[0])
			result[0] = key.value.get();
		else if (key.value.get() > result[1])
			result[1] = key.value.get();
	}
	return result;
}
