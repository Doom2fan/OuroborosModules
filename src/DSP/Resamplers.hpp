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

#include "../PluginDef.hpp"
#include "Filters.hpp"

#include <algorithm>

namespace OuroborosModules::DSP {
    /*
     * Interfaces
     */
    template<typename T>
    struct Interpolator {
        Interpolator () { }
        virtual ~Interpolator () = default;

        virtual void setParams (int factor) = 0;
        virtual void process (T* outputBuffer, T input) = 0;
    };

    template<typename T>
    struct Decimator {
        Decimator () { }
        virtual ~Decimator () = default;

        virtual void setParams (int factor) = 0;
        virtual T process (const T* inputBuffer) = 0;
    };

    /*
     * Butterworth 6P
     */
    template<typename T>
    struct Butterworth6PInterpolator : Interpolator<T> {
      private:
        int oversampleFactor = 1;
        Butterworth6P<T> filter = Butterworth6P<T> ();

      public:
        void setParams (int factor) override {
            oversampleFactor = factor;
            filter.setCutoffFreq (1.f / (factor * 4));
        }

        void process (T* outputBuffer, T input) override {
            outputBuffer [0] = filter.process (input * oversampleFactor);

            auto zero = T (0);
            for (int i = 1; i < oversampleFactor; ++i)
                outputBuffer [i] = filter.process (zero);
        }
    };

    template<typename T>
    struct Butterworth6PDecimator : Decimator<T> {
      private:
        int oversampleFactor = 1;
        Butterworth6P<T> filter = Butterworth6P<T> ();

      public:
        void setParams (int factor) override {
            oversampleFactor = factor;
            filter.setCutoffFreq (1.f / (factor * 4));
        }

        T process (const T* inputBuffer) override {
            for (int i = 0; i < oversampleFactor - 1; ++i)
                filter.process (inputBuffer [i]);
            return filter.process (inputBuffer [oversampleFactor - 1]);
        }
    };

    struct HalfBandInfo {
        static constexpr int FilterLength = 47;
        static constexpr int BufferSize = 64;  // Power of 2 for fast modulo
        static constexpr int CenterTap = 23;

        // Only non-zero coefficients are stored
        // For a 31-tap half-band filter, we have 16 non-zero coefficients
        // (15 symmetric pairs + 1 center tap)
        static constexpr float Coefficients [] = {
            -0.0001977007639963f,
            0.0005764333789606f,
            -0.0013514572110066f,
            0.0027283620871093f,
            -0.0049870509231914f,
            0.0084982426864263f,
            -0.0137870949237456f,
            0.0217118427107294f,
            -0.0339787322258089f,
            0.0549439077729472f,
            -0.1006567014198994f,
            0.3164571530206271f,
            0.5000097416049799f,
        };

        // Tap positions (stride of 2)
        static constexpr int TapOffsets [] = {
            0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 23
        };
    };

    template<typename T, typename TInfo = HalfBandInfo>
    struct HalfBandFilter {
      private:
        static constexpr int FilterLength = TInfo::FilterLength;
        static constexpr int CoefficientsCount = (sizeof (TInfo::Coefficients) / sizeof (TInfo::Coefficients [0]));
        static constexpr int BufferSize = TInfo::BufferSize;
        static constexpr int BufferMask = BufferSize - 1;
        static constexpr int CenterTap = TInfo::CenterTap;

      private:
        alignas (16) T delayLine [BufferSize];
        int writeIndex;

        void feedDelay (T input) {
            delayLine [writeIndex] = input;
            writeIndex = (writeIndex + 1) & BufferMask;
        }

        /**
         * Compute polyphase
         */
        T computeEvenPhase () {
            int readIndex = writeIndex;
            auto sum = T (0.f);

            // Symmetric pairs
            for (int i = 0; i < CoefficientsCount - 1; i++) {
                int left = (readIndex - TInfo::TapOffsets [i]) & BufferMask;
                int right = (readIndex - (FilterLength - 1 - TInfo::TapOffsets [i])) & BufferMask;
                sum += T (TInfo::Coefficients [i]) * (delayLine [left] + delayLine [right]);
            }

            return sum;
        }

