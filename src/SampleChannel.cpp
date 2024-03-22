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

#include "SampleChannel.hpp"

#include "PluginDef.hpp"

#include <audioFile.h>
#include <fmt/format.h>
#include <samplerate.h>

namespace OuroborosModules {
namespace Audio {
    void AudioSample::generateSamples () {
        samples [0] = std::vector<float> ();
        samples [1] = std::vector<float> ();

        if (_rawSamples [0].size () < 1)
            return;

        // Set up the resampler.
        auto src = src_new (SRC_SINC_FASTEST, 1, nullptr);
        assert (src != nullptr);

        // Set up the resampling info.
        const int bufferSize = 128;
        float tmpBuffer [bufferSize];
        SRC_DATA srcData;

        srcData.data_out = tmpBuffer;
        srcData.output_frames = bufferSize;
        srcData.src_ratio = static_cast<double> (_curSampleRate) / getRawSampleRate ();

        for (int i = 0, count = isStereo () ? 2 : 1; i < count; i++) {
            if (i > 0)
                src_reset (src);

            srcData.data_in = _rawSamples [i].data ();

            int totalSize = _rawSamples [i].size ();
            int curSize = 0;
            while (curSize < totalSize) {
                srcData.input_frames = std::min (totalSize - curSize, bufferSize);
                srcData.end_of_input = (totalSize - curSize) <= bufferSize;

                src_process (src, &srcData);
                samples [i].reserve (samples [i].size () + srcData.output_frames_gen);
                samples [i].insert (samples [i].end (), std::begin (tmpBuffer), std::begin (tmpBuffer) + srcData.output_frames_gen);

                srcData.data_in += srcData.input_frames_used;
                curSize += srcData.input_frames_used;
            }
        }

        src_delete (src);
    }

    AudioSample::LoadStatus AudioSample::load (std::string path) {
        {
            FILE* file = fopen (path.c_str (), "r");
            if (file == nullptr)
                return LoadStatus::FileDoesntExist;
            fclose (file);
        }

        AudioFile<float> audioFile;
        audioFile.shouldLogErrorsToConsole (false);

        if (!audioFile.load (path))
            return LoadStatus::InvalidFile;

        if (!audioFile.isMono () && !audioFile.isStereo ())
            return LoadStatus::ChannelCount;

        _curSampleRate = APP->engine->getSampleRate ();
        _rawSampleRate = audioFile.getSampleRate ();
        _isStereo = audioFile.isStereo ();
        for (int i = 0, count = std::min (audioFile.getNumChannels (), 2); i < count; i++)
            _rawSamples [i] = audioFile.samples [i];

        assert (!isStereo () || _rawSamples [0].size () == _rawSamples [1].size ());

        generateSamples ();
        assert (!isStereo () || samples [0].size () == samples [1].size ());

        return LoadStatus::Success;
    }

    void AudioSample::clear () {
        _rawSampleRate = 0;
        _isStereo = false;

        for (int i = 0; i < 2; i++) {
            _rawSamples [i] = std::vector<float> ();
            samples [i] = std::vector<float> ();
        }
    }

    void AudioSample::onSampleRateChange (int newSampleRate) {
        if (_curSampleRate == newSampleRate)
            return;

        _curSampleRate = newSampleRate;
        generateSamples ();
    }


    bool SampleChannel::process (float& audioLeft, float& audioRight) {
        if (_sampleAudio == nullptr || !_isPlaying)
            return false;

        if (_sampleTime >= static_cast<int> (_sampleAudio->samples [0].size ())) {
            _isPlaying = false;
            return false;
        }

        int sampleIdx = _sampleTime++;

        audioLeft = _sampleAudio->samples [0] [sampleIdx];
        audioRight = _sampleAudio->isStereo () ? _sampleAudio->samples [1] [sampleIdx] : audioLeft;

        return true;
    }

    void SampleChannel::onSampleRateChange (int sampleRate) {
        if (sampleRate != _curSampleRate) {
            _sampleTime = std::lround (_sampleTime * (static_cast<double> (sampleRate) / _curSampleRate));
            _curSampleRate = sampleRate;
        }

        if (_sampleAudio != nullptr)
            _sampleAudio->onSampleRateChange (sampleRate);
    }

    void SampleChannel::load (std::shared_ptr<AudioSample> sample) {
        _curSampleRate = APP->engine->getSampleRate ();

        _sampleAudio = sample;
        reset ();
    }

    void SampleChannel::play () {
        if (_sampleAudio == nullptr)
            return;

        _isPlaying = true;
        _sampleTime = 0.f;
    }

    void SampleChannel::play (std::shared_ptr<AudioSample> sample) {
        if (sample == nullptr)
            return;

        _sampleAudio = sample;

        _isPlaying = true;
        _sampleTime = 0.f;
    }

    void SampleChannel::reset () {
        _isPlaying = false;
        _sampleTime = 0.f;
    }

    std::string getErrorMessage (AudioSample::LoadStatus loadStatus, std::string path) {
        switch (loadStatus) {
            case AudioSample::LoadStatus::Success: return "";

            case AudioSample::LoadStatus::FileDoesntExist:
                return fmt::format (FMT_STRING ("Sample file \"{}\" does not exist."), path);

            case AudioSample::LoadStatus::InvalidFile:
                return fmt::format (FMT_STRING ("Sample file \"{}\" could not be loaded."), path);

            case AudioSample::LoadStatus::ChannelCount:
                return fmt::format (FMT_STRING ("Sample file \"{}\" is not stereo or mono."), path);

            default:
                return fmt::format (FMT_STRING ("Unknown error while loading sample file \"{}\"."), path);
        }
    }
}
}