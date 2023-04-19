/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2023 Giovanni A. Zuliani | Monocasual Laboratories
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */

#include "core/model/model.h"
#include "utils/log.h"
#include "utils/string.h"
#include <cassert>
#include <memory>
#ifdef G_DEBUG_MODE
#include "core/channels/channelFactory.h"
#include <fmt/core.h>
#endif
#include <fmt/ostream.h>

using namespace mcl;

namespace giada::m::model
{
namespace
{
template <typename T>
auto getIter_(const std::vector<std::unique_ptr<T>>& source, ID id)
{
	return u::vector::findIf(source, [id](const std::unique_ptr<T>& p) { return p->id == id; });
}

/* -------------------------------------------------------------------------- */

template <typename S>
auto* get_(S& source, ID id)
{
	auto it = getIter_(source, id);
	return it == source.end() ? nullptr : it->get();
}

/* -------------------------------------------------------------------------- */

template <typename D, typename T>
void remove_(D& dest, T& ref)
{
	u::vector::removeIf(dest, [&ref](const auto& other) { return other.get() == &ref; });
}
} // namespace

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#ifdef G_DEBUG_MODE

void Layout::debug() const
{
	mixer.debug();
	channels.debug();
}

#endif

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

DataLock::DataLock(Model& m, SwapType t)
: m_model(m)
, m_swapType(t)
{
	m_model.get().locked = true;
	m_model.swap(SwapType::NONE);
}

DataLock::~DataLock()
{
	m_model.get().locked = false;
	m_model.swap(m_swapType);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

Model::Model()
: onSwap(nullptr)
{
}

/* -------------------------------------------------------------------------- */

void Model::init()
{
	m_shared = {};

	Layout& layout          = get();
	layout                  = {};
	layout.sequencer.shared = &m_shared.sequencerShared;
	layout.mixer.shared     = &m_shared.mixerShared;

	swap(SwapType::NONE);
}

/* -------------------------------------------------------------------------- */

void Model::reset()
{
	m_shared = {};

	Layout& layout          = get();
	layout.sequencer        = {};
	layout.sequencer.shared = &m_shared.sequencerShared;
	layout.mixer            = {};
	layout.mixer.shared     = &m_shared.mixerShared;
	layout.channels         = {};

	swap(SwapType::NONE);
}

/* -------------------------------------------------------------------------- */

void Model::load(const Conf& conf)
{
	Layout& layout = get();

	layout.kernelAudio.api                     = conf.soundSystem;
	layout.kernelAudio.deviceOut.index         = conf.soundDeviceOut;
	layout.kernelAudio.deviceOut.channelsCount = conf.channelsOutCount;
	layout.kernelAudio.deviceOut.channelsStart = conf.channelsOutStart;
	layout.kernelAudio.deviceIn.index          = conf.soundDeviceIn;
	layout.kernelAudio.deviceIn.channelsCount  = conf.channelsInCount;
	layout.kernelAudio.deviceIn.channelsStart  = conf.channelsInStart;
	layout.kernelAudio.samplerate              = conf.samplerate;
	layout.kernelAudio.buffersize              = conf.buffersize;
	layout.kernelAudio.limitOutput             = conf.limitOutput;
	layout.kernelAudio.rsmpQuality             = conf.rsmpQuality;
	layout.kernelAudio.recTriggerLevel         = conf.recTriggerLevel;

	layout.kernelMidi.api         = conf.midiSystem;
	layout.kernelMidi.portOut     = conf.midiPortOut;
	layout.kernelMidi.portIn      = conf.midiPortIn;
	layout.kernelMidi.midiMapPath = conf.midiMapPath;
	layout.kernelMidi.sync        = conf.midiSync;

	layout.mixer.inputRecMode   = conf.inputRecMode;
	layout.mixer.recTriggerMode = conf.recTriggerMode;

	layout.midiIn.enabled    = conf.midiInEnabled;
	layout.midiIn.filter     = conf.midiInFilter;
	layout.midiIn.rewind     = conf.midiInRewind;
	layout.midiIn.startStop  = conf.midiInStartStop;
	layout.midiIn.actionRec  = conf.midiInActionRec;
	layout.midiIn.inputRec   = conf.midiInInputRec;
	layout.midiIn.metronome  = conf.midiInMetronome;
	layout.midiIn.volumeIn   = conf.midiInVolumeIn;
	layout.midiIn.volumeOut  = conf.midiInVolumeOut;
	layout.midiIn.beatDouble = conf.midiInBeatDouble;
	layout.midiIn.beatHalf   = conf.midiInBeatHalf;

	layout.chansStopOnSeqHalt         = conf.chansStopOnSeqHalt;
	layout.treatRecsAsLoops           = conf.treatRecsAsLoops;
	layout.inputMonitorDefaultOn      = conf.inputMonitorDefaultOn;
	layout.overdubProtectionDefaultOn = conf.overdubProtectionDefaultOn;

	swap(model::SwapType::NONE);
}

/* -------------------------------------------------------------------------- */

void Model::store(Conf& conf) const
{
	const Layout& layout = get();

	conf.soundSystem      = layout.kernelAudio.api;
	conf.soundDeviceOut   = layout.kernelAudio.deviceOut.index;
	conf.channelsOutCount = layout.kernelAudio.deviceOut.channelsCount;
	conf.channelsOutStart = layout.kernelAudio.deviceOut.channelsStart;
	conf.soundDeviceIn    = layout.kernelAudio.deviceIn.index;
	conf.channelsInCount  = layout.kernelAudio.deviceIn.channelsCount;
	conf.channelsInStart  = layout.kernelAudio.deviceIn.channelsStart;
	conf.samplerate       = layout.kernelAudio.samplerate;
	conf.buffersize       = layout.kernelAudio.buffersize;
	conf.limitOutput      = layout.kernelAudio.limitOutput;
	conf.rsmpQuality      = layout.kernelAudio.rsmpQuality;
	conf.recTriggerLevel  = layout.kernelAudio.recTriggerLevel;

	conf.midiSystem  = layout.kernelMidi.api;
	conf.midiPortOut = layout.kernelMidi.portOut;
	conf.midiPortIn  = layout.kernelMidi.portIn;
	conf.midiMapPath = layout.kernelMidi.midiMapPath;
	conf.midiSync    = layout.kernelMidi.sync;

	conf.inputRecMode   = layout.mixer.inputRecMode;
	conf.recTriggerMode = layout.mixer.recTriggerMode;

	conf.midiInEnabled    = layout.midiIn.enabled;
	conf.midiInFilter     = layout.midiIn.filter;
	conf.midiInRewind     = layout.midiIn.rewind;
	conf.midiInStartStop  = layout.midiIn.startStop;
	conf.midiInActionRec  = layout.midiIn.actionRec;
	conf.midiInInputRec   = layout.midiIn.inputRec;
	conf.midiInMetronome  = layout.midiIn.metronome;
	conf.midiInVolumeIn   = layout.midiIn.volumeIn;
	conf.midiInVolumeOut  = layout.midiIn.volumeOut;
	conf.midiInBeatDouble = layout.midiIn.beatDouble;
	conf.midiInBeatHalf   = layout.midiIn.beatHalf;

	conf.chansStopOnSeqHalt         = layout.chansStopOnSeqHalt;
	conf.treatRecsAsLoops           = layout.treatRecsAsLoops;
	conf.inputMonitorDefaultOn      = layout.inputMonitorDefaultOn;
	conf.overdubProtectionDefaultOn = layout.overdubProtectionDefaultOn;
}

/* -------------------------------------------------------------------------- */

bool Model::registerThread(Thread t, bool realtime) const
{
	return m_swapper.registerThread(u::string::toString(t), realtime);
}

/* -------------------------------------------------------------------------- */

Layout&       Model::get() { return m_swapper.get(); }
const Layout& Model::get() const { return m_swapper.get(); }
LayoutLock    Model::get_RT() const { return LayoutLock(m_swapper); }

/* -------------------------------------------------------------------------- */

void Model::set(const Layout& layout)
{
	get() = layout;
	swap(model::SwapType::NONE);
}

/* -------------------------------------------------------------------------- */

void Model::swap(SwapType t)
{
	m_swapper.swap();
	if (onSwap != nullptr)
		onSwap(t);
}

/* -------------------------------------------------------------------------- */

DataLock Model::lockData(SwapType t)
{
	return DataLock(*this, t);
}

/* -------------------------------------------------------------------------- */

bool Model::isLocked() const
{
	return m_swapper.isRtLocked();
}

/* -------------------------------------------------------------------------- */

template <typename T>
T& Model::getAllShared()
{
	if constexpr (std::is_same_v<T, PluginPtrs>)
		return m_shared.plugins;
	if constexpr (std::is_same_v<T, WavePtrs>)
		return m_shared.waves;
	if constexpr (std::is_same_v<T, Actions::Map>)
		return m_shared.actions;
	if constexpr (std::is_same_v<T, ChannelSharedPtrs>)
		return m_shared.channelsShared;

	assert(false);
}

template PluginPtrs&        Model::getAllShared<PluginPtrs>();
template WavePtrs&          Model::getAllShared<WavePtrs>();
template Actions::Map&      Model::getAllShared<Actions::Map>();
template ChannelSharedPtrs& Model::getAllShared<ChannelSharedPtrs>();

/* -------------------------------------------------------------------------- */

template <typename T>
T* Model::findShared(ID id)
{
	if constexpr (std::is_same_v<T, Plugin>)
		return get_(m_shared.plugins, id);
	if constexpr (std::is_same_v<T, Wave>)
		return get_(m_shared.waves, id);

	assert(false);
}

template Plugin* Model::findShared<Plugin>(ID id);
template Wave*   Model::findShared<Wave>(ID id);

/* -------------------------------------------------------------------------- */

template <typename T>
void Model::addShared(T obj)
{
	if constexpr (std::is_same_v<T, PluginPtr>)
		m_shared.plugins.push_back(std::move(obj));
	if constexpr (std::is_same_v<T, WavePtr>)
		m_shared.waves.push_back(std::move(obj));
	if constexpr (std::is_same_v<T, ChannelSharedPtr>)
		m_shared.channelsShared.push_back(std::move(obj));
}

template void Model::addShared<PluginPtr>(PluginPtr p);
template void Model::addShared<WavePtr>(WavePtr p);
template void Model::addShared<ChannelSharedPtr>(ChannelSharedPtr p);

/* -------------------------------------------------------------------------- */

template <typename T>
void Model::removeShared(const T& ref)
{
	if constexpr (std::is_same_v<T, Plugin>)
		remove_(m_shared.plugins, ref);
	if constexpr (std::is_same_v<T, Wave>)
		remove_(m_shared.waves, ref);
}

template void Model::removeShared<Plugin>(const Plugin& t);
template void Model::removeShared<Wave>(const Wave& t);

/* -------------------------------------------------------------------------- */

template <typename T>
T& Model::backShared()
{
	if constexpr (std::is_same_v<T, Plugin>)
		return *m_shared.plugins.back().get();
	if constexpr (std::is_same_v<T, Wave>)
		return *m_shared.waves.back().get();
	if constexpr (std::is_same_v<T, ChannelShared>)
		return *m_shared.channelsShared.back().get();
}

template Plugin&        Model::backShared<Plugin>();
template Wave&          Model::backShared<Wave>();
template ChannelShared& Model::backShared<ChannelShared>();

/* -------------------------------------------------------------------------- */

template <typename T>
void Model::clearShared()
{
	if constexpr (std::is_same_v<T, PluginPtrs>)
		m_shared.plugins.clear();
	if constexpr (std::is_same_v<T, WavePtrs>)
		m_shared.waves.clear();
}

template void Model::clearShared<PluginPtrs>();
template void Model::clearShared<WavePtrs>();

/* -------------------------------------------------------------------------- */

#ifdef G_DEBUG_MODE

void Model::debug()
{
	puts("======== SYSTEM STATUS ========");

	puts("-------------------------------");
	m_swapper.debug();
	puts("-------------------------------");

	get().debug();

	puts("model::channelsShared");

	for (int i = 0; const auto& c : m_shared.channelsShared)
	{
		fmt::print("\t{}) - {}\n", i++, (void*)c.get());
	}

	puts("model::shared.waves");

	for (int i = 0; const auto& w : m_shared.waves)
		fmt::print("\t{}) {} - ID={} name='{}'\n", i++, (void*)w.get(), w->id, w->getPath());

	puts("model::shared.actions");

	for (const auto& [frame, actions] : getAllShared<Actions::Map>())
	{
		fmt::print("\tframe: {}\n", frame);
		for (const Action& a : actions)
			fmt::print("\t\t({}) - ID={}, frame={}, channel={}, value=0x{}, prevId={}, prev={}, nextId={}, next={}\n",
			    (void*)&a, a.id, a.frame, a.channelId, a.event.getRaw(), a.prevId, (void*)a.prev, a.nextId, (void*)a.next);
	}

	puts("model::shared.plugins");

	for (int i = 0; const auto& p : m_shared.plugins)
		fmt::print("\t{}) {} - ID={}\n", i++, (void*)p.get(), p->id);
}

#endif // G_DEBUG_MODE
} // namespace giada::m::model
