/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
 *  Copyright (C) 2016-2023 VCV
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

#include "Median.hpp"

#include "../DSP/Filters.hpp"
#include "../Math.hpp"
#include "../Utils.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
    rack::plugin::Model* modelMedian = createModel<Modules::Median::MedianWidget> ("Median");
}

namespace OuroborosModules::Modules::Median {
    MedianModule::MedianModule () {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure parameters.
        configParam (PARAM_OVERSAMPLE, 1.f, MaxOversample, 1.f, "Oversample", "x", 0, 1);

        for (int i = 0; i < 3; i++) {
            configInput (INPUT_VALUES + i, fmt::format (FMT_STRING ("Value {}"), i + 1));

            auto valScaleText = fmt::format (FMT_STRING ("Value {} attenuverter"), i + 1);
            configParam (PARAM_VAL_SCALE + i, -1.f, 1.f, 0.f, valScaleText, "%", 0, 100);

            auto valOffsetText = fmt::format (FMT_STRING ("Value {} offset"), i + 1);
            configParam (PARAM_VAL_OFFSET + i, -1.f, 1.f, 0.f, valOffsetText, "V", 0, 10);
        }

        configOutput (OUTPUT_MIN, "Minimum");
        configOutput (OUTPUT_MID, "Median");
        configOutput (OUTPUT_MAX, "Maximum");

        clockOversample.setDivision (7);
        setOversampleRate (1);

        clockLights.setDivision (32);
    }

    rack::simd::float_4 MedianModule::getBank (int inputNum, int currentChannel) {
        using rack::simd::float_4;
        auto vec = inputs [INPUT_VALUES + inputNum].getPolyVoltageSimd<float_4> (currentChannel);
        vec *= float_4 (params [PARAM_VAL_SCALE + inputNum].getValue ());
        vec += float_4 (params [PARAM_VAL_OFFSET + inputNum].getValue () * 10.f);
        return vec;
    }

    void MedianModule::setOversampleRate (int newOversampleRate) {
        assert (newOversampleRate > 0);
        assert (newOversampleRate <= MaxOversample);

        if (newOversampleRate == oversampleRate)
            return;

        oversampleRate = newOversampleRate;

        for (int bank = 0; bank < SIMDBankCount; bank++) {
            for (int i = 0; i < 3; i++) {
                upsamplerFilter [bank] [i].setOversampleRate (newOversampleRate);
                downsamplerFilter [bank] [i].setOversampleRate (newOversampleRate);
            }
        }
    }

