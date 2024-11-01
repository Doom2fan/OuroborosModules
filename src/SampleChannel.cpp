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

#include <AudioFile.h>
#include <fmt/format.h>

namespace OuroborosModules::Audio {
    /*
     * AudioSample
     */
    AudioSample::LoadStatus AudioSample::loadInternal (
        std::string path,
        std::shared_ptr<AudioSample>& audioSample,
        bool isShared,
        int sampleRate
    ) {
        {
            FILE* file = fopen (path.c_str (), "r");
            if (file == nullptr)
                return LoadStatus::FileDoesntExist;
            DEFER ({ fclose (file); });
        }

        AudioFile<float> audioFile;
        audioFile.shouldLogErrorsToConsole (false);

        if (!audioFile.load (path))
            return LoadStatus::InvalidFile;

        if (!audioFile.isMono () && !audioFile.isStereo ())
            return LoadStatus::ChannelCount;

        audioSample = std::make_shared<AudioSample> ();

        auto rawSampleRate = audioFile.getSampleRate ();
        auto rawSampleCount = static_cast<std::size_t> (audioFile.getNumSamplesPerChannel ());
        audioSample->_isStereo = audioFile.isStereo ();
        assert (audioFile.getNumChannels () == audioSample->getChannelCount ());

        std::vector<float> samples;
        if (audioSample->isStereo ()) {
            samples.resize (rawSampleCount * audioSample->getChannelCount ());
            const auto& samplesL = audioFile.samples [0];
            const auto& samplesR = audioFile.samples [1];
            for (std::size_t i = 0; i < rawSampleCount; i++) {
                samples [i * 2    ] = samplesL [i];
                samples [i * 2 + 1] = samplesR [i];
            }
        } else
            samples = audioFile.samples [0];

        assert (samples.size () == rawSampleCount * audioSample->getChannelCount ());

        audioSample->_rawBuffer = AudioBuffer (rawSampleRate, rawSampleCount, samples);

        audioSample->_isShared = isShared;
        audioSample->_rawOnly = sampleRate < 0;
        audioSample->generateSamples (sampleRate);

        return LoadStatus::Success;
    }

    /**/
    AudioSample::LoadStatus AudioSample::load (std::string path, int sampleRate, std::shared_ptr<AudioSample>& audioSample) {
        return loadInternal (path, audioSample, false, sampleRate);
    }

    AudioSample::LoadStatus AudioSample::load (std::string path, std::shared_ptr<AudioSample>& audioSample) {
        return loadInternal (path, audioSample, false, APP->engine->getSampleRate ());
    }

    AudioSample::LoadStatus AudioSample::loadShared (std::string path, int sampleRate, std::shared_ptr<AudioSample>& audioSample) {
        return loadInternal (path, audioSample, true, sampleRate);
    }

    AudioSample::LoadStatus AudioSample::loadSharedRaw (std::string path, std::shared_ptr<AudioSample>& audioSample) {
        return loadInternal (path, audioSample, true, -1);
    }

    void AudioSample::generateSamples (int sampleRate) {
        if (_rawOnly ||
            sampleRate < 1 ||
            _rawBuffer.getSampleCount () < 1 ||
            _rawBuffer.getSampleRate () < 1 ||
            _rawBuffer.getSampleRate () == sampleRate) {
            _noResampledBuffer = true;
            _resampledBuffer = AudioBuffer ();
            return;
        }

        auto channelCount = getChannelCount ();
        // Set up the resampler.
        auto src = src_new (SRC_SINC_FASTEST, channelCount, nullptr);
        assert (src != nullptr);
        DEFER ({ src = src_delete (src); });

        // Set up the resampling info.
        const std::size_t rawBufferSize = 128;
        float tmpBuffer [rawBufferSize];
        auto bufferSize = rawBufferSize / channelCount;
        SRC_DATA srcData;

        srcData.data_in = _rawBuffer.getSamples ().data ();
        srcData.data_out = tmpBuffer;
        srcData.output_frames = bufferSize;
        srcData.src_ratio = static_cast<double> (sampleRate) / _rawBuffer.getSampleRate ();

        std::vector<float> samples;
        auto tmpBufferStart = std::begin (tmpBuffer);
        auto rawSampleCount = _rawBuffer.getSampleCount ();
        std::size_t curTime = 0;
        std::size_t outSampleCount = 0;
        while (curTime < rawSampleCount) {
            srcData.input_frames = std::min (rawSampleCount - curTime, bufferSize);
            srcData.end_of_input = (rawSampleCount - curTime) <= bufferSize;

            if (src_process (src, &srcData))
                break;

            samples.insert (samples.end (), tmpBufferStart, tmpBufferStart + srcData.output_frames_gen * channelCount);

            srcData.data_in += srcData.input_frames_used * channelCount;
            curTime += srcData.input_frames_used;
            outSampleCount += srcData.output_frames_gen;
        }

        _resampledBuffer = AudioBuffer (sampleRate, outSampleCount, samples);
        _noResampledBuffer = false;
    }

    void AudioSample::onSampleRateChange (int newSampleRate) {
        generateSamples (newSampleRate);
    }

    std::shared_ptr<AudioSample> AudioSample::withSampleRate (int newSampleRate, bool shared) {
        auto newSample = std::make_shared<AudioSample> (*this);
        newSample->_isShared = shared;
        newSample->_rawOnly = newSampleRate < 0;
        newSample->generateSamples (newSampleRate);
        return newSample;
    }

