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

DEFINE_BOOL (plugSound_Enable, "plugSound::Enable", true)
DEFINE_FLOAT (float, plugSound_Volume, "plugSound::Volume", 1.f)
DEFINE_STD_STRING (plugSound_ConnectSound, "plugSound::ConnectSound", "<Default>")
DEFINE_STD_STRING (plugSound_DisconnectSound, "plugSound::DisconnectSound", "<Default>")

DEFINE_BOOL (cableColor_Latch, "cableColorManager::Latch", true)
DEFINE_BOOL (cableColor_GlobalKeys, "cableColorManager::GlobalKeys", false)
DEFINE_BOOL (cableColor_DisplayKeys, "cableColorManager::DisplayKeyMappings", true)
DEFINE_BOOL (cableColor_PortHover, "cableColorManager::PortHover", false)
DEFINE_BOOL (cableColor_CenterEmblem, "cableColorManager::CenterEmblem", true)
DEFINE_STRUCT (CableColorModule::CollectionsStorage, cableColor_Collections, "cableColorManager::Collections", CableColorModule::CollectionsStorage::defaults ())
DEFINE_STRUCT (CableColorModule::CableColorKey, cableColor_LatchKey, "cableColorManager::LatchKey", CableColorModule::CableColorKey ())
DEFINE_STRUCT (CableColorModule::CableColorKey, cableColor_CycleFwdKey, "cableColorManager::CycleForwardKey", CableColorModule::CableColorKey ())
DEFINE_STRUCT (CableColorModule::CableColorKey, cableColor_CycleBackKey, "cableColorManager::CycleBackwardKey", CableColorModule::CableColorKey ())

DEFINE_STRUCT (RGBColor, stereoVCA_DefaultDisplayColor, "stereoVCA::DefaultDisplayColor", Colors::DisplayColors.at ("Yellow"))