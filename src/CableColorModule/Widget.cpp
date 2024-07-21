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

#include "CableColorModule.hpp"

#include "../UI/CommonWidgets.hpp"
#include "../UI/MenuItems/CommonItems.hpp"
#include "../UI/MenuItems/TextField.hpp"
#include "CableColorManager.hpp"
#include "CCM_Common.hpp"
#include "ColorDisplayWidget.hpp"

#include <unordered_set>

namespace OuroborosModules {
namespace CableColorModule {
    CableColorModuleWidget::CableColorModuleWidget (CableColorModule* module) { constructor (module, "panels/CableColorModule"); }

    void CableColorModuleWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createWidget;
        using rack::createWidgetCentered;
        using rack::math::Vec;
        using rack::window::mm2px;
        using Widgets::ImageWidget;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = createWidget<ImageWidget> (Vec ());
        addChild (emblemWidget);
        updateEmblem (curTheme, curEmblem);

        colorDisplayWidget = new ColorDisplayWidget (module, Vec (mm2px (47.88), mm2px (112.448)));
        colorDisplayWidget->box.pos = findNamed ("widgetColorDisplay")
            .value_or (Vec ())
            .minus (colorDisplayWidget->box.size.div (2));
        addChild (colorDisplayWidget);

        // Skip if we're in the module browser.
        if (module == nullptr)
            return;

        update ();
    }

    CableColorModuleWidget::~CableColorModuleWidget () {
        if (masterModule == module)
            masterModule = nullptr;

        if (keyContainer != nullptr) {
            keyContainer->moduleWidget = nullptr;
            keyContainer->requestDelete ();
            keyContainer = nullptr;
        }
    }

    void CableColorModuleWidget::update () {
        auto isMaster = module->checkMaster ();

        if (isMaster && keyContainer == nullptr) {
            keyContainer = new KeyContainer (this);
            APP->scene->addChild (keyContainer);
        } else if (!isMaster && keyContainer != nullptr) {
            keyContainer->requestDelete ();
            keyContainer = nullptr;
        }

        if (!isMaster)
            return;

        module->colorManager->updateCurrentColor ();
    }

    void CableColorModuleWidget::step () {
        _WidgetBase::step ();

        // Skip if we're in the module browser.
        if (module == nullptr)
            return;

        update ();

        if (module->updateEmblem) {
            updateEmblem (curTheme, curEmblem);
            module->updateEmblem = false;
        }
    }

    void CableColorModuleWidget::updateEmblem (ThemeKind theme, EmblemKind emblem) {
        if (emblemWidget == nullptr)
            return;

        if (emblem == EmblemKind::None) {
            emblemWidget->hide ();
            return;
        } else
            emblemWidget->show ();

        emblemWidget->setSvg (Theme::getEmblem (emblem, theme));

        auto centerEmblem = pluginSettings.cableColor_CenterEmblem;
        if (module != nullptr && module->centerEmblem != CenterEmblem::Default)
            centerEmblem = module->centerEmblem == CenterEmblem::True;

        auto emblemPos = centerEmblem ? box.size.div (2) : findNamed ("widgetLogo").value_or (rack::math::Vec ());
        auto emblemSize = rack::window::mm2px (centerEmblem ? 45.296f : Constants::StdEmblemSize);

        emblemWidget->setZoom (emblemSize);
        emblemWidget->setSize (rack::math::Vec (emblemSize));
        emblemWidget->box.pos = emblemPos.minus (emblemWidget->box.size.div (2));
    }

    void CableColorModuleWidget::onChangeTheme (ThemeKind kind) {
        _WidgetBase::onChangeTheme (kind);
        updateEmblem (kind, curEmblem);
    }

    void CableColorModuleWidget::onChangeEmblem (EmblemKind kind) {
        _WidgetBase::onChangeEmblem (kind);
        updateEmblem (curTheme, kind);
    }

