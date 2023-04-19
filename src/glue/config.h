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

#ifndef G_GLUE_CONFIG_H
#define G_GLUE_CONFIG_H

#include "core/kernelAudio.h"
#include "core/types.h"
#include <RtMidi.h>
#include <map>
#include <string>
#include <vector>

/* giada::c::config
Functions to interact with the Configuration panel. Only the main thread can use 
these! */

namespace giada::c::config
{
enum class DeviceType
{
	INPUT,
	OUTPUT
};

struct AudioDeviceData
{
	AudioDeviceData() = default;
	AudioDeviceData(DeviceType t, const m::KernelAudio::Device&);

	DeviceType                type        = DeviceType::OUTPUT;
	int                       index       = -1;
	std::string               name        = "";
	int                       channelsMax = 0;
	std::vector<unsigned int> sampleRates = {};

	/* Selectable values. */

	int channelsCount = 0;
	int channelsStart = 0;
};

struct AudioData
{
	void setOutputDevice(int index);
	void setInputDevice(int index);

	std::map<RtAudio::Api, std::string> apis;
	std::vector<AudioDeviceData>        outputDevices;
	std::vector<AudioDeviceData>        inputDevices;

	/* Selectable values. */

	RtAudio::Api    api;
	AudioDeviceData outputDevice;
	AudioDeviceData inputDevice;
	int             bufferSize;
	int             sampleRate;
	bool            limitOutput;
	float           recTriggerLevel;
	int             resampleQuality;
};

struct MidiData
{
	std::map<RtMidi::Api, std::string> apis;
	std::map<int, std::string>         syncModes;
	std::vector<std::string>           midiMaps;
	std::vector<std::string>           outPorts;
	std::vector<std::string>           inPorts;

	/* Selectable values. */

	RtMidi::Api api;
	int         syncMode;
	int         midiMap;
	int         outPort;
	int         inPort;
};

struct PluginData
{
	int         numAvailablePlugins;
	std::string pluginPath;
};

struct MiscData
{
	int                      logMode;
	bool                     showTooltips;
	std::vector<std::string> langMaps;
	float                    uiScaling;

	/* Selectable values. */

	std::string langMap;
};

struct BehaviorsData
{
	bool chansStopOnSeqHalt;
	bool treatRecsAsLoops;
	bool inputMonitorDefaultOn;
	bool overdubProtectionDefaultOn;
};

/* get*
Return viewModel objects filled with data. */

AudioData     getAudioData();
MidiData      getMidiData();
PluginData    getPluginData();
MiscData      getMiscData();
BehaviorsData getBehaviorsData();

void changeAudioAPI(RtAudio::Api);
void changeMidiAPI(RtMidi::Api);

void save(const AudioData&);
bool save(const MidiData&);
void save(const MiscData&);
void save(const PluginData&);
void save(const BehaviorsData&);
void scanPlugins(std::string dir, const std::function<void(float)>& progress);

/* setPluginPathCb
Callback attached to the DirBrowser for adding new Plug-in search paths in the
configuration window. */

void setPluginPathCb(void* data);

} // namespace giada::c::config

#endif