        T computeOddPhase () {
            int readIndex = writeIndex;

            // Center tap
            return T (TInfo::Coefficients [CoefficientsCount - 1]) * delayLine [(readIndex - 1 - CenterTap) & BufferMask];
        }

      public:
        HalfBandFilter () {
            reset ();
        }

        /**
         * 2x Upsampling: Takes one input sample, produces two output samples
         * First output aligns with input timing, second is interpolated
         */
        void upsample (T input, T& output1, T& output2) {
            // Compute first output (aligns with input)
            feedDelay (input * 2);
            output1 = computeEvenPhase ();

            // Compute second output (interpolated sample)
            feedDelay (input * 2);
            output2 = computeOddPhase ();
        }

        /**
         * Batch upsampling
         */
        void upsampleBlock (const T* input, T* output, size_t inputLength) {
            for (size_t i = 0; i < inputLength; i++)
                upsample (input, output [i * 2], output [i * 2 + 1]);
        }

        /**
         * Batch downsampling
         */
        void downsampleBlock (const T* input, T* output, size_t inputLength) {
            for (size_t i = 0, outputIdx = 0; i < inputLength; i += 2, outputIdx++) {
                feedDelay (input [i]);
                T result = computeEvenPhase ();
                feedDelay (input [i + 1]);
                result += computeOddPhase ();

                output [outputIdx] = result;
            }
        }

        void reset () {
            memset (delayLine, 0, sizeof (delayLine));
            writeIndex = 0;
        }

        // Filter specifications
        static constexpr int getFilterLength () { return FilterLength; }
        static constexpr int getLatencySamples () { return (FilterLength - 1) / 2; }
    };

    template<typename T, template<typename> typename U>
    struct FilterCascade {
      public:
        static constexpr int MaxOversampleRate = 32;

      private:
        U<T>* filters = nullptr;
        int oversampleFactor = 0;
        int numStages = 0;

      public:
        FilterCascade () {
            setParams (1);
        }

        virtual ~FilterCascade () {
            if (filters != nullptr)
                delete [] filters;
        }

        void setParams (int factor) {
            assert (rack::math::isPow2 (factor));
            assert (factor <= MaxOversampleRate);
            if (!rack::math::isPow2 (factor))
                factor = 1;

            // Count the stages.
            numStages = 0;
            for (int i = factor; i > 1; i >>= 1, numStages++) ;

            // Allocate the filters.
            if (filters != nullptr) {
                delete [] filters;
                filters = nullptr;
            }

            if (numStages > 0)
                filters = new U<T> [numStages];

            oversampleFactor = factor;
        }

        void reset () {
            if (filters == nullptr)
                return;

            for (int i = 0; i < numStages; i++)
                filters [i].reset ();
        }

        void upsample (T input, T* outputBuffer) {
            if (numStages == 0) {
                outputBuffer [0] = input;
                return;
            }

            T bufferA [MaxOversampleRate];
            T bufferB [MaxOversampleRate];

            T* frontBuffer = bufferA;
            T* backBuffer = bufferB;

            // First stage.
            filters [0].upsample (input, backBuffer [0], backBuffer [1]);

            // Rest of the stages.
            for (int stage = 1; stage < numStages; stage++) {
                int inputSize = 1 << stage;

                for (int sample = 0; sample < inputSize; sample++) {
                    filters [stage].upsample (
                        backBuffer [sample],
                        frontBuffer [sample * 2],
                        frontBuffer [sample * 2 + 1]
                    );
                }

                std::swap (frontBuffer, backBuffer);
            }

            std::copy (backBuffer, backBuffer + oversampleFactor, outputBuffer);
        }

