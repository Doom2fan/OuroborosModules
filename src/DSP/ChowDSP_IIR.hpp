/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
 *  Copyright (C) ChowDSP
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

namespace OuroborosModules::DSP {
    /** Digital IIR filter processor. Using TDF-II structure:
     * https://ccrma.stanford.edu/~jos/filters/Transposed_Direct_Forms.html
    */
    template<int ORDER, typename T = float>
    struct IIRFilter {
        /** transfer function numerator coefficients: b_0, b_1, etc.*/
        T b [ORDER] = { };

        /** transfer function denominator coefficients: a_0, a_1, etc.*/
        T a [ORDER] = { };

        /** filter state */
        T z [ORDER];

        IIRFilter () {
            reset ();
        }

        void reset () {
            std::fill (z, &z [ORDER], 0.0f);
        }

        void setCoefficients (const T* b, const T* a) {
            for (int i = 0; i < ORDER; i++)
                this->b [i] = b [i];
            for (int i = 1; i < ORDER; i++)
                this->a [i] = a [i];
        }

        template<int N = ORDER>
        inline typename std::enable_if<N == 2, T>::type process (T x) noexcept {
            T y = z [1] + x * b [0];
            z [1] = x * b [1] - y * a [1];
            return y;
        }

        template<int N = ORDER>
        inline typename std::enable_if<N == 3, T>::type process (T x) noexcept {
            T y = z [1] + x * b [0];
            z [1] = z [2] + x * b [1] - y * a [1];
            z [2] = x * b [2] - y * a [2];
            return y;
        }

        template<int N = ORDER>
        inline typename std::enable_if<(N > 3), T>::type process (T x) noexcept {
            T y = z [1] + x * b [0];

            for (int i = 1; i < ORDER - 1; ++i)
                z [i] = z [i + 1] + x * b [i] - y * a [i];

            z [ORDER - 1] = x * b [ORDER - 1] - y * a [ORDER - 1];

            return y;
        }

        /** Computes the complex transfer function $H(s)$ at a particular frequency
        s: normalized angular frequency equal to $2 \pi f / f_{sr}$ ($\pi$ is the Nyquist frequency)
        */
        std::complex<T> getTransferFunction (T s) {
            // Compute sum(a_k z^-k) / sum(b_k z^-k) where z = e^(i s)
            std::complex<T> bSum (b [0], 0);
            std::complex<T> aSum (1, 0);
            for (int i = 1; i < ORDER; i++) {
                T p = -i * s;
                std::complex<T> z (rack::simd::cos (p), rack::simd::sin (p));
                bSum += b [i] * z;
                aSum += a [i - 1] * z;
            }

            return bSum / aSum;
        }

        T getFrequencyResponse (T f) {
            return rack::simd::abs (getTransferFunction (2 * M_PI * f));
        }

        T getFrequencyPhase (T f) {
            return rack::simd::arg (getTransferFunction (2 * M_PI * f));
        }
    };

    template<typename T = float>
    struct TBiquadFilter : IIRFilter<3, T> {
        enum Type {
            LOWPASS,
            HIGHPASS,
            LOWSHELF,
            HIGHSHELF,
            BANDPASS,
            PEAK,
            NOTCH,
            NUM_TYPES
        };

        TBiquadFilter () {
            setParameters (LOWPASS, 0.f, 0.f, 1.f);
        }

