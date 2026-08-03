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

        setOversampleRate (1);

    }

    void MedianModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        ModuleBase::onSampleRateChange (e);

        // Clock dividers.
        clockOversample = DSP::ClockDivider (static_cast<uint32_t> (e.sampleRate / (48000.f / 32)), rack::random::u32 ());
        clockLights = DSP::ClockDivider (static_cast<uint32_t> (e.sampleRate / (48000.f / 64)), rack::random::u32 ());
    }

    rack::simd::float_4 MedianModule::getBank (int inputNum, int currentChannel) {
        using rack::simd::float_4;
        auto vec = getInputPolySimd<float_4> (INPUT_VALUES + inputNum, currentChannel);
        vec *= float_4 (getParam (PARAM_VAL_SCALE + inputNum));
        vec += float_4 (getParam (PARAM_VAL_OFFSET + inputNum) * 10.f);
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
                upsamplerFilter [bank] [i].setParams (newOversampleRate);
                downsamplerFilter [bank] [i].setParams (newOversampleRate);
            }
        }
    }

    void MedianModule::process (const ProcessArgs& args) {
        using rack::simd::float_4;

        // Check for oversample updates.
        if (clockOversample.process ()) {
            const auto newOversampleRate = static_cast<int> (getParam (PARAM_OVERSAMPLE));
            setOversampleRate (newOversampleRate);
        }

        // Don't waste CPU if there's nothing connected to the outputs.
        const auto outConnectedMin = isOutputConnected (OUTPUT_MIN);
        const auto outConnectedMid = isOutputConnected (OUTPUT_MID);
        const auto outConnectedMax = isOutputConnected (OUTPUT_MAX);
        if (!outConnectedMin && !outConnectedMid && !outConnectedMax) {
            if (clockLights.process ()) {
                auto lightTime = args.sampleTime * clockLights.getDivision ();
                for (int i = 0; i < 3 * 3; i++)
                    setLightSmooth (LIGHT_OUTPUT + i, 0.f, lightTime);
            }

            return;
        }

        const bool inputConnected [3] = {
            isInputConnected (INPUT_VALUES + 0),
            isInputConnected (INPUT_VALUES + 1),
            isInputConnected (INPUT_VALUES + 2),
        };
        const auto oversampleOutMin = (oversampleRate > 1 && outConnectedMin);
        const auto oversampleOutMid = (oversampleRate > 1 && outConnectedMid);
        const auto oversampleOutMax = (oversampleRate > 1 && outConnectedMax);

        // Calculate polyphony and SIMD counts.
        const int channelCount = std::max (1, std::max (
            getInputChannels (INPUT_VALUES + 0),
            std::max (getInputChannels (INPUT_VALUES + 1), getInputChannels (INPUT_VALUES + 2))
        ));
        int bankCount = channelCount / SIMDBankSize;
        if (bankCount * SIMDBankSize < channelCount)
            bankCount++;

        // Set the output polyphony count.
        setOutputChannels (OUTPUT_MIN, channelCount);
        setOutputChannels (OUTPUT_MID, channelCount);
        setOutputChannels (OUTPUT_MAX, channelCount);

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

            setOutputSimd (OUTPUT_MIN, vecMin, currentChannel);
            setOutputSimd (OUTPUT_MID, vecMid, currentChannel);
            setOutputSimd (OUTPUT_MAX, vecMax, currentChannel);
        }

        if (clockLights.process ()) {
            auto lightTime = args.sampleTime * clockLights.getDivision ();
            if (channelCount > 1) {
                // Polyphonic mode. Show blue output lights.
                for (int i = 0; i < 3; i++) {
                    setLightSmooth (LIGHT_OUTPUT + i * 3 + 0, 0.f, lightTime);
                    setLightSmooth (LIGHT_OUTPUT + i * 3 + 1, 0.f, lightTime);
                    setLightSmooth (LIGHT_OUTPUT + i * 3 + 2, 1.f, lightTime);
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
                setLightSmooth (LIGHT_OUTPUT + lightsIdx + 0, std::max (-output, 0.f), lightTime);
                setLightSmooth (LIGHT_OUTPUT + lightsIdx + 1, std::max ( output, 0.f), lightTime);
                setLightSmooth (LIGHT_OUTPUT + lightsIdx + 2, 0.f, lightTime);
            }
        }
    }
}