    void MedianModule::process (const ProcessArgs& args) {
        using rack::simd::float_4;

        // Check for oversample updates.
        if (clockOversample.process ()) {
            const auto newOversampleRate = static_cast<int> (params [PARAM_OVERSAMPLE].getValue ());
            setOversampleRate (newOversampleRate);
        }

        // Don't waste CPU if there's nothing connected to the outputs.
        const auto outConnectedMin = outputs [OUTPUT_MIN].isConnected ();
        const auto outConnectedMid = outputs [OUTPUT_MID].isConnected ();
        const auto outConnectedMax = outputs [OUTPUT_MAX].isConnected ();
        if (!outConnectedMin && !outConnectedMid && !outConnectedMax) {
            if (clockLights.process ()) {
                auto lightTime = args.sampleTime * clockLights.getDivision ();
                for (int i = 0; i < 3 * 3; i++)
                    lights [LIGHT_OUTPUT + i].setBrightnessSmooth (0.f, lightTime);
            }

            return;
        }

        const bool inputConnected [3] = {
            inputs [INPUT_VALUES + 0].isConnected (),
            inputs [INPUT_VALUES + 1].isConnected (),
            inputs [INPUT_VALUES + 2].isConnected (),
        };
        const auto oversampleOutMin = (oversampleRate > 1 && outConnectedMin);
        const auto oversampleOutMid = (oversampleRate > 1 && outConnectedMid);
        const auto oversampleOutMax = (oversampleRate > 1 && outConnectedMax);

        // Calculate polyphony and SIMD counts.
        const int channelCount = std::max (1, std::max (
            inputs [INPUT_VALUES + 0].getChannels (),
            std::max (inputs [INPUT_VALUES + 1].getChannels (), inputs [INPUT_VALUES + 2].getChannels ())
        ));
        int bankCount = channelCount / SIMDBankSize;
        if (bankCount * SIMDBankSize < channelCount)
            bankCount++;

        // Set the output polyphony count.
        outputs [OUTPUT_MIN].setChannels (channelCount);
        outputs [OUTPUT_MID].setChannels (channelCount);
        outputs [OUTPUT_MAX].setChannels (channelCount);

        // Generate samples.
        float_4 buffer [3] [MaxOversample];
        for (int bank = 0; bank < bankCount; bank++) {
            const int currentChannel = bank * SIMDBankSize;

            if (oversampleRate > 1) {
                for (auto i = 0; i < 3; i++) {
                    if (inputConnected [i])
                        upsamplerFilter [bank] [i].process (buffer [i], getBank (i, currentChannel));
                    else
                        std::fill (std::begin (buffer [i]), std::begin (buffer [i]) + oversampleRate, getBank (i, currentChannel));
                }
            } else {
                buffer [0] [0] = getBank (0, currentChannel);
                buffer [1] [0] = getBank (1, currentChannel);
                buffer [2] [0] = getBank (2, currentChannel);
            }

            for (int sampleIdx = 0; sampleIdx < oversampleRate; ++sampleIdx) {
                Math::Sort3 (
                    rack::simd::clamp (buffer [0] [sampleIdx], float_4 (-10.f), float_4 (10.f)),
                    rack::simd::clamp (buffer [1] [sampleIdx], float_4 (-10.f), float_4 (10.f)),
                    rack::simd::clamp (buffer [2] [sampleIdx], float_4 (-10.f), float_4 (10.f)),
                    buffer [0] [sampleIdx],
                    buffer [1] [sampleIdx],
                    buffer [2] [sampleIdx]
                );
            }

            auto vecMin = oversampleOutMin ? downsamplerFilter [bank] [0].process (buffer [0]) : buffer [0] [0];
            auto vecMid = oversampleOutMid ? downsamplerFilter [bank] [1].process (buffer [1]) : buffer [1] [0];
            auto vecMax = oversampleOutMax ? downsamplerFilter [bank] [2].process (buffer [2]) : buffer [2] [0];

            outputs [OUTPUT_MIN].setVoltageSimd (vecMin, currentChannel);
            outputs [OUTPUT_MID].setVoltageSimd (vecMid, currentChannel);
            outputs [OUTPUT_MAX].setVoltageSimd (vecMax, currentChannel);
        }

        if (clockLights.process ()) {
            auto lightTime = args.sampleTime * clockLights.getDivision ();
            if (channelCount > 1) {
                // Polyphonic mode. Show blue output lights.
                for (int i = 0; i < 3; i++) {
                    lights [LIGHT_OUTPUT + i * 3 + 0].setBrightnessSmooth (0.f, lightTime);
                    lights [LIGHT_OUTPUT + i * 3 + 1].setBrightnessSmooth (0.f, lightTime);
                    lights [LIGHT_OUTPUT + i * 3 + 2].setBrightnessSmooth (1.f, lightTime);
                }
            } else for (int i = 0; i < 3; i++) {
                // Monophonic mode. Show red (neg) and green (pos) lights.
                float output;
                int lightsIdx;
                switch (i) {
                    case 0: output = outputs [OUTPUT_MIN].getVoltage (); lightsIdx = OUTLIGHT_Min; break;
                    case 1: output = outputs [OUTPUT_MID].getVoltage (); lightsIdx = OUTLIGHT_Mid; break;
                    case 2: output = outputs [OUTPUT_MAX].getVoltage (); lightsIdx = OUTLIGHT_Max; break;
                }
                lights [LIGHT_OUTPUT + lightsIdx + 0].setBrightnessSmooth (std::max (-output, 0.f), lightTime);
                lights [LIGHT_OUTPUT + lightsIdx + 1].setBrightnessSmooth (std::max ( output, 0.f), lightTime);
                lights [LIGHT_OUTPUT + lightsIdx + 2].setBrightnessSmooth (0.f, lightTime);
            }
        }
    }
}