        /** Calculates and sets the biquad transfer function coefficients.
        f: normalized frequency (cutoff frequency / sample rate), must be less than 0.5
        Q: quality factor
        V: gain
        */
        void setParameters (Type type, float f, float Q, float V) {
            float K = std::tan (M_PI * f);

            switch (type) {
                case LOWPASS: {
                    float norm = 1.f / (1.f + K / Q + K * K);
                    this->b [0] = K * K * norm;
                    this->b [1] = 2.f * this->b [0];
                    this->b [2] = this->b [0];
                    this->a [1] = 2.f * (K * K - 1.f) * norm;
                    this->a [2] = (1.f - K / Q + K * K) * norm;
                } break;

                case HIGHPASS: {
                    float norm = 1.f / (1.f + K / Q + K * K);
                    this->b [0] = norm;
                    this->b [1] = -2.f * this->b [0];
                    this->b [2] = this->b [0];
                    this->a [1] = 2.f * (K * K - 1.f) * norm;
                    this->a [2] = (1.f - K / Q + K * K) * norm;
                } break;

                case LOWSHELF: {
                    float sqrtV = std::sqrt (V);

                    if (V >= 1.f) {
                        float norm = 1.f / (1.f + M_SQRT2 * K + K * K);
                        this->b [0] = (1.f + M_SQRT2 * sqrtV * K + V * K * K) * norm;
                        this->b [1] = 2.f * (V * K * K - 1.f) * norm;
                        this->b [2] = (1.f - M_SQRT2 * sqrtV * K + V * K * K) * norm;
                        this->a [1] = 2.f * (K * K - 1.f) * norm;
                        this->a [2] = (1.f - M_SQRT2 * K + K * K) * norm;
                    } else {
                        float norm = 1.f / (1.f + M_SQRT2 / sqrtV * K + K * K / V);
                        this->b [0] = (1.f + M_SQRT2 * K + K * K) * norm;
                        this->b [1] = 2.f * (K * K - 1) * norm;
                        this->b [2] = (1.f - M_SQRT2 * K + K * K) * norm;
                        this->a [1] = 2.f * (K * K / V - 1.f) * norm;
                        this->a [2] = (1.f - M_SQRT2 / sqrtV * K + K * K / V) * norm;
                    }
                } break;

                case HIGHSHELF: {
                    float sqrtV = std::sqrt (V);

                    if (V >= 1.f) {
                        float norm = 1.f / (1.f + M_SQRT2 * K + K * K);
                        this->b [0] = (V + M_SQRT2 * sqrtV * K + K * K) * norm;
                        this->b [1] = 2.f * (K * K - V) * norm;
                        this->b [2] = (V - M_SQRT2 * sqrtV * K + K * K) * norm;
                        this->a [1] = 2.f * (K * K - 1.f) * norm;
                        this->a [2] = (1.f - M_SQRT2 * K + K * K) * norm;
                    } else {
                        float norm = 1.f / (1.f / V + M_SQRT2 / sqrtV * K + K * K);
                        this->b [0] = (1.f + M_SQRT2 * K + K * K) * norm;
                        this->b [1] = 2.f * (K * K - 1.f) * norm;
                        this->b [2] = (1.f - M_SQRT2 * K + K * K) * norm;
                        this->a [1] = 2.f * (K * K - 1.f / V) * norm;
                        this->a [2] = (1.f / V - M_SQRT2 / sqrtV * K + K * K) * norm;
                    }
                } break;

                case BANDPASS: {
                    float norm = 1.f / (1.f + K / Q + K * K);
                    this->b [0] = K / Q * norm;
                    this->b [1] = 0.f;
                    this->b [2] = -this->b [0];
                    this->a [1] = 2.f * (K * K - 1.f) * norm;
                    this->a [2] = (1.f - K / Q + K * K) * norm;
                } break;

                case PEAK: {
                    float c = 1.f / K;
                    float phi = c * c;
                    float Knum = c / Q;
                    float Kdenom = Knum;

                    if (V > 1.f)
                        Knum *= V;
                    else
                        Kdenom /= V;

                    float norm = phi + Kdenom + 1.f;
                    this->b [0] = (phi + Knum +  1.f) / norm;
                    this->b [1] = 2.f  * (1.f  - phi) / norm;
                    this->b [2] = (phi - Knum +  1.f) / norm;
                    this->a [1] = 2.f  * (1.f  - phi) / norm;
                    this->a [2] = (phi - Kdenom + 1.f) / norm;
                } break;

                case NOTCH: {
                    float norm = 1.f / (1.f + K / Q + K * K);
                    this->b [0] = (1.f + K * K) * norm;
                    this->b [1] = 2.f * (K * K - 1.f) * norm;
                    this->b [2] = this->b [0];
                    this->a [1] = this->b [1];
                    this->a [2] = (1.f - K / Q + K * K) * norm;
                } break;

                default: break;
            }
        }
    };
}