    /*
     * SampleChannel
     */
    SampleChannel::SampleChannel () {
    }

    SampleChannel::~SampleChannel () {
        _src = src_delete (_src);
    }

    void SampleChannel::decideBuffer () {
        if (_sampleAudio == nullptr || _isPlaying == true)
            return;

        _isPlayingRaw = _sampleAudio->getResampledBuffer ().getSampleRate () != _curSampleRate;
    }

    const AudioBuffer& SampleChannel::getBuffer () const {
        assert (_sampleAudio != nullptr);
        return _isPlayingRaw ? _sampleAudio->getRawBuffer () : _sampleAudio->getResampledBuffer ();
    }

    bool SampleChannel::process (float& audioLeft, float& audioRight) {
        if (_sampleAudio == nullptr || !_isPlaying) {
            audioLeft = audioRight = 0.f;
            return false;
        }

        assert (_src != nullptr);

        auto& audioBuffer = getBuffer ();
        auto sampleCount = audioBuffer.getSampleCount ();
        if (_sampleTime >= sampleCount && outBuffer.empty ()) {
            audioLeft = audioRight = 0.f;
            reset ();
            return false;
        }

        auto channelCount = _sampleAudio->getChannelCount ();
        for (int i = 3; outBuffer.empty () && i > 0; i--) {
            if (_sampleTime >= sampleCount)
                break;

            // Setup
            auto remainingSamples = sampleCount - _sampleTime;
            auto outBufferSize = outBuffer.capacity () / channelCount;
            auto inputCount = std::min (remainingSamples, outBufferSize);

            if (_curSampleRate == audioBuffer.getSampleRate ()) {
                auto totalSamples = inputCount * channelCount;
                auto inputSamples = audioBuffer.getSamples ().data () + _sampleTime * channelCount;
                std::copy (inputSamples, inputSamples + totalSamples, (float*) outBuffer.endData ());

                // Finalize
                _sampleTime += inputCount;
                outBuffer.endIncr (totalSamples);
            } else {
                SRC_DATA srcData;
                srcData.data_in = audioBuffer.getSamples ().data () + _sampleTime * channelCount;
                srcData.data_out = (float*) outBuffer.endData ();
                srcData.input_frames = inputCount;
                srcData.output_frames = outBufferSize;
                srcData.end_of_input = false;
                srcData.src_ratio = _srcRatio;

                // Process
                if (src_process (_src, &srcData)) {
                    audioLeft = audioRight = 0.f;
                    reset ();
                    return false;
                }

                // Finalize
                _sampleTime += srcData.input_frames_used;
                outBuffer.endIncr (srcData.output_frames_gen * channelCount);
            }
        }

        if (!outBuffer.empty ()) {
            audioLeft = outBuffer.shift ();
            audioRight = _sampleAudio->isStereo () ? outBuffer.shift () : audioLeft;
        } else
            audioLeft = audioRight = 0.f;

        return true;
    }

    void SampleChannel::calcSrcRatio () {
        if (_sampleAudio == nullptr)
            return;

        auto& sampleBuffer = getBuffer ();
        if (sampleBuffer.getSampleRate () == _curSampleRate)
            return;

        _srcRatio = static_cast<double> (_curSampleRate) / sampleBuffer.getSampleRate ();

        if (_src != nullptr)
            src_set_ratio (_src, _srcRatio);
    }

    void SampleChannel::onSampleRateChange (int newSampleRate) {
        _curSampleRate = newSampleRate;
        if (_sampleAudio != nullptr && !_sampleAudio->isShared ()) {
            auto oldAudioSampleRate = _sampleAudio->getResampledBuffer ().getSampleRate ();
            _sampleAudio->onSampleRateChange (newSampleRate);
            if (_isPlaying && !_isPlayingRaw && oldAudioSampleRate != _sampleAudio->getResampledBuffer ().getSampleRate ())
                reset ();
        }
    }

    void SampleChannel::load (std::shared_ptr<AudioSample> sample) {
        if (_sampleAudio == sample) {
            onSampleRateChange (APP->engine->getSampleRate ());
            reset ();
            return;
        }

        if (sample == nullptr) {
            _sampleAudio = nullptr;
            _src = src_delete (_src);
            reset ();
            return;
        }

        if (_src == nullptr || _sampleAudio == nullptr || _sampleAudio->isStereo () != sample->isStereo ()) {
            src_delete (_src);
            _src = src_new (SRC_SINC_FASTEST, sample->getChannelCount (), nullptr);
            assert (_src != nullptr);
        }

        _sampleAudio = sample;
        onSampleRateChange (APP->engine->getSampleRate ());
        reset ();
    }

    void SampleChannel::play () {
        if (_sampleAudio == nullptr)
            return;

        assert (_src != nullptr);

        reset ();
        decideBuffer ();
        calcSrcRatio ();
        _isPlaying = true;
    }

    void SampleChannel::play (std::shared_ptr<AudioSample> sample) {
        load (sample);
        play ();
    }

    void SampleChannel::reset () {
        /*if (_src != nullptr)
            src_reset (_src);*/
        outBuffer.clear ();

        _isPlaying = false;
        _isPlayingRaw = false;
        _sampleTime = 0;
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