    void CableColorModuleWidget::createLocalStyleMenu (rack::ui::Menu* menu) {
        auto createEmblemLocationItem = [&] (std::string name, CenterEmblem centerEmblem) {
            struct HistoryEmblemLocation : rack::history::ModuleAction {
                CenterEmblem oldLocation;
                CenterEmblem newLocation;

                HistoryEmblemLocation () { name = "change emblem location"; }

                void undo () override {
                    auto module = dynamic_cast<CableColorModule*> (APP->engine->getModule (moduleId));
                    if (module == nullptr)
                        return;

                    module->centerEmblem = oldLocation;
                    module->updateEmblem = true;
                }

                void redo () override {
                    auto module = dynamic_cast<CableColorModule*> (APP->engine->getModule (moduleId));
                    if (module == nullptr)
                        return;

                    module->centerEmblem = newLocation;
                    module->updateEmblem = true;
                }
            };

            return rack::createCheckMenuItem (name, "",
                [=] { return module->centerEmblem == centerEmblem; },
                [=] {
                    auto history = new HistoryEmblemLocation;

                    history->moduleId = module->id;
                    history->oldLocation = module->centerEmblem;
                    history->newLocation = centerEmblem;

                    APP->history->push (history);

                    module->centerEmblem = centerEmblem;
                    module->updateEmblem = true;
                }
            );
        };

        _WidgetBase::createLocalStyleMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createMenuLabel ("Emblem location"));

