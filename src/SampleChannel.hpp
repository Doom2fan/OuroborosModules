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

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace OuroborosModules::Audio {
    struct AudioSample {
      public:
        enum class LoadStatus {
            Success,
            FileDoesntExist,
            InvalidFile,
            ChannelCount,
        };

        std::vector<float> samples [2];

        bool isStereo () { return _isStereo; }
        int getRawSampleRate () { return _rawSampleRate; }

        LoadStatus load (std::string path);
        void clear ();

        void onSampleRateChange (int newSampleRate);

      private:
        int _curSampleRate;
        int _rawSampleRate;
        std::vector<float> _rawSamples [2];
        bool _isStereo;

        void generateSamples ();
    };

    struct SampleChannel {
      private:
        int _curSampleRate;

        bool _isPlaying;
        int _sampleTime;

        std::shared_ptr<AudioSample> _sampleAudio;

      public:
        bool process (float& audioLeft, float& audioRight);

        void load (std::shared_ptr<AudioSample> sample);

        bool isPlaying () { return _isPlaying; }
        void play ();
        void play (std::shared_ptr<AudioSample> sample);
        void reset ();

        void onSampleRateChange (int sampleRate);
    };

    std::string getErrorMessage (AudioSample::LoadStatus loadStatus, std::string path);
}