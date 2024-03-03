DEFINE_BOOL (debug_Logging, "debug::Logging", false)

DEFINE_ENUM (ThemeKind, global_ThemeLight, "global::ThemeLight", ThemeKind::Light)
DEFINE_ENUM (ThemeKind, global_ThemeDark, "global::ThemeDark", ThemeKind::Dark)
DEFINE_ENUM (EmblemKind, global_DefaultEmblem, "global::DefaultEmblem", EmblemKind::Dragon)

DEFINE_BOOL (plugSound_Enable, "plugSound::Enable", true)
DEFINE_FLOAT (float, plugSound_Volume, "plugSound::Volume", 1.f)
DEFINE_STD_STRING (plugSound_ConnectSound, "plugSound::ConnectSound", "<Default>")
DEFINE_STD_STRING (plugSound_DisconnectSound, "plugSound::DisconnectSound", "<Default>")

DEFINE_INT (int, cables_CalcRate, "cables::CalcRate", 120)