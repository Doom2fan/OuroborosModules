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

#include "../Math.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"

#include <vector>
#include <atomic>

namespace OuroborosModules::Modules::Conductor {
    static constexpr int MaxPatterns = 128;
    static constexpr int QueueCleared = -1;

    int patternFloatToInt (float v);

    struct ConductorDataUpdatedEvent {
        int patternCount;

        int currentPattern;
        int queuedPattern;
    };

    struct ConductorExpander;

    struct ConductorCore : ModuleBase {
      private:
        int heartbeatTimer = 0;

      protected:
        void processCore ();

        virtual void prepareDataUpdatedEvent (ConductorDataUpdatedEvent& e) = 0;
        void emitOnDataUpdated ();

      public:
        void expanderConnected (ConductorExpander* expander);

        virtual void requestEnqueue (int newQueue) = 0;
        virtual void requestDequeue () = 0;
    };

    struct ConductorExpander : ModuleBase {
      private:
        std::atomic<int> heartbeatTimer = 0;
        ConductorCore* coreModule = nullptr;

        bool tryFindCore = false;

      protected:
        ConductorCore* getCoreModule () { return coreModule; }

        void processExpander ();

      public:
        virtual ~ConductorExpander () = default;

        void heartbeat (ConductorCore* core);

        virtual void onDataUpdated (const ConductorDataUpdatedEvent& e) = 0;

        void onExpanderChange (const ExpanderChangeEvent& e) override;
    };

    struct PatternIndexQuantity : rack::engine::ParamQuantity {
        float getDisplayValue () override {
            displayMultiplier = getPatternCount () - 1;
            displayOffset = 1;

            return patternFloatToInt (ParamQuantity::getDisplayValue ());
        }

        virtual int getPatternCount () = 0;
    };

    struct LedNumberDisplay : rack_themer::ThemedWidgetBase<rack::widget::Widget> {
        static constexpr int MaxDigits = 8;
        int numDigits;
        float fontSize;
        std::function<int ()> numberFunc;

        bool disabled = false;

        LedNumberDisplay (rack::math::Vec size, float fontSize, int digits, std::function<int ()> numberFunc)
            : numDigits (std::clamp (digits, 1, MaxDigits)), fontSize (fontSize), numberFunc (numberFunc) {
            assert (digits > 0 && digits < MaxDigits);
            box.size = size;

            auto displayBG = rack::createWidget<rack::LedDisplay> (rack::math::Vec ());
            displayBG->box.size = size;
            addChild (displayBG);
        }

        virtual std::shared_ptr<rack::window::Font> getFont () const {
            return APP->window->loadFont (rack::asset::system ("res/fonts/DSEG7ClassicMini-BoldItalic.ttf"));
        }

        void draw (const DrawArgs& args) override {
            _ThemedWidgetBase::draw (args);

            auto font = getFont ();
            nvgFontFaceId (args.vg, font->handle);
            nvgFontSize (args.vg, fontSize);
            nvgTextLetterSpacing (args.vg, 0);

            nvgTextAlign (args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);

            char text [MaxDigits + 1];

            // Background lines
            std::fill (text, text + numDigits, '8');
            nvgFillColor (args.vg, nvgRGBf (.25f, .25f, .25f));
            nvgText (args.vg, VEC_ARGS (box.size / 2.), text, text + numDigits);

            if (disabled)
                return;

            // Text
            fmt::format_to_n (text, MaxDigits, FMT_STRING ("{0:!>{1}d}"), numberFunc (), numDigits);
            nvgFillColor (args.vg, rack::color::WHITE);
            nvgText (args.vg, VEC_ARGS (box.size / 2.), text, text + numDigits);
        }
    };
}