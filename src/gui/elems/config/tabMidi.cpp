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

#include "gui/elems/config/tabMidi.h"
#include "core/const.h"
#include "gui/elems/basics/box.h"
#include "gui/elems/basics/check.h"
#include "gui/elems/basics/choice.h"
#include "gui/elems/config/stringMenu.h"
#include "gui/ui.h"
#include "utils/gui.h"
#include <string>

constexpr int LABEL_WIDTH = 120;

extern giada::v::Ui g_ui;

namespace giada::v
{
geTabMidi::geTabMidi(geompp::Rect<int> bounds)
: Fl_Group(bounds.x, bounds.y, bounds.w, bounds.h, g_ui.getI18Text(LangMap::CONFIG_MIDI_TITLE))
, m_data(c::config::getMidiData())
{
	end();

	geFlex* body = new geFlex(bounds.reduced(G_GUI_OUTER_MARGIN), Direction::VERTICAL, G_GUI_OUTER_MARGIN);
	{
		system = new geChoice(g_ui.getI18Text(LangMap::CONFIG_MIDI_SYSTEM), LABEL_WIDTH);

		geFlex* line1 = new geFlex(Direction::HORIZONTAL, G_GUI_OUTER_MARGIN);
		{
			portOut   = new geStringMenu(g_ui.getI18Text(LangMap::CONFIG_MIDI_OUTPUTPORT),
                m_data.outPorts, g_ui.getI18Text(LangMap::CONFIG_MIDI_NOPORTSFOUND), LABEL_WIDTH);
			enableOut = new geCheck(0, 0, 0, 0);

			line1->add(portOut);
			line1->add(enableOut, 12);
			line1->end();
		}

		geFlex* line2 = new geFlex(Direction::HORIZONTAL, G_GUI_OUTER_MARGIN);
		{
			portIn   = new geStringMenu(g_ui.getI18Text(LangMap::CONFIG_MIDI_INPUTPORT),
                m_data.inPorts, g_ui.getI18Text(LangMap::CONFIG_MIDI_NOPORTSFOUND), LABEL_WIDTH);
			enableIn = new geCheck(0, 0, 0, 0);

			line2->add(portIn);
			line2->add(enableIn, 12);
			line2->end();
		}

		midiMap = new geStringMenu(g_ui.getI18Text(LangMap::CONFIG_MIDI_OUTPUTMIDIMAP),
		    m_data.midiMaps, g_ui.getI18Text(LangMap::CONFIG_MIDI_NOMIDIMAPSFOUND), LABEL_WIDTH);
		sync    = new geChoice(g_ui.getI18Text(LangMap::CONFIG_MIDI_SYNC), LABEL_WIDTH);

		body->add(system, 20);
		body->add(line1, 20);
		body->add(line2, 20);
		body->add(midiMap, 20);
		body->add(sync, 20);
		body->add(new geBox(g_ui.getI18Text(LangMap::CONFIG_RESTARTGIADA)));
		body->end();
	}

	add(body);
	resizable(body);

	system->onChange = [this](ID id) {
		m_data.api = static_cast<RtMidi::Api>(id);
	};

	portOut->onChange = [this](ID id) { m_data.outPort = id; };

	portIn->onChange = [this](ID id) { m_data.inPort = id; };

	enableOut->copy_tooltip(g_ui.getI18Text(LangMap::CONFIG_MIDI_LABEL_ENABLEOUT));
	enableOut->onChange = [this](bool b) {
		if (b)
		{
			m_data.outPort = portOut->getSelectedId();
			portOut->activate();
		}
		else
		{
			m_data.outPort = -1;
			portOut->deactivate();
		}
	};

	enableIn->copy_tooltip(g_ui.getI18Text(LangMap::CONFIG_MIDI_LABEL_ENABLEIN));
	enableIn->onChange = [this](bool b) {
		if (b)
		{
			m_data.inPort = portIn->getSelectedId();
			portIn->activate();
		}
		else
		{
			m_data.inPort = -1;
			portIn->deactivate();
		}
	};

	midiMap->onChange = [this](ID id) { m_data.midiMap = id; };

	sync->onChange = [this](ID id) { m_data.syncMode = id; };

	rebuild(c::config::getMidiData());
}

/* -------------------------------------------------------------------------- */

void geTabMidi::rebuild(const c::config::MidiData& data)
{
	m_data = data;

	for (const auto& [key, value] : m_data.apis)
		system->addItem(value.c_str(), key);
	system->showItem(m_data.api);

	portOut->showItem(m_data.outPort);
	if (m_data.outPort == -1)
		portOut->deactivate();

	portIn->showItem(m_data.inPort);
	if (m_data.inPort == -1)
		portIn->deactivate();

	enableOut->value(m_data.outPort != -1);

	enableIn->value(m_data.inPort != -1);

	midiMap->showItem(m_data.midiMap);

	for (const auto& [key, value] : m_data.syncModes)
		sync->addItem(value.c_str(), key);
	sync->showItem(m_data.syncMode);
}

/* -------------------------------------------------------------------------- */

void geTabMidi::save() const
{
	c::config::save(m_data);
}
} // namespace giada::v