        menu->addChild (createEmblemLocationItem ("Default", CenterEmblem::Default));
        menu->addChild (createEmblemLocationItem ("Bottom", CenterEmblem::False));
        menu->addChild (createEmblemLocationItem ("Center", CenterEmblem::True));
    }

    void CableColorModuleWidget::createPluginSettingsMenu (rack::ui::Menu* menu) {
        using rack::createBoolMenuItem;
        using rack::createBoolPtrMenuItem;
        using rack::createMenuItem;
        using rack::createMenuLabel;

        _WidgetBase::createPluginSettingsMenu (menu);

        // Visual.
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Visual"));
        menu->addChild (createBoolPtrMenuItem ("Display key mappings", "", &pluginSettings.cableColor_DisplayKeys));
        menu->addChild (createBoolMenuItem ("Center emblem", "",
            [] () { return pluginSettings.cableColor_CenterEmblem; },
            [&] (bool enable) {
                pluginSettings.cableColor_CenterEmblem = enable;
                module->updateEmblem = true;
            }
        ));

        // Behaviour.
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Behaviour"));
        menu->addChild (createBoolPtrMenuItem ("Latch color", "", &pluginSettings.cableColor_Latch));
        menu->addChild (createBoolPtrMenuItem ("Port hover mode", "", &pluginSettings.cableColor_PortHover));
        menu->addChild (createBoolPtrMenuItem ("Key mappings always active", "", &pluginSettings.cableColor_GlobalKeys));

        // Key mappings.
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Global key mappings"));
        auto keyMappingFunction = [=] (const std::string& keyName, CableColorKey* key) {
            menu->addChild (createMenuItem (
                fmt::format (FMT_STRING ("Set {}"), keyName),
                key->keyText (),
                [=] { module->colorManager->setLearnMode (keyName, key); },
                false, true
            ));
            menu->addChild (createMenuItem (
                fmt::format (FMT_STRING ("Unset {}"), keyName),
                "",
                [=] { *key = CableColorKey (); },
                !key->isMapped ()
            ));
        };
        keyMappingFunction ("toggle latch color", &pluginSettings.cableColor_LatchKey);
        menu->addChild (new rack::ui::MenuEntry);
        keyMappingFunction ("cycle forwards", &pluginSettings.cableColor_CycleFwdKey);
        menu->addChild (new rack::ui::MenuEntry);
        keyMappingFunction ("cycle backwards", &pluginSettings.cableColor_CycleBackKey);
    }

    static std::string getCableColorMenuItemText (const CableColor& color) {
        return fmt::format (FMT_STRING ("     {}"), color.label);
    }

    struct SaveCollectionMenuItem : rack::ui::MenuItem {
      private:
        CableColorModule* module;
        UI::TextField* nameField;

      public:
        SaveCollectionMenuItem (CableColorModule* newModule, UI::TextField* newNameField) {
            module = newModule;
            nameField = newNameField;

            text = "Save collection";
            rightText = "";
        }

        void saveCollection (const std::string& name) {
            auto collection = module->colorManager->getCollection ();
            collection.setName (name);
            pluginSettings.cableColor_Collections.addCollection (collection);
        }

        void onAction (const rack::event::Action& e) override {
            auto name = nameField->text;
            if (!pluginSettings.cableColor_Collections.hasCollection (name)) {
                saveCollection (name);
                e.consume (this);

                return;
            }

            auto menu = rack::createMenu ();
            menu->addChild (rack::createMenuLabel ("A collection with the specified name already exists. Are you sure?"));
            menu->addChild (rack::createMenuItem ("Save collection", "", [=] { saveCollection (name); }, false, true));
        }
    };

    void CableColorModuleWidget::createCollectionsMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createMenuItem;
        using rack::createMenuLabel;
        using rack::createSubmenuItem;

        menu->addChild (new UI::SafeMenuItem (
            "Reset collections list",
            [=] { pluginSettings.cableColor_Collections = CollectionsStorage::defaults (); },
            true
        ));

        menu->addChild (new rack::ui::MenuSeparator);
        auto newCollectionNameTextField = new UI::TextField ();
        newCollectionNameTextField->box.size.x = 200.f;
        newCollectionNameTextField->text = "";
        newCollectionNameTextField->placeholder = "Collection name...";
        menu->addChild (newCollectionNameTextField);
        menu->addChild (new SaveCollectionMenuItem (module, newCollectionNameTextField));

        menu->addChild (new rack::ui::MenuSeparator);
        auto defaultCollectionName = pluginSettings.cableColor_Collections.getDefaultCollectionName ();
        for (const auto& collectionKVP : pluginSettings.cableColor_Collections) {
            auto collectionName = collectionKVP.first;
            auto rightText = (collectionName == defaultCollectionName) ? "[Default]" : "";

            menu->addChild (rack::createSubmenuItem (collectionName, rightText, [=] (Menu* menu) {
                menu->addChild (createMenuItem (
                    "Load to module",
                    "",
                    [=] { module->colorManager->changeCollection (collectionKVP.second, true); }
                ));
                menu->addChild (createMenuItem (
                    "Set as default collection",
                    "",
                    [=] { pluginSettings.cableColor_Collections.setDefaultCollection (collectionName); }
                ));
                menu->addChild (new UI::SafeMenuItem (
                    "Delete collection",
                    [=] { pluginSettings.cableColor_Collections.removeCollection (collectionName); },
                    true
                ));

                menu->addChild (new rack::ui::MenuSeparator);
                menu->addChild (createMenuLabel ("Colors"));
                for (uint32_t i = 0; i < collectionKVP.second.count (); i++) {
                    const auto& cableColor = collectionKVP.second [i];

                    auto colorItem = new UI::ColorMenuItem;
                    colorItem->color = cableColor.color;
                    colorItem->text = getCableColorMenuItemText (cableColor);
                    colorItem->rightText = cableColor.key.keyText ();
                    colorItem->disabled = true;

                    menu->addChild (colorItem);
                }
            }));
        }
    }

    template<typename T = rack::ui::MenuItem>
    struct ReplacePatchCablesItem : public T {
      protected:
        CableColorModule* module;
        bool isCollectionColor;
        uint32_t collectionIndex;

        virtual std::vector<rack::app::CableWidget*> getCables () = 0;
        rack::ui::Menu* createChildMenu () override {
            using rack::ui::Menu;
            using rack::createMenuItem;

            Menu* menu = new Menu;

            const auto& colorCollection = module->colorManager->getCollection ();
            for (uint32_t i = 0; i < colorCollection.count (); i++) {
                auto cableColor = colorCollection [i];
                auto colorItem = createMenuItem<UI::ColorMenuItem> (
                    getCableColorMenuItemText (cableColor), "",
                    [=] {
                        auto cables = getCables ();
                        module->colorManager->replacePatchCableColor (cables, i);
                    },
                    isCollectionColor && i == collectionIndex
                );
                colorItem->color = cableColor.color;
                menu->addChild (colorItem);
            }

            menu->addChild (new rack::ui::MenuSeparator);
            menu->addChild (createMenuItem ("Sequence", "", [=] {
                auto cables = getCables ();
                module->colorManager->replacePatchCableColorAll (cables, false);
            }));
            menu->addChild (createMenuItem ("Random", "", [=] {
                auto cables = getCables ();
                module->colorManager->replacePatchCableColorAll (cables, true);
            }));

            return menu;
        }
    };

    struct ReplacePatchCablesColorItem : ReplacePatchCablesItem<UI::ColorMenuItem> {
        ReplacePatchCablesColorItem (CableColorModule* newModule, NVGcolor newColor) {
            module = newModule;
            color = newColor;
            text = "";
            rightText = RIGHT_ARROW;

            isCollectionColor = false;
            collectionIndex = 0;
        }

        ReplacePatchCablesColorItem (CableColorModule* newModule, uint32_t newIndex) {
            module = newModule;

            auto cableColor = module->colorManager->getCollection () [newIndex];
            color = cableColor.color;
            text = getCableColorMenuItemText (cableColor);
            rightText = RIGHT_ARROW;

            isCollectionColor = true;
            collectionIndex = newIndex;
        }

        std::vector<rack::app::CableWidget*> getCables () override {
            std::vector<rack::app::CableWidget*> cables;

            if (auto cableContainer = APP->scene->rack->getCableContainer ()) {
                for (auto it : cableContainer->children) {
                    auto cable = dynamic_cast<rack::app::CableWidget*> (it);
                    if (cable == nullptr || !cable->isComplete ())
                        continue;

                    if (!rack::color::isEqual (cable->color, color))
                        continue;

                    cables.push_back (cable);
                }
            }

            return cables;
        }
    };

    struct ReplacePatchCablesAllItem : ReplacePatchCablesItem<rack::ui::MenuItem> {
        ReplacePatchCablesAllItem (CableColorModule* newModule) {
            module = newModule;
            text = "All";
            rightText = RIGHT_ARROW;

            isCollectionColor = false;
            collectionIndex = 0;
        }

        std::vector<rack::app::CableWidget*> getCables () override {
            std::vector<rack::app::CableWidget*> cables;

            if (auto cableContainer = APP->scene->rack->getCableContainer ()) {
                for (auto it : cableContainer->children) {
                    auto cable = dynamic_cast<rack::app::CableWidget*> (it);
                    if (cable == nullptr || !cable->isComplete ())
                        continue;

                    cables.push_back (cable);
                }
            }

            return cables;
        }
    };

    void CableColorModuleWidget::createReplacePatchCablesMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createMenuItem;
        using rack::createMenuLabel;
        using rack::createSubmenuItem;

        auto colorManager = module->colorManager;
        const auto& colorCollection = colorManager->getCollection ();

        std::vector<NVGcolor> patchColors;
        std::unordered_set<NVGcolor> patchColorsSet;

        // Find what cable colors are used in the patch.
        if (auto cableContainer = APP->scene->rack->getCableContainer ()) {
            for (auto it : cableContainer->children) {
                auto cable = dynamic_cast<rack::app::CableWidget*> (it);
                if (cable == nullptr || !cable->isComplete ())
                    continue;

                auto cableColor = cable->color;
                if (patchColorsSet.find (cableColor) != patchColorsSet.end ())
                    continue;

                patchColors.push_back (cableColor);
                patchColorsSet.insert (cableColor);
            }
        }

        // Insert items for the collection colors first.
        auto hasCollectionColors = false;
        for (uint32_t i = 0; i < colorCollection.count (); i++) {
            auto color = colorCollection [i].color;
            if (auto search = patchColorsSet.find (color); search == patchColorsSet.end ())
                continue;

            menu->addChild (new ReplacePatchCablesColorItem (module, i));

            patchColorsSet.erase (color);
            hasCollectionColors = true;
        }

        // Insert items for the non-collection colors.
        auto hasNonCollectionColors = false;
        for (auto color : patchColors) {
            if (auto search = patchColorsSet.find (color); search == patchColorsSet.end ())
                continue;

            if (!hasNonCollectionColors && hasCollectionColors)
                menu->addChild (new rack::ui::MenuSeparator);

            menu->addChild (new ReplacePatchCablesColorItem (module, color));

            patchColorsSet.erase (color);
            hasNonCollectionColors = true;
        }

        // Add the item for replacing all colors.
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (new ReplacePatchCablesAllItem (module));
    }

    void CableColorModuleWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::createCheckMenuItem;
        using rack::createMenuItem;
        using rack::createMenuLabel;
        using rack::createSubmenuItem;
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createSubmenuItem (
            "Replace patch cable colors", "",
            [=] (Menu* menu) { createReplacePatchCablesMenu (menu); }
        ));

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createSubmenuItem ("Collections", "", [=] (Menu* menu) { createCollectionsMenu (menu); }));
        menu->addChild (createMenuItem (
            "Add new color", "",
            [=] {
                module->colorManager->addNewColor (
                    nvgHSL (rack::random::uniform (), 1, .5 + rack::random::normal () / 2.),
                    CableColorKey (),
                    ""
                );
            },
            false, true
        ));
        menu->addChild (createMenuItem (
            "Delete all colors", "",
            [=] { module->colorManager->clearColors (true); }
        ));
    }
}
}