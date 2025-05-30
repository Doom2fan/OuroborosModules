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

#pragma once

#include <samplerate.h>
#include <rack.hpp>

#include <memory>
#include <string>
#include <vector>

namespace OuroborosModules::Audio {
    struct AudioBuffer {
      private:
        int _sampleRate = 0;
        std::size_t _sampleCount = 0;
        std::vector<float> _samples;

      public:
        AudioBuffer () { }
        AudioBuffer (int sampleRate, std::size_t sampleCount, const std::vector<float>& samples)
            : _sampleRate (sampleRate), _sampleCount (sampleCount), _samples (samples) { }

        int getSampleRate () const { return _sampleRate; }
        std::size_t getSampleCount () const { return _sampleCount; }
        const std::vector<float>& getSamples () const { return _samples; }
    };

    struct AudioSample {
      public:
        enum class LoadStatus {
            Success,
            FileDoesntExist,
            InvalidFile,
            ChannelCount,
        };

        static LoadStatus load (std::string path, int sampleRate, std::shared_ptr<AudioSample>& audioSample);
        static LoadStatus load (std::string path, std::shared_ptr<AudioSample>& audioSample);
        static LoadStatus loadShared (std::string path, int sampleRate, std::shared_ptr<AudioSample>& audioSample);
        static LoadStatus loadSharedRaw (std::string path, std::shared_ptr<AudioSample>& audioSample);

        bool isShared () const { return _isShared; }
        bool isStereo () const { return _isStereo; }
        int getChannelCount () const { return isStereo () ? 2 : 1; }
        const AudioBuffer& getRawBuffer () const { return _rawBuffer; }
        const AudioBuffer& getResampledBuffer () const { return _noResampledBuffer ? _rawBuffer : _resampledBuffer; }

        void onSampleRateChange (int newSampleRate);

        std::shared_ptr<AudioSample> withSampleRate (int newSampleRate, bool shared);

      private:
        bool _isShared;
        bool _isStereo;
        bool _rawOnly;
        bool _noResampledBuffer;
        AudioBuffer _rawBuffer;
        AudioBuffer _resampledBuffer;

        void generateSamples (int sampleRate);

        static LoadStatus loadInternal (std::string path, std::shared_ptr<AudioSample>& audioSample, bool isShared, int sampleRate);
    };

    struct SampleChannel {
      private:
        int _curSampleRate = 0;

        bool _isPlaying = false;
        bool _isPlayingRaw = false;
        std::size_t _sampleTime = 0;

        SRC_STATE* _src = nullptr;
        double _srcRatio = 0.f;
        rack::dsp::DoubleRingBuffer<float, 16 * 2> outBuffer;

        std::shared_ptr<AudioSample> _sampleAudio = nullptr;

        void calcSrcRatio ();
        void decideBuffer ();
        const AudioBuffer& getBuffer () const;

      public:
        SampleChannel ();
        ~SampleChannel ();

        bool process (float& audioLeft, float& audioRight);

        void load (std::shared_ptr<AudioSample> sample);

        bool isPlaying () { return _isPlaying; }
        void play ();
        void play (std::shared_ptr<AudioSample> sample);
        void reset ();

        void onSampleRateChange (int newSampleRate);
    };

    std::string getErrorMessage (AudioSample::LoadStatus loadStatus, std::string path);
}