        T downsample (const T* inputBuffer) {
            if (numStages == 0)
                return inputBuffer [0];

            T bufferA [MaxOversampleRate];
            T bufferB [MaxOversampleRate];

            T* frontBuffer = bufferA;
            T* backBuffer = bufferB;

            std::copy (inputBuffer, inputBuffer + oversampleFactor, backBuffer);

            // N - 1 stages.
            for (int stage = numStages - 1; stage > 0; stage--) {
                filters [stage].downsampleBlock (backBuffer, frontBuffer, 1 << (stage + 1));
                std::swap (frontBuffer, backBuffer);
            }

            // Last stage.
            filters [0].downsampleBlock (backBuffer, frontBuffer, 2);

            return frontBuffer [0];
        }
    };

    template<typename T>
    struct HalfBandInterpolator : Interpolator<T> {
      private:
        FilterCascade<T, HalfBandFilter> cascade;

      public:
        void setParams (int factor) override {
            cascade.setParams (factor);
        }

        void process (T* outputBuffer, T input) override {
            cascade.upsample (input, outputBuffer);
        }
    };

    template<typename T>
    struct HalfBandDecimator : Decimator<T> {
      private:
        FilterCascade<T, HalfBandFilter> cascade;

      public:
        void setParams (int factor) override {
            cascade.setParams (factor);
        }

        T process (const T* inputBuffer) override {
            return cascade.downsample (inputBuffer);
        }
    };

    template<typename T, typename TFilters>
    struct OptimizedHalfBandCascade {
      private:
        static constexpr int MaxOversample = 16;
        static constexpr int MaxStages = 4;

        std::unique_ptr<TFilters> filters = nullptr;
        int oversampleFactor = 0;
        int numStages = 0;

      public:
        OptimizedHalfBandCascade () {
            filters = std::make_unique<TFilters> ();
            setParams (1);
        }

        void setParams (int factor) {
            assert (rack::math::isPow2 (factor));
            assert (factor < MaxOversample);

            if (!rack::math::isPow2 (factor))
                factor = 1;
            if (factor > MaxOversample)
                factor = MaxOversample;

            // Count the stages.
            numStages = 0;
            for (int i = factor; i > 1; i >>= 1)
                numStages++;

            // Reset the filters.
            reset ();

            oversampleFactor = factor;
        }

        void reset () {
            filters->filter2.reset ();
            filters->filter4.reset ();
            filters->filter8.reset ();
            filters->filter16.reset ();
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        void upsample (T input, T* outputBuffer) {
            if (numStages == 0) {
                outputBuffer [0] = input;
                return;
            }

            T bufferA [MaxOversample];
            T bufferB [MaxOversample];

            T* frontBuffer = bufferA;
            T* backBuffer = bufferB;

            // First stage.
            filters->filter2.upsample (input, backBuffer [0], backBuffer [1]);

            // Rest of the stages.
            #define RUN_STAGE(stage, oversampleCount) \
                if (oversampleFactor >= oversampleCount) { \
                    for (int sample = 0; sample < (1 << (stage - 1)); sample++) { \
                        filters->filter##oversampleCount.upsample ( \
                            backBuffer [sample], \
                            frontBuffer [sample * 2], \
                            frontBuffer [sample * 2 + 1]  \
                        ); \
                    } \
                    std::swap (frontBuffer, backBuffer); \
                }

            RUN_STAGE (2, 4)
            RUN_STAGE (3, 8)
            RUN_STAGE (4, 16)

            #undef RUN_STAGE

            std::copy (backBuffer, backBuffer + oversampleFactor, outputBuffer);
        }

