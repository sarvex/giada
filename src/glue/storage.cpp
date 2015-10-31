/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * glue
 * Intermediate layer GUI <-> CORE.
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2015 Giovanni A. Zuliani | Monocasual
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


#include "../gui/elems/ge_column.h"
#include "../gui/elems/ge_keyboard.h"
#include "../gui/dialogs/gd_mainWindow.h"
#include "../core/mixer.h"
#include "../core/channel.h"
#include "../core/patch.h"
#include "../core/sampleChannel.h"
#include "../core/midiChannel.h"
#include "../core/wave.h"
#include "../utils/gui_utils.h"
#include "glue.h" // TODO - remove, used only for DEPR calls
#include "storage.h"


extern gdMainWindow *mainWin;
extern Mixer	   		 G_Mixer;
extern Patch         G_Patch;


int glue_savePatch(const char *fullpath, const char *name, bool isProject)
{
	G_Patch.init();

	__fillPatchGlobals__(name);
	__fillPatchChannels__(isProject);
	__fillPatchColumns__();

	if (G_Patch.write(fullpath)) {
		gu_update_win_label(name);
		gLog("[glue] patch saved as %s\n", fullpath);
		return 1;
	}
	return 0;
}


/* -------------------------------------------------------------------------- */


void __fillPatchGlobals__(const char *name)
{
	G_Patch.version      = VERSIONE;
	G_Patch.versionFloat = VERSIONE_FLOAT;
	G_Patch.name         = name;
	G_Patch.bpm          = G_Mixer.bpm;
	G_Patch.bars         = G_Mixer.bars;
	G_Patch.beats        = G_Mixer.beats;
	G_Patch.quantize     = G_Mixer.quantize;
	G_Patch.masterVolIn  = G_Mixer.inVol;
  G_Patch.masterVolOut = G_Mixer.outVol;
  G_Patch.metronome    = G_Mixer.metronome;
}


/* -------------------------------------------------------------------------- */


void __fillPatchChannels__(bool isProject)
{
	for (unsigned i=0; i<G_Mixer.channels.size; i++) {
		G_Mixer.channels.at(i)->fillPatch(&G_Patch, i, isProject);
	}
}


/* -------------------------------------------------------------------------- */


void __fillPatchColumns__()
{
	for (unsigned i=0; i<mainWin->keyboard->getTotalColumns(); i++) {
		gColumn *gCol = mainWin->keyboard->getColumn(i);
		Patch::column_t pCol;
		pCol.index = gCol->getIndex();
		pCol.width = gCol->w();
		for (unsigned k=0; k<gCol->countChannels(); k++) {
			Channel *colChannel = gCol->getChannel(k);
			for (unsigned j=0; j<G_Mixer.channels.size; j++) {
				Channel *mixerChannel = G_Mixer.channels.at(j);
				if (colChannel == mixerChannel) {
					pCol.channels.add(mixerChannel->index);
					break;
				}
			}
		}
		G_Patch.columns.add(pCol);
	}
}


/* -------------------------------------------------------------------------- */


int glue_saveProject(const char *folderPath, const char *projName)
{
	if (gIsProject(folderPath)) {
		gLog("[glue] the project folder already exists\n");
		// don't exit
	}
	else if (!gMkdir(folderPath)) {
		gLog("[glue] unable to make project directory!\n");
		return 0;
	}

	/* copy all samples inside the folder. Takes and logical ones are saved
	 * via glue_saveSample() */

	for (unsigned i=0; i<G_Mixer.channels.size; i++) {

		if (G_Mixer.channels.at(i)->type == CHANNEL_SAMPLE) {

			SampleChannel *ch = (SampleChannel*) G_Mixer.channels.at(i);

			if (ch->wave == NULL)
				continue;

			/* update the new samplePath: everything now comes from the
			 * project folder (folderPath) */

			char samplePath[PATH_MAX];
			sprintf(samplePath, "%s%s%s.%s", folderPath, gGetSlash().c_str(), ch->wave->basename().c_str(), ch->wave->extension().c_str());

			/* remove any existing file */

			if (gFileExists(samplePath))
				remove(samplePath);
			if (ch->save(samplePath))
				ch->wave->pathfile = samplePath;
		}
	}

	char gptcPath[PATH_MAX];
	sprintf(gptcPath, "%s%s%s.gptc", folderPath, gGetSlash().c_str(), gStripExt(projName).c_str());
	glue_savePatch(gptcPath, projName, true); // true == it's a project

	return 1;
}