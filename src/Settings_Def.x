/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

DEFINE_BOOL (debug_Logging, "debug::Logging", false)

DEFINE_STRUCT (ThemeId, global_ThemeLight, "global::ThemeLight", ThemeId::getFromKey ("Light"))
DEFINE_STRUCT (ThemeId, global_ThemeDark, "global::ThemeDark", ThemeId::getFromKey ("Dark"))
DEFINE_STRUCT (EmblemId, global_DefaultEmblem, "global::DefaultEmblem", EmblemId::getFromKey ("Dragon"))

DEFINE_BOOL (metaSounds_Enable, "metaSounds::Enable", true)
DEFINE_FLOAT (float, metaSounds_Volume, "metaSounds::Volume", 1.f)
DEFINE_STRUCT (SoundSettings, metaSounds_CablePlugged, "metaSounds::CablePlugged", SoundSettings (Constants::MetaSound_DefaultMarker, true, 1.f))
DEFINE_STRUCT (SoundSettings, metaSounds_CableUnplugged, "metaSounds::CableUnplugged", SoundSettings (Constants::MetaSound_DefaultMarker, true, 1.f))
DEFINE_STRUCT (SoundSettings, metaSounds_ModulePlaced, "metaSounds::ModulePlaced", SoundSettings (Constants::MetaSound_DefaultMarker, true, 1.f))
DEFINE_STRUCT (SoundSettings, metaSounds_ModuleRemoved, "metaSounds::ModuleRemoved", SoundSettings (Constants::MetaSound_DefaultMarker, true, 1.f))

DEFINE_BOOL (chroma_Latch, "cableColorManager::Latch", true)
DEFINE_BOOL (chroma_GlobalKeys, "cableColorManager::GlobalKeys", false)
DEFINE_BOOL (chroma_DisplayKeys, "cableColorManager::DisplayKeyMappings", true)
DEFINE_BOOL (chroma_PortHover, "cableColorManager::PortHover", false)
DEFINE_BOOL (chroma_CenterEmblem, "cableColorManager::CenterEmblem", true)
DEFINE_STRUCT (Modules::Chroma::CollectionsStorage, chroma_Collections, "cableColorManager::Collections", Modules::Chroma::CollectionsStorage::defaults ())
DEFINE_STRUCT (Modules::Chroma::CableColorKey, chroma_LatchKey, "cableColorManager::LatchKey", Modules::Chroma::CableColorKey ())
DEFINE_STRUCT (Modules::Chroma::CableColorKey, chroma_CycleFwdKey, "cableColorManager::CycleForwardKey", Modules::Chroma::CableColorKey ())
DEFINE_STRUCT (Modules::Chroma::CableColorKey, chroma_CycleBackKey, "cableColorManager::CycleBackwardKey", Modules::Chroma::CableColorKey ())

DEFINE_STRUCT (RGBColor, stVCA_DefaultDisplayColor, "stereoVCA::DefaultDisplayColor", Colors::DisplayColors.at ("Yellow"))