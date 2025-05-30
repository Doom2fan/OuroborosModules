/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
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

#include "Meta.hpp"

namespace OuroborosModules::Modules::Meta {
    void SampleSlot::update () {
        auto data = metaSounds_GetData ((MetaModule::MetaSounds_Channels) channelIdx);
        if (data == nullptr)
            return;

        // Load new samples.
        auto newSample = data->getAudio ();
        if (newSample != sample) {
            sample = newSample;
            sampleChannel.load (newSample);
        }

        // Update the gain.
        auto gain = std::pow (pluginSettings.metaSounds_Volume * data->getVolume (), 2.f);
        audioGain = gain * 5.f; // Bake the 5V scaling into the gain.
    }

    bool SampleSlot::process (float& audioLeft, float& audioRight) {
        auto ret = sampleChannel.process (audioLeft, audioRight);
        if (!ret)
            return false;

        audioLeft *= audioGain;
        audioRight *= audioGain;
        return true;
    }
}