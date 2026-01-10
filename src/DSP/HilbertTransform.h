/*
 * CODE TAKEN FROM sst-basic-blocks. (https://github.com/surge-synthesizer/sst-basic-blocks)
 * The only modifications made to the code were reformatting and namespacing.
 * As such, this file is licensed under the same licenses as the file
 * "sst-basic-blocks/include/sst/basic-blocks/dsp/HilbertTransform.h" in the sst-basic-blocks repository.
 */

/*
 * sst-basic-blocks - an open source library of core audio utilities
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful on the audio thread for blocks,
 * modulation, etc... or useful for adapting code to multiple environments.
 *
 * Copyright 2023, various authors, as described in the GitHub
 * transaction log. Parts of this code are derived from similar
 * functions original in Surge or ShortCircuit.
 *
 * sst-basic-blocks is released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html.
 *
 * A very small number of explicitly chosen header files can also be
 * used in an MIT/BSD context. Please see the README.md file in this
 * repo or the comments in the individual files. Only headers with an
 * explicit mention that they are dual licensed may be copied and reused
 * outside the GPL3 terms.
 *
 * All source in sst-basic-blocks available at
 * https://github.com/surge-synthesizer/sst-basic-blocks
 */

#pragma once

#include "../PluginDef.hpp"

#include <utility>
#include <complex>

namespace OuroborosModules::DSP {
    /*
     * This file is available for use under the MIT license or
     * the GPL3 license, as described in README.
     *
     * Thanks to Sean Costello for the conversation which led
     * to BaconPaul understanding the hilbert transform in the
     * context of pitch shifters, and from sharing this
     * serial-biquad implementation with allpass coefficients.
     * The coefficients first appear in one of the
     * Electronotes series papers by Bernie Hutchins.
     *
     * The hilbert transform takes a real valued signal
     * and returns a complex valued signal which allows you
     * to create the "analytic signal" - namely the signal with
     * only positive frequencies. That is, take a real signal
     * u(t) = cos(wt) and imagine it as a complex signal
     * U(t) = 0.5 (e^iwt + e^-iwt).  The modified signal
     * v(t) = H(u(t)) contains only the positive
     * frequency. Basically at this point go read wikipedia
     * and stuff, but that gives you loads of cool properties
     * for modifying phase and pitch in the complex plane.
     *
     * The actual calculation of the hilbert transform numerically
     * is tricky, which is why we are particularly grateful to
     * Sean for sharing this implementation (which he shared
     * in float form, and which is presented here both in
     * mono-float form and in stereo-SSE form).
     */
    struct HilbertTransform {
        struct BQ {
            float a1 {1}, a2 {0}, b0 {1}, b1 {0}, b2 {0}, reg0 {0}, reg1 {0};

            inline void reset () {
                reg0 = 0;
                reg1 = 0;
            }

            inline void setCoefs (float _a1, float _a2, float _b0, float _b1, float _b2) {
                a1 = _a1;
                a2 = _a2;
                b0 = _b0;
                b1 = _b1;
                b2 = _b2;
            }

            inline float step (float input) {
                double op;

                op = input * b0 + reg0;
                reg0 = input * b1 - a1 * op + reg1;
                reg1 = input * b2 - a2 * op;

                return (float) op;
            }
        } allpass [2] [3];

        float sampleRate {0};
        float hilbertCoefs [12];

        void setSampleRate (float sr) {
            sampleRate = sr;
            setHilbertCoefs ();
        }

        void setHilbertCoefs () {
            assert (sampleRate);
            float a1, a2, b0, b1, b2;

            // phase difference network normalized pole frequencies
            // first six numbers are for real, the rest for imaginary
            const float poles [12] = {  .3609f,  2.7412f, 11.1573f, 44.7581f, 179.6242f,  798.4578f,
                                       1.2524f,  5.5671f, 22.3423f, 89.6271f, 364.7914f, 2770.1114f };

            float minFreq = 25.f; // minimum frequency of phase differencing network, in Hertz

            // bilinear z-transform for calculating coefficients for 1st order allpasses
            for (int j = 0; j < 12; j++)
                hilbertCoefs [j] = (1.f - (minFreq * M_PI * poles [j]) / sampleRate) /
                                   (1.f + (minFreq * M_PI * poles [j]) / sampleRate);

            // calculate biquad coeffcients for 2nd order allpasses, using the first order allpasses
            // in the following order: highest (in a branch) by lowest, next highest by next lowest,
            // next highest by next lowest.

            for (int j = 0; j < 3; j++) {
                allpass [0] [j].reset (); // clears states
                a1 = -(hilbertCoefs [j] + hilbertCoefs [5 - j]);
                a2 =   hilbertCoefs [j] * hilbertCoefs [5 - j];
                b0 =   hilbertCoefs [j] * hilbertCoefs [5 - j];
                b1 = -(hilbertCoefs [j] + hilbertCoefs [5 - j]);
                b2 = 1.f;
                allpass [0] [j].setCoefs (a1, a2, b0, b1, b2);
            }

            for (int j = 0; j < 3; j++)
            {
                allpass [1] [j].reset (); // clears states
                a1 = -(hilbertCoefs[j + 6] + hilbertCoefs [11 - j]);
                a2 =   hilbertCoefs[j + 6] * hilbertCoefs [11 - j];
                b0 =   hilbertCoefs[j + 6] * hilbertCoefs [11 - j];
                b1 = -(hilbertCoefs[j + 6] + hilbertCoefs [11 - j]);
                b2 = 1.f;
                allpass [1] [j].setCoefs (a1, a2, b0, b1, b2);
            }
        }

        std::pair<float, float> stepPair (float in) {
            float im {in}, re {in};

            for (int i = 0; i < 3; ++i) {
                re = allpass [0] [i].step (re);
                im = allpass [1] [i].step (im);
            }
            return {re, im};
        }

        std::complex<float> stepComplex (float in) {
            auto [r, i] = stepPair (in);
            return {r, i};
        }
    };
}