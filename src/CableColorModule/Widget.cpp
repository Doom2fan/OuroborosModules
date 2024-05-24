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
#include "CableColorManager.hpp"
#include "CCM_Common.hpp"
#include "ColorDisplayWidget.hpp"

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

        auto emblemPos = centerEmblem ? box.size.div (2) : findNamed ("widgetLogo");
        auto emblemSize = rack::window::mm2px (centerEmblem ? 45.296f : 8.719f);

        emblemWidget->setZoom (emblemSize);
        emblemWidget->setSize (rack::math::Vec (emblemSize));
        emblemWidget->box.pos = emblemPos.value_or (rack::math::Vec ()).minus (emblemWidget->box.size.div (2));
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
        auto defaultCollectionName = pluginSettings.cableColor_Collections.getDefaultCollectionName ();
        for (const auto& collectionKVP : pluginSettings.cableColor_Collections) {
            auto collectionName = collectionKVP.first;
            auto rightText = (collectionName == defaultCollectionName) ? "[Default]" : "";

            menu->addChild (rack::createSubmenuItem (collectionName, rightText, [=] (Menu* menu) {
                menu->addChild (createMenuItem (
                    "Replace current collection",
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
                    colorItem->text = fmt::format (FMT_STRING ("     {}"), cableColor.label);
                    colorItem->rightText = cableColor.key.keyText ();
                    colorItem->disabled = true;

                    menu->addChild (colorItem);
                }
            }));
        }
    }

    void CableColorModuleWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::createCheckMenuItem;
        using rack::createMenuItem;
        using rack::createMenuLabel;
        using rack::createSubmenuItem;
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

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