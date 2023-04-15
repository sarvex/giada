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

#include "core/kernelMidi.h"
#include "core/const.h"
#include "core/midiEvent.h"
#include "core/model/kernelAudio.h"
#include "utils/log.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>

namespace giada::m
{
namespace
{
constexpr auto OUTPUT_NAME       = "Giada MIDI output";
constexpr auto INPUT_NAME        = "Giada MIDI input";
constexpr int  MAX_RTMIDI_EVENTS = 8;
constexpr int  MAX_NUM_PRODUCERS = 2; // Real-time thread and MIDI sync thread
} // namespace

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

KernelMidi::KernelMidi(model::Model& m)
: onMidiReceived(nullptr)
, onMidiSent(nullptr)
, m_model(m)
, m_worker(G_KERNEL_MIDI_OUTPUT_RATE_MS)
, m_midiQueue(MAX_RTMIDI_EVENTS, 0, MAX_NUM_PRODUCERS) // See https://github.com/cameron314/concurrentqueue#preallocation-correctly-using-try_enqueue
, m_elpsedTime(0.0)
{
}

/* -------------------------------------------------------------------------- */

bool KernelMidi::init()
{
	const model::KernelMidi& kernelMidi = m_model.get().kernelMidi;

	openOutDevice(kernelMidi.system, kernelMidi.portOut);
	openInDevice(kernelMidi.system, kernelMidi.portIn);

	return true; // TODO
}

/* -------------------------------------------------------------------------- */

void KernelMidi::start()
{
	if (m_midiOut == nullptr)
		return;
	m_worker.start([this]() {
		RtMidiMessage msg;
		while (m_midiQueue.try_dequeue(msg))
			m_midiOut->sendMessage(&msg);
	});
}

/* -------------------------------------------------------------------------- */

bool KernelMidi::openOutDevice(RtMidi::Api api, int port)
{
	if (port == -1)
		return false;

	u::log::print("[KM] Opening output - API=%d, port=%d, device='%s'\n", api, port, OUTPUT_NAME);

	m_midiOut = makeDevice<RtMidiOut>(api, OUTPUT_NAME);
	if (m_midiOut == nullptr)
		return false;

	return openPort(*m_midiOut, port);
}

/* -------------------------------------------------------------------------- */

bool KernelMidi::openInDevice(RtMidi::Api api, int port)
{
	if (port == -1)
		return false;

	u::log::print("[KM] Opening input - API=%d, port=%d, device '%s'\n", api, port, INPUT_NAME);

	m_midiIn = makeDevice<RtMidiIn>(api, INPUT_NAME);
	if (m_midiIn == nullptr)
		return false;

	if (!openPort(*m_midiIn, port))
		return false;

	m_midiIn->setCallback(&s_callback, this);
	m_midiIn->ignoreTypes(true, /*midiTime=*/false, true); // Don't ignore time msgs

	return true;
}

/* -------------------------------------------------------------------------- */

void KernelMidi::logPorts()
{
	if (m_midiOut != nullptr)
		logPorts(*m_midiOut, OUTPUT_NAME);
	if (m_midiIn != nullptr)
		logPorts(*m_midiIn, INPUT_NAME);
}

/* -------------------------------------------------------------------------- */

bool KernelMidi::hasAPI(RtMidi::Api API) const
{
	std::vector<RtMidi::Api> APIs;
	RtMidi::getCompiledApi(APIs);
	for (unsigned i = 0; i < APIs.size(); i++)
		if (APIs.at(i) == API)
			return true;
	return false;
}

/* -------------------------------------------------------------------------- */

std::vector<std::string> KernelMidi::getOutPorts() const
{
	std::vector<std::string> out;
	for (unsigned i = 0; i < countOutPorts(); i++)
		out.push_back(getPortName(*m_midiOut, i));
	return out;
}

std::vector<std::string> KernelMidi::getInPorts() const
{
	std::vector<std::string> out;
	for (unsigned i = 0; i < countInPorts(); i++)
		out.push_back(getPortName(*m_midiIn, i));
	return out;
}

/* -------------------------------------------------------------------------- */

bool KernelMidi::send(const MidiEvent& event) const
{
	if (m_midiOut == nullptr)
		return false;

	assert(event.getNumBytes() > 0 && event.getNumBytes() <= 3);
	assert(onMidiSent != nullptr);

	RtMidiMessage msg;
	if (event.getNumBytes() == 1)
		msg = {event.getByte1()};
	else if (event.getNumBytes() == 2)
		msg = {event.getByte1(), event.getByte2()};
	else
		msg = {event.getByte1(), event.getByte2(), event.getByte3()};

	G_DEBUG("Send MIDI msg=0x{:0X}", event.getRaw());

	onMidiSent();

	return m_midiQueue.try_enqueue(msg);
}

/* -------------------------------------------------------------------------- */

unsigned KernelMidi::countOutPorts() const { return m_midiOut != nullptr ? m_midiOut->getPortCount() : 0; }
unsigned KernelMidi::countInPorts() const { return m_midiIn != nullptr ? m_midiIn->getPortCount() : 0; }

/* -------------------------------------------------------------------------- */

void KernelMidi::s_callback(double deltatime, RtMidiMessage* msg, void* data)
{
	static_cast<KernelMidi*>(data)->callback(deltatime, msg);
}

/* -------------------------------------------------------------------------- */

void KernelMidi::callback(double deltatime, RtMidiMessage* msg)
{
	assert(onMidiReceived != nullptr);
	assert(msg->size() > 0);

	m_elpsedTime += deltatime;

	MidiEvent event;
	if (msg->size() == 1)
		event = MidiEvent::makeFrom1Byte((*msg)[0], m_elpsedTime);
	else if (msg->size() == 2)
		event = MidiEvent::makeFrom2Bytes((*msg)[0], (*msg)[1], m_elpsedTime);
	else if (msg->size() == 3)
		event = MidiEvent::makeFrom3Bytes((*msg)[0], (*msg)[1], (*msg)[2], m_elpsedTime);
	else
		assert(false); // MIDI messages longer than 3 bytes are not supported

	onMidiReceived(event);

	G_DEBUG("Recv MIDI msg=0x{:0X}, timestamp={}", event.getRaw(), m_elpsedTime);
}

/* -------------------------------------------------------------------------- */

template <typename Device>
std::unique_ptr<Device> KernelMidi::makeDevice(RtMidi::Api api, std::string name) const
{
	try
	{
		return std::make_unique<Device>(static_cast<RtMidi::Api>(api), name);
	}
	catch (RtMidiError& error)
	{
		u::log::print("[KM] Error opening device '%s': %s\n", name.c_str(), error.getMessage());
		return nullptr;
	}
}

template std::unique_ptr<RtMidiOut> KernelMidi::makeDevice(RtMidi::Api, std::string) const;
template std::unique_ptr<RtMidiIn>  KernelMidi::makeDevice(RtMidi::Api, std::string) const;

/* -------------------------------------------------------------------------- */

bool KernelMidi::openPort(RtMidi& device, int port)
{
	try
	{
		device.openPort(port, device.getPortName(port));
		return true;
	}
	catch (RtMidiError& error)
	{
		u::log::print("[KM] Error opening port %d: %s\n", port, error.getMessage());
		return false;
	}
}

/* -------------------------------------------------------------------------- */

std::string KernelMidi::getPortName(RtMidi& device, int port) const
{
	try
	{
		return device.getPortName(port);
	}
	catch (RtMidiError& /*error*/)
	{
		return "";
	}
}

/* -------------------------------------------------------------------------- */

void KernelMidi::logPorts(RtMidi& device, std::string name) const
{
	u::log::print("[KM] Device '%s': %d MIDI ports found\n", name.c_str(), device.getPortCount());
	for (unsigned i = 0; i < device.getPortCount(); i++)
		u::log::print("  %d) %s\n", i, device.getPortName(i));
}

/* -------------------------------------------------------------------------- */

void KernelMidi::logCompiledAPIs()
{
	std::vector<RtMidi::Api> apis;
	RtMidi::getCompiledApi(apis);

	u::log::print("[KM] Compiled RtMidi APIs: %d\n", apis.size());

	for (const RtMidi::Api& api : apis)
	{
		switch (api)
		{
		case RtMidi::Api::UNSPECIFIED:
			u::log::print("  UNSPECIFIED\n");
			break;
		case RtMidi::Api::MACOSX_CORE:
			u::log::print("  CoreAudio\n");
			break;
		case RtMidi::Api::LINUX_ALSA:
			u::log::print("  ALSA\n");
			break;
		case RtMidi::Api::UNIX_JACK:
			u::log::print("  JACK\n");
			break;
		case RtMidi::Api::WINDOWS_MM:
			u::log::print("  Microsoft Multimedia MIDI API\n");
			break;
		case RtMidi::Api::RTMIDI_DUMMY:
			u::log::print("  Dummy\n");
			break;
		case RtMidi::Api::WEB_MIDI_API:
			u::log::print("  Web MIDI API\n");
			break;
		default:
			u::log::print("  (unknown)\n");
			break;
		}
	}
}
} // namespace giada::m
