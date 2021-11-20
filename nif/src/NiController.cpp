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
#include "NiController.h"
#include "File.h"

const size_t nif::NiBoolData::TYPE = std::hash<std::string>{}("NiBoolData");
const size_t nif::NiFloatData::TYPE = std::hash<std::string>{}("NiFloatData");
const size_t nif::NiInterpolator::TYPE = std::hash<std::string>{}("NiInterpolator");
const size_t nif::NiBoolInterpolator::TYPE = std::hash<std::string>{}("NiBoolInterpolator");
const size_t nif::NiFloatInterpolator::TYPE = std::hash<std::string>{}("NiFloatInterpolator");
const size_t nif::NiBlendInterpolator::TYPE = std::hash<std::string>{}("NiBlendInterpolator");
const size_t nif::NiBlendBoolInterpolator::TYPE = std::hash<std::string>{}("NiBlendBoolInterpolator");
const size_t nif::NiBlendFloatInterpolator::TYPE = std::hash<std::string>{}("NiBlendFloatInterpolator");
const size_t nif::NiTimeController::TYPE = std::hash<std::string>{}("NiTimeController");
const size_t nif::NiSingleInterpController::TYPE = std::hash<std::string>{}("NiSingleInterpController");


void nif::ReadSyncer<nif::NiBoolData>::operator()(NiBoolData& object, const Niflib::NiBoolData* native, File& file)
{
	assert(native);

	object.keyType.set(nif_type_conversion<KeyType>::from(native->GetKeyType()));

	object.keys.clear();
	for (auto&& key : native->GetKeys()) {
		object.keys.push_back();
		object.keys.back().time.set(key.time);
		object.keys.back().value.set(static_cast<bool>(key.data));
		object.keys.back().fwdTan.set(static_cast<bool>(key.forward_tangent));
		object.keys.back().bwdTan.set(static_cast<bool>(key.backward_tangent));
		object.keys.back().tension.set(key.tension);
		object.keys.back().bias.set(key.bias);
		object.keys.back().continuity.set(key.continuity);
	}
}

void nif::WriteSyncer<nif::NiBoolData>::operator()(const NiBoolData& object, Niflib::NiBoolData* native, const File& file)
{
	assert(native);

	native->SetKeyType(nif_type_conversion<Niflib::KeyType>::from(object.keyType.get()));

	auto&& keys = native->GetKeysRef();
	keys.clear();
	for (auto&& key : object.keys) {
		keys.push_back({ key.time.get(), key.value.get(), key.fwdTan.get(),
			key.bwdTan.get(), key.tension.get(), key.bias.get(), key.continuity.get() });
	}
}


void nif::ReadSyncer<nif::NiFloatData>::operator()(NiFloatData& object, const Niflib::NiFloatData* native, File& file)
{
	assert(native);

	object.keyType.set(nif_type_conversion<KeyType>::from(native->GetKeyType()));

	object.keys.clear();
	for (auto&& key : native->GetKeys()) {
		object.keys.push_back();
		object.keys.back().time.set(key.time);
		object.keys.back().value.set(key.data);
		object.keys.back().fwdTan.set(key.forward_tangent);
		object.keys.back().bwdTan.set(key.backward_tangent);
		object.keys.back().tension.set(key.tension);
		object.keys.back().bias.set(key.bias);
		object.keys.back().continuity.set(key.continuity);
	}
}

void nif::WriteSyncer<nif::NiFloatData>::operator()(const NiFloatData& object, Niflib::NiFloatData* native, const File& file)
{
	assert(native);

	native->SetKeyType(nif_type_conversion<Niflib::KeyType>::from(object.keyType.get()));

	auto&& keys = native->GetKeysRef();
	keys.clear();
	for (auto&& key : object.keys) {
		keys.push_back({ key.time.get(), key.value.get(), key.fwdTan.get(),
			key.bwdTan.get(), key.tension.get(), key.bias.get(), key.continuity.get() });
	}
}


void nif::ReadSyncer<nif::NiBoolInterpolator>::operator()(NiBoolInterpolator& object, const Niflib::NiBoolInterpolator* native, File& file)
{
	assert(native);
	object.value.set(native->GetBoolValue());
	object.data.assign(file.get<NiBoolData>(native->GetData()));
}

void nif::WriteSyncer<nif::NiBoolInterpolator>::operator()(const NiBoolInterpolator& object, Niflib::NiBoolInterpolator* native, const File& file)
{
	assert(native);
	native->SetBoolValue(object.value.get());
	native->SetData(file.get<Niflib::NiBoolData>(object.data.assigned()));
}