        T downsample (const T* inputBuffer) {
            if (numStages == 0)
                return inputBuffer [0];

            T bufferA [MaxOversample];
            T bufferB [MaxOversample];

            T* frontBuffer = bufferA;
            T* backBuffer = bufferB;

            std::copy (inputBuffer, inputBuffer + oversampleFactor, backBuffer);

            // N - 1 stages.
            #define RUN_STAGE(stage, oversampleCount) \
                if (oversampleFactor >= oversampleCount) { \
                    filters->filter##oversampleCount.downsampleBlock (backBuffer, frontBuffer, 1 << stage); \
                    std::swap (frontBuffer, backBuffer); \
                }

            RUN_STAGE (4, 16)
            RUN_STAGE (3, 8)
            RUN_STAGE (2, 4)

            #undef RUN_STAGE

            // Last stage.
            filters->filter2.downsampleBlock (backBuffer, frontBuffer, 2);

            return frontBuffer [0];
        }
#pragma GCC diagnostic pop
    };

    template<typename T>
    struct OptimizedHalfBandInterpolator : Interpolator<T> {
      private:
        struct HalfBandInfo2 {
            // Parameters: length: 47, transition band: 0.192
            static constexpr int FilterLength = 47;
            static constexpr int BufferSize = 64;
            static constexpr int CenterTap = 23;

            static constexpr float Coefficients [] = {
                -0.00000282545734567922179802305741869172805991183849982917308807f,
                 0.00002447455626053901068042706334892955055693164467811584472656f,
                -0.00011968907192440857991614505673183543876803014427423477172852f,
                 0.00042832754120345297756122793231270406977273523807525634765625f,
                -0.00124149751591897381831908209193215952836908400058746337890625f,
                 0.00307864335390409336185024713472557778004556894302368164062500f,
                -0.00677181029595662863085347993319373927079141139984130859375000f,
                 0.01360080620689151434388186601154302479699254035949707031250000f,
                -0.02570070964435900578992644227582786697894334793090820312500000f,
                 0.04771350251766769134942336449967115186154842376708984375000000f,
                -0.09570181493568089003964871608332032337784767150878906250000000f,
                 0.31469257644961479147838190328911878168582916259765625000000000f,
                 0.50000000000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 23,
            };
        };

        struct HalfBandInfo4 {
            // Parameters: length: 27, transition band: 0.26
            static constexpr int FilterLength = 27;
            static constexpr int BufferSize = 32;
            static constexpr int CenterTap = 13;

            static constexpr float Coefficients [] = {
                 0.00005076010672315541395575866356892902331310324370861053466797f,
                -0.00053000579917758235213437467692187965440098196268081665039062f,
                 0.00284618327575729456249686855073832703055813908576965332031250f,
                -0.01051937491384237997882333814914090908132493495941162109375000f,
                 0.03077212934473826447256961102993955137208104133605957031250000f,
                -0.08202681067701000916514431082759983837604522705078125000000000f,
                 0.30940713350449533525221568197594024240970611572265625000000000f,
                 0.50000000000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 4, 6, 8, 10, 12, 13,
            };
        };

        struct HalfBandInfo8 {
            // Parameters: length: 15, transition band: 0.4
            static constexpr int FilterLength = 15;
            static constexpr int BufferSize = 16;
            static constexpr int CenterTap = 7;

            static constexpr float Coefficients [] = {
                -0.00144623175067867728443848918118419533129781484603881835937500f,
                 0.01303640223554676860762135959248553263023495674133300781250000f,
                -0.06168586492519635700038094228148111142218112945556640625000000f,
                 0.30009564798069393587454101179901044815778732299804687500000000f,
                 0.50000000000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 4, 6, 7,
            };
        };

        struct HalfBandInfo16 {
            // Parameters: length: 15, transition band: 0.45
            static constexpr int FilterLength = 15;
            static constexpr int BufferSize = 16;
            static constexpr int CenterTap = 7;

            static constexpr float Coefficients [] = {
                -0.00127003550541604439023946060416392356273718178272247314453125f,
                 0.01220687787258861387029718059693550458177924156188964843750000f,
                -0.06025046578819941156535122672721627168357372283935546875000000f,
                 0.29931362330658395354276990474318154156208038330078125000000000f,
                 0.50000000000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 4, 6, 7,
            };
        };

