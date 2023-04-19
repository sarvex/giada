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

#ifndef G_CONFIG_API_H
#define G_CONFIG_API_H

#include "core/kernelAudio.h"
#include <vector>

namespace giada::m::model
{
class Model;
}

namespace giada::m
{
class ConfigApi
{
public:
	ConfigApi(model::Model&, KernelAudio&, KernelMidi&, MidiMapper<KernelMidi>&);

	bool                             audio_hasAPI(RtAudio::Api) const;
	RtAudio::Api                     audio_getAPI() const;
	std::vector<KernelAudio::Device> audio_getAvailableDevices() const;
	KernelAudio::Device              audio_getCurrentOutDevice() const;
	KernelAudio::Device              audio_getCurrentInDevice() const;
	bool                             audio_isLimitOutput() const;
	float                            audio_getRecTriggerLevel() const;
	Resampler::Quality               audio_getResamplerQuality() const;
	int                              audio_getSampleRate() const;
	int                              audio_getBufferSize() const;

	void audio_setAPI(RtAudio::Api);

	bool audio_openStream(
	    const model::KernelAudio::Device& out,
	    const model::KernelAudio::Device& in,
	    unsigned int                      sampleRate,
	    unsigned int                      bufferSize);

	void audio_storeData(bool limitOutput, Resampler::Quality, float recTriggerLevel);

	bool                            midi_hasAPI(RtMidi::Api) const;
	RtMidi::Api                     midi_getAPI() const;
	int                             midi_getSyncMode() const;
	int                             midi_getCurrentOutPort() const;
	int                             midi_getCurrentInPort() const;
	std::vector<std::string>        midi_getOutPorts() const;
	std::vector<std::string>        midi_getInPorts() const;
	const std::vector<std::string>& midi_getMidiMapFilesFound() const;

	bool midi_setAPI(RtMidi::Api);
	bool midi_openPorts(int out, int in);

	void midi_storeData(int syncMode);

private:
	model::Model&           m_model;
	KernelAudio&            m_kernelAudio;
	KernelMidi&             m_kernelMidi;
	MidiMapper<KernelMidi>& m_midiMapper;
};
} // namespace giada::m

#endif