void nif::ReadSyncer<nif::NiFloatInterpolator>::operator()(NiFloatInterpolator& object, const Niflib::NiFloatInterpolator* native, File& file)
{
	assert(native);
	object.value.set(native->GetFloatValue());
	object.data.assign(file.get<NiFloatData>(native->GetData()));
}

void nif::WriteSyncer<nif::NiFloatInterpolator>::operator()(const NiFloatInterpolator& object, Niflib::NiFloatInterpolator* native, const File& file)
{
	assert(native);
	native->SetFloatValue(object.value.get());
	native->SetData(file.get<Niflib::NiFloatData>(object.data.assigned()));
}

/*nif::NiInterpolator::NiInterpolator(native_type* obj) : NiObject(obj) {}

nif::native::NiInterpolator& nif::NiInterpolator::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiInterpolator::TYPE));
	return static_cast<native::NiInterpolator&>(*m_ptr);
}


nif::NiBoolData::NiBoolData() : NiBoolData(new Niflib::NiBoolData) {}
nif::NiBoolData::NiBoolData(native_type* obj) :
	NiObject(obj), 
	m_keyType(*this, obj, &native::NiBoolData::GetKeyType, &native::NiBoolData::SetKeyType)
{}

nif::native::NiBoolData& nif::NiBoolData::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiBoolData::TYPE));
	return static_cast<native::NiBoolData&>(*m_ptr);
}


nif::NiBoolInterpolator::NiBoolInterpolator() : NiBoolInterpolator(new Niflib::NiBoolInterpolator) {}

nif::NiBoolInterpolator::NiBoolInterpolator(native_type* obj) :
	NiInterpolator(obj),
	m_value(*this, &getNative(), &native::NiBoolInterpolator::GetBoolValue, &native::NiBoolInterpolator::SetBoolValue),
	m_data(*this, &getNative(), &native::NiBoolInterpolator::GetData, &native::NiBoolInterpolator::SetData)
{}

nif::native::NiBoolInterpolator& nif::NiBoolInterpolator::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiBoolInterpolator::TYPE));
	return static_cast<native::NiBoolInterpolator&>(*m_ptr);
}


nif::NiFloatData::NiFloatData() : NiFloatData(new Niflib::NiFloatData) {}
nif::NiFloatData::NiFloatData(native_type* obj) :
	NiObject(obj),
	m_keyType(*this, obj, &native::NiFloatData::GetKeyType, &native::NiFloatData::SetKeyType),
	m_keys(*this)
{}

nif::native::NiFloatData& nif::NiFloatData::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiFloatData::TYPE));
	return static_cast<native::NiFloatData&>(*m_ptr);
}

nif::NiFloatData::IplnData::IplnData(NiFloatData& super) :
	m_keys(super), m_tans(super), m_tbcs(super), m_super{ super }
{}

nif::VectorProperty<nif::Key<float>>& nif::NiFloatData::IplnData::keys()
{
	return m_keys;
}

nif::VectorProperty<nif::Tangent<float>>& nif::NiFloatData::IplnData::tangents()
{
	return m_tans;
}

nif::VectorProperty<nif::TBC>& nif::NiFloatData::IplnData::tbc()
{
	return m_tbcs;
}

std::vector<nif::Key<float>> nif::NiFloatData::IplnData::Keys::get() const
{
	auto&& keys = nativePtr()->GetKeysRef();
	std::vector<Key<float>> result;
	result.reserve(keys.size());
	for (auto&& key : keys)
		result.push_back({ key.time, key.data });

	return result;
}

void nif::NiFloatData::IplnData::Keys::set(const std::vector<Key<float>>& keys)
{
	auto&& dest = nativePtr()->GetKeysRef();

	//first erase excess
	for (size_t i = dest.size(); i > keys.size(); i--)
		erase(i - 1);

	assert(dest.size() <= keys.size());//sanity check

	//then set existing
	size_t i = 0;
	for (; i < dest.size(); i++) {
		if (keys[i].key != dest[i].time || keys[i].value != dest[i].data) {
			dest[i].time = keys[i].key;
			dest[i].data = keys[i].value;
			notifySet(i, keys[i]);
		}
	}

	//then insert extra
	dest.reserve(keys.size());
	for (; i < keys.size(); i++)
		insert(i, keys[i]);
}

nif::Key<float> nif::NiFloatData::IplnData::Keys::get(int i) const
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	return { keys[i].time, keys[i].data };
}

void nif::NiFloatData::IplnData::Keys::set(int i, const Key<float>& key)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	if (key.key != keys[i].time || key.value != keys[i].data) {
		keys[i].time = key.key;
		keys[i].data = key.value;
		notifySet(i, key);
	}
}

int nif::NiFloatData::IplnData::Keys::insert(int i, const Key<float>& key)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0);

	std::vector<Niflib::Key<float>>::iterator it;
	if (static_cast<size_t>(i) < keys.size())
		it = keys.begin() + i;
	else {
		it = keys.end();
		i = keys.size();
	}

	keys.insert(it, Niflib::Key<float>{key.key, key.value, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
	notifyInsert(i);
	block().m_keys.m_tans.notifyInsert(i);
	block().m_keys.m_tbcs.notifyInsert(i);

	return i;
}

int nif::NiFloatData::IplnData::Keys::erase(int i)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	keys.erase(keys.begin() + i);
	notifyErase(i);
	block().m_keys.m_tans.notifyErase(i);
	block().m_keys.m_tbcs.notifyErase(i);

	return i;
}

std::vector<nif::Tangent<float>> nif::NiFloatData::IplnData::Tangents::get() const
{
	auto&& keys = nativePtr()->GetKeysRef();
	std::vector<Tangent<float>> result;
	result.reserve(keys.size());
	for (auto&& key : keys)
		result.push_back({ key.forward_tangent, key.backward_tangent });

	return result;
}

void nif::NiFloatData::IplnData::Tangents::set(const std::vector<Tangent<float>>& keys)
{
	auto&& dest = nativePtr()->GetKeysRef();

	//first erase excess
	for (size_t i = dest.size(); i > keys.size(); i--)
		erase(i - 1);

	assert(dest.size() <= keys.size());//sanity check

	//then set existing
	size_t i = 0;
	for (; i < dest.size(); i++) {
		if (keys[i].forward != dest[i].forward_tangent || keys[i].backward != dest[i].backward_tangent) {
			dest[i].forward_tangent = keys[i].forward;
			dest[i].backward_tangent = keys[i].backward;
			notifySet(i, keys[i]);
		}
	}

	//then insert extra
	dest.reserve(keys.size());
	for (; i < keys.size(); i++)
		insert(i, keys[i]);
}

nif::Tangent<float> nif::NiFloatData::IplnData::Tangents::get(int i) const
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	return { keys[i].forward_tangent, keys[i].backward_tangent };
}

void nif::NiFloatData::IplnData::Tangents::set(int i, const Tangent<float>& tan)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	if (tan.forward != keys[i].forward_tangent || tan.backward != keys[i].backward_tangent) {
		keys[i].forward_tangent = tan.forward;
		keys[i].backward_tangent = tan.backward;
		notifySet(i, tan);
	}
}

int nif::NiFloatData::IplnData::Tangents::insert(int i, const Tangent<float>& tan)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0);

	std::vector<Niflib::Key<float>>::iterator it;
	if (static_cast<size_t>(i) < keys.size())
		it = keys.begin() + i;
	else {
		it = keys.end();
		i = keys.size();
	}

	keys.insert(it, Niflib::Key<float>{0.0f, 0.0f, tan.forward, tan.backward, 0.0f, 0.0f, 0.0f });
	notifyInsert(i);
	block().m_keys.m_keys.notifyInsert(i);
	block().m_keys.m_tbcs.notifyInsert(i);

	return i;
}

int nif::NiFloatData::IplnData::Tangents::erase(int i)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	keys.erase(keys.begin() + i);
	notifyErase(i);
	block().m_keys.m_keys.notifyErase(i);
	block().m_keys.m_tbcs.notifyErase(i);

	return i;
}

std::vector<nif::TBC> nif::NiFloatData::IplnData::TBCs::get() const
{
	auto&& keys = nativePtr()->GetKeysRef();
	std::vector<TBC> result;
	result.reserve(keys.size());
	for (auto&& key : keys)
		result.push_back({ key.tension, key.bias, key.continuity });

	return result;
}

void nif::NiFloatData::IplnData::TBCs::set(const std::vector<TBC>& keys)
{
	auto&& dest = nativePtr()->GetKeysRef();

	//first erase excess
	for (size_t i = dest.size(); i > keys.size(); i--)
		erase(i - 1);

	assert(dest.size() <= keys.size());//sanity check

	//then set existing
	size_t i = 0;
	for (; i < dest.size(); i++) {
		if (keys[i].tension != dest[i].tension || 
			keys[i].bias != dest[i].bias || 
			keys[i].continuity != dest[i].continuity) 
		{
			dest[i].tension = keys[i].tension;
			dest[i].bias = keys[i].bias;
			dest[i].continuity = keys[i].continuity;
			notifySet(i, keys[i]);
		}
	}

	//then insert extra
	dest.reserve(keys.size());
	for (; i < keys.size(); i++)
		insert(i, keys[i]);
}

nif::TBC nif::NiFloatData::IplnData::TBCs::get(int i) const
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	return { keys[i].tension, keys[i].bias, keys[i].continuity };
}

void nif::NiFloatData::IplnData::TBCs::set(int i, const TBC& tbc)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	if (tbc.tension != keys[i].tension || tbc.bias != keys[i].bias || tbc.continuity != keys[i].continuity) {
		keys[i].tension = tbc.tension;
		keys[i].bias = tbc.bias;
		keys[i].continuity = tbc.continuity;
		notifySet(i, tbc);
	}
}

int nif::NiFloatData::IplnData::TBCs::insert(int i, const TBC& tbc)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0);

	std::vector<Niflib::Key<float>>::iterator it;
	if (static_cast<size_t>(i) < keys.size())
		it = keys.begin() + i;
	else {
		it = keys.end();
		i = keys.size();
	}

	keys.insert(it, Niflib::Key<float>{0.0f, 0.0f, 0.0f, 0.0f, tbc.tension, tbc.bias, tbc.continuity });
	notifyInsert(i);
	block().m_keys.m_keys.notifyInsert(i);
	block().m_keys.m_tans.notifyInsert(i);

	return i;
}

int nif::NiFloatData::IplnData::TBCs::erase(int i)
{
	auto&& keys = nativePtr()->GetKeysRef();
	assert(i >= 0 && static_cast<size_t>(i) < keys.size());

	keys.erase(keys.begin() + i);
	notifyErase(i);
	block().m_keys.m_keys.notifyErase(i);
	block().m_keys.m_tans.notifyErase(i);

	return i;
}


nif::NiFloatInterpolator::NiFloatInterpolator() : NiFloatInterpolator(new Niflib::NiFloatInterpolator) {}

nif::NiFloatInterpolator::NiFloatInterpolator(native_type* obj) :
	NiInterpolator(obj),
	m_value(*this, &getNative(), &native::NiFloatInterpolator::GetFloatValue, &native::NiFloatInterpolator::SetFloatValue),
	m_data(*this, &getNative(), &native::NiFloatInterpolator::GetData, &native::NiFloatInterpolator::SetData)
{
}

nif::native::NiFloatInterpolator& nif::NiFloatInterpolator::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiFloatInterpolator::TYPE));
	return static_cast<native::NiFloatInterpolator&>(*m_ptr);
}


nif::NiBlendBoolInterpolator::NiBlendBoolInterpolator() : NiBlendBoolInterpolator(new Niflib::NiBlendBoolInterpolator) {}
nif::NiBlendBoolInterpolator::NiBlendBoolInterpolator(native_type* obj) : NiInterpolator(obj) {}

nif::native::NiBlendBoolInterpolator& nif::NiBlendBoolInterpolator::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiBlendBoolInterpolator::TYPE));
	return static_cast<native::NiBlendBoolInterpolator&>(*m_ptr);
}


nif::NiBlendFloatInterpolator::NiBlendFloatInterpolator() : NiBlendFloatInterpolator(new Niflib::NiBlendFloatInterpolator) {}
nif::NiBlendFloatInterpolator::NiBlendFloatInterpolator(native_type* obj) : NiInterpolator(obj) {}

nif::native::NiBlendFloatInterpolator& nif::NiBlendFloatInterpolator::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiBlendFloatInterpolator::TYPE));
	return static_cast<native::NiBlendFloatInterpolator&>(*m_ptr);
}


nif::NiTimeController::NiTimeController(native_type* obj) :
	NiObject(obj),
	m_flags(*this, &getNative(), &native::NiTimeController::GetFlags, &native::NiTimeController::SetFlags),
	m_frequency(*this, &getNative(), &native::NiTimeController::GetFrequency, &native::NiTimeController::SetFrequency),
	m_phase(*this, &getNative(), &native::NiTimeController::GetPhase, &native::NiTimeController::SetPhase),
	m_startTime(*this, &getNative(), &native::NiTimeController::GetStartTime, &native::NiTimeController::SetStartTime),
	m_stopTime(*this, &getNative(), &native::NiTimeController::GetStopTime, &native::NiTimeController::SetStopTime)
{}

nif::native::NiTimeController& nif::NiTimeController::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiTimeController::TYPE));
	return static_cast<native::NiTimeController&>(*m_ptr);
}


nif::NiSingleInterpController::NiSingleInterpController(native_type* obj) :
	NiTimeController(obj),
	m_iplr(*this, &getNative(), &native::NiSingleInterpController::GetInterpolator, &native::NiSingleInterpController::SetInterpolator)
{
}

nif::native::NiSingleInterpController& nif::NiSingleInterpController::getNative() const
{
	assert(m_ptr && m_ptr->GetType().IsDerivedType(Niflib::NiSingleInterpController::TYPE));
	return static_cast<native::NiSingleInterpController&>(*m_ptr);
}*/