        struct Filters {
            HalfBandFilter<T, HalfBandInfo2> filter2;
            HalfBandFilter<T, HalfBandInfo4> filter4;
            HalfBandFilter<T, HalfBandInfo8> filter8;
            HalfBandFilter<T, HalfBandInfo16> filter16;
        };

        OptimizedHalfBandCascade<T, Filters> cascade;

      public:
        void setParams (int factor) override {
            cascade.setParams (factor);
        }

        void process (T* outputBuffer, T input) override {
            cascade.upsample (input, outputBuffer);
        }
    };

    template<typename T>
    struct OptimizedHalfBandDecimator : Decimator<T> {
      private:
        struct HalfBandInfo2 {
            // Parameters: length: 47, transition band: .1
            static constexpr int FilterLength = 47;
            static constexpr int BufferSize = 64;
            static constexpr int CenterTap = 23;

            static constexpr float Coefficients [] = {
                -0.00019789114578275086788090864065026153184589929878711700439453f,
                 0.00057672564464539841436185874101738590979948639869689941406250f,
                -0.00135211229824330671916987611780314182396978139877319335937500f,
                 0.00272918915712982252871898758428415021626278758049011230468750f,
                -0.00498828498272814880448722263395211484748870134353637695312500f,
                 0.00849965141109245911343883506106067216023802757263183593750000f,
                -0.01378875776791373478080693359970609890297055244445800781250000f,
                 0.02171344059871797849137209368564072065055370330810546875000000f,
                -0.03398025346507325961109557965755811892449855804443359375000000f,
                 0.05494499599260666000688502208504360169172286987304687500000000f,
                -0.10065745033826115073516405118425609543919563293457031250000000f,
                 0.31645738803103795611093573825201019644737243652343750000000000f,
                 0.50000000000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 23,
            };
        };

        struct HalfBandInfo4 {
            // Parameters: length: 19, transition band: .245
            static constexpr int FilterLength = 19;
            static constexpr int BufferSize = 32;
            static constexpr int CenterTap = 9;

            static constexpr float Coefficients [] = {
                 0.00119046446664385139994901674498350985231809318065643310546875f,
                -0.00734950015842671371818495273942062340211123228073120117187500f,
                 0.02661805495664077217399245967044407734647393226623535156250000f,
                -0.07838759871027872749849052524950820952653884887695312500000000f,
                 0.30795139545441257977742566254164557904005050659179687500000000f,
                 0.50000000000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 4, 6, 8, 9,
            };
        };

        struct HalfBandInfo8 {
            // Parameters: length: 11, transition band: .365
            static constexpr int FilterLength = 11;
            static constexpr int BufferSize = 16;
            static constexpr int CenterTap = 5;

            static constexpr float Coefficients [] = {
                 0.0073550788202958357342442496928924811072647571563720703125f,
                -0.0529124755881889979880483565466420259326696395874023437500f,
                 0.2955727989648236908593048610782716423273086547851562500000f,
                 0.5000000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 4, 5,
            };
        };

        struct HalfBandInfo16 {
            // Parameters: length: 7, transition band: .420
            static constexpr int FilterLength = 7;
            static constexpr int BufferSize = 8;
            static constexpr int CenterTap = 3;

            static constexpr float Coefficients [] = {
                -0.0327725769570514080530898581855581142008304595947265625f,
                 0.2827272518400801848414971573220100253820419311523437500f,
                 0.5000000000000000000000000000000000000000000000000000000f,
            };

            static constexpr int TapOffsets [] = {
                0, 2, 3,
            };
        };

        struct Filters {
            HalfBandFilter<T, HalfBandInfo2> filter2;
            HalfBandFilter<T, HalfBandInfo4> filter4;
            HalfBandFilter<T, HalfBandInfo8> filter8;
            HalfBandFilter<T, HalfBandInfo16> filter16;
        };

        OptimizedHalfBandCascade<T, Filters> cascade;

      public:
        void setParams (int factor) override {
            cascade.setParams (factor);
        }

        T process (const T* inputBuffer) override {
            return cascade.downsample (inputBuffer);
        }
    };
}