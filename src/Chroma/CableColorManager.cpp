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

#include "CableColorManager.hpp"

#include "../CableHandler.hpp"
#include "../JsonUtils.hpp"
#include "../PluginSettings.hpp"
#include "../UI/Input.hpp"
#include "CCM_Common.hpp"
#include "Chroma.hpp"
#include "HistoryActions.hpp"

#include <fmt/format.h>

namespace {
    std::weak_ptr<OuroborosModules::Modules::Chroma::CableColorManager> currentColorManagerPtr;
}

namespace OuroborosModules::Modules::Chroma {
    std::shared_ptr<CableColorManager> getColorManager () {
        if (auto manager = currentColorManagerPtr.lock ())
            return manager;

        auto manager = std::make_shared<CableColorManager> ();
        currentColorManagerPtr = manager;
        return manager;
    }

    /*
     * CableColorKey
     */
    const std::string CableColorKey::keyText () const {
        if (!isMapped ())
            return "[None]";

        auto ctrlText = (mods & RACK_MOD_CTRL) ? fmt::format (FMT_STRING ("{}+"), RACK_MOD_CTRL_NAME) : "";
        auto shiftText = (mods & GLFW_MOD_SHIFT) ? "Shift+" : "";
        auto altText = (mods & GLFW_MOD_ALT) ? "Alt+" : "";
        auto keyText = key != -1
                     ? Input::keyName (key)
                     : Input::buttonName (button);

        return fmt::format (FMT_STRING ("{}{}{}{}"), ctrlText, shiftText, altText, keyText);
    }

    bool CableColorKey::matches (CableColorKey otherKey) {
        if (otherKey.mods != mods)
            return false;

        if (button != -1)
            return otherKey.button == button;
        if (key != -1)
            return otherKey.key == key;

        return false;
    }

    json_t* CableColorKey::dataToJson () const {
        auto keyJ = json_object ();

        json_object_set_new_int (keyJ, "button", button);
        json_object_set_new_int (keyJ, "key", key);
        json_object_set_new_int (keyJ, "mods", mods);

        return keyJ;
    }

    bool CableColorKey::dataFromJson (json_t* keyJ) {
        if (!json_is_object (keyJ))
            return false;

        json_object_try_get_int (keyJ, "button", button);
        json_object_try_get_int (keyJ, "key", key);
        json_object_try_get_int (keyJ, "mods", mods);

        return true;
    }

    /*
     * CableColor
     */
    json_t* CableColor::dataToJson () const {
        auto colorJ = json_object ();

        json_object_set_new_float (colorJ, "color::R", color.r);
        json_object_set_new_float (colorJ, "color::G", color.g);
        json_object_set_new_float (colorJ, "color::B", color.b);
        json_object_set_new_float (colorJ, "color::A", color.a);

        json_object_set_new (colorJ, "key", key.dataToJson ());

        json_object_set_new_string (colorJ, "label", label);

        return colorJ;
    }

    bool CableColor::dataFromJson (json_t* colorJ) {
        if (!json_is_object (colorJ))
            return false;

        json_object_try_get_float (colorJ, "color::R", color.r);
        json_object_try_get_float (colorJ, "color::G", color.g);
        json_object_try_get_float (colorJ, "color::B", color.b);
        json_object_try_get_float (colorJ, "color::A", color.a);

        auto keyJ = json_object_get (colorJ, "key");
        if (!key.dataFromJson (keyJ))
            return false;

        json_object_try_get_string (colorJ, "label", label);

        return true;
    }

    /*
     * CableColorCollection
     */
    void CableColorCollection::resetToDefaults () {
        clear ();

        setName ("Default");
        addColor (rack::color::fromHexString ("#F3374B"), CableColorKey (-1, GLFW_KEY_1, 0), "");
        addColor (rack::color::fromHexString ("#FFB437"), CableColorKey (-1, GLFW_KEY_2, 0), "");
        addColor (rack::color::fromHexString ("#00B56E"), CableColorKey (-1, GLFW_KEY_3, 0), "");
        addColor (rack::color::fromHexString ("#3695EF"), CableColorKey (-1, GLFW_KEY_4, 0), "");
        addColor (rack::color::fromHexString ("#8B4ADE"), CableColorKey (-1, GLFW_KEY_5, 0), "");
    }

    json_t* CableColorCollection::dataToJson () const {
        auto collectionJ = json_object ();

        json_object_set_new_string (collectionJ, "name", name);

        auto colorsJ = json_array ();
        for (auto color : colors)
            json_array_append_new (colorsJ, color.dataToJson ());

        json_object_set_new (collectionJ, "colors", colorsJ);

        return collectionJ;
    }

    bool CableColorCollection::dataFromJson (json_t* collectionJ) {
        if (!json_is_object (collectionJ))
            return false;

        name = "";
        json_object_try_get_string (collectionJ, "name", name);

        colors.clear ();
        auto colorsJ = json_object_get (collectionJ, "colors");
        if (json_is_array (colorsJ)) {
            size_t colorIdx;
            json_t* colorJ;
            json_array_foreach (colorsJ, colorIdx, colorJ) {
                auto color = CableColor ();

                if (color.dataFromJson (colorJ))
                    colors.push_back (color);
            }
        }

        return true;
    }

    void CableColorCollection::removeColor (uint32_t index) {
        if (index >= count ())
            return;
        colors.erase (colors.begin () + index);
    }

    void CableColorCollection::addColor (uint32_t index, CableColor color) {
        if (index > count ())
            return;
        colors.insert (colors.begin () + index, color);
    }

    /*
     * CollectionsStorage
     */
    CollectionsStorage CollectionsStorage::defaults () {
        using rack::color::fromHexString;

        auto collectionsStorage = CollectionsStorage ();

        auto defaultCollection = CableColorCollection ();
        defaultCollection.resetToDefaults ();
        collectionsStorage.addCollection (defaultCollection);
        collectionsStorage.setDefaultCollection (defaultCollection.getName ());

        auto omriCollection = CableColorCollection ("Omri Cohen/Modular Fungi");
        omriCollection.addColor (fromHexString ("#c91847"), CableColorKey (-1, GLFW_KEY_1, 0), "Audio");
        omriCollection.addColor (fromHexString ("#0986ad"), CableColorKey (-1, GLFW_KEY_2, 0), "Clk/Trg/Gate");
        omriCollection.addColor (fromHexString ("#c9b70e"), CableColorKey (-1, GLFW_KEY_3, 0), "V/Oct");
        omriCollection.addColor (fromHexString ("#0c8e15"), CableColorKey (-1, GLFW_KEY_4, 0), "Modulation");
        collectionsStorage.addCollection (omriCollection);

        auto ouroborosCollection = CableColorCollection ("Ouroboros");
        ouroborosCollection.addColor (fromHexString ("#f3374b"), CableColorKey (-1, GLFW_KEY_1, 0), "Audio");
        ouroborosCollection.addColor (fromHexString ("#ffb437"), CableColorKey (-1, GLFW_KEY_2, 0), "Gates");
        ouroborosCollection.addColor (fromHexString ("#00b56e"), CableColorKey (-1, GLFW_KEY_3, 0), "Clocks");
        ouroborosCollection.addColor (fromHexString ("#3695ef"), CableColorKey (-1, GLFW_KEY_4, 0), "Pitches");
        ouroborosCollection.addColor (fromHexString ("#8b4ade"), CableColorKey (-1, GLFW_KEY_5, 0), "Modulation");
        ouroborosCollection.addColor (fromHexString ("#ff40ff"), CableColorKey (-1, GLFW_KEY_6, 0), "Run/Rst/Sync/Etc");
        collectionsStorage.addCollection (ouroborosCollection);

        return collectionsStorage;
    }

    void CollectionsStorage::removeCollection (const std::string& collectionName) {
        collections.erase (collectionName);

        if (defaultCollectionName == collectionName)
            defaultCollectionName = "";
    }

    bool CollectionsStorage::hasCollection (const std::string& key) const {
        return collections.find (key) != collections.end ();
    }

    bool CollectionsStorage::tryGetCollection (const std::string& key, CableColorCollection& collection) const {
        if (auto search = collections.find (key); search != collections.end ()) {
            collection = search->second;
            return true;
        }

        return false;
    }

    bool CollectionsStorage::tryGetDefaultCollection (CableColorCollection& collection) const {
        return tryGetCollection (defaultCollectionName, collection);
    }

    bool CollectionsStorage::setDefaultCollection (const std::string& collectionName) {
        if (auto search = collections.find (collectionName); search == collections.end ())
            return false;

        defaultCollectionName = collectionName;
        return true;
    }

    json_t* CollectionsStorage::dataToJson () const {
        auto storageJ = json_object ();

        json_object_set_new_string (storageJ, "defaultCollectionName", defaultCollectionName);

        auto collectionsJ = json_array ();
        for (auto& collection : collections)
            json_array_append_new (collectionsJ, collection.second.dataToJson ());

        json_object_set_new (storageJ, "collections", collectionsJ);

        return storageJ;
    }

    void CollectionsStorage::dataFromJson (json_t* storageJ) {
        if (!json_is_object (storageJ))
            return;

        auto collectionsJ = json_object_get (storageJ, "collections");
        if (!json_is_array (collectionsJ))
            return;

        json_object_try_get_string (storageJ, "defaultCollectionName", defaultCollectionName);

        collections.clear ();

        size_t collectionIdx;
        json_t* collectionJ;
        json_array_foreach (collectionsJ, collectionIdx, collectionJ) {
            auto collection = CableColorCollection ();

            if (collection.dataFromJson (collectionJ))
                addCollection (collection);
        }
    }

    /*
     * CableColorManager
     */
    CableColorManager::CableColorManager () {
        if (!pluginSettings.chroma_Collections.tryGetDefaultCollection (colorCollection))
            colorCollection.resetToDefaults ();
    }

    void CableColorManager::setCurrentColor (uint32_t index, bool forced, bool allowPortHover) {
        if (index >= colorCollection.count ())
            return;

        forced |= pluginSettings.chroma_GlobalKeys;

        // Gather the cables.
        auto cablesToModify = std::vector<rack::app::CableWidget*> ();
        auto heldCable = APP->scene->rack->getIncompleteCable ();
        if (heldCable != nullptr)
            cablesToModify.push_back (heldCable);
        else if (allowPortHover && pluginSettings.chroma_PortHover) {
            auto hoveredWidget = APP->event->getHoveredWidget ();
            if (auto hoveredPort = dynamic_cast<rack::PortWidget*> (hoveredWidget))
                cablesToModify = APP->scene->rack->getCablesOnPort (hoveredPort);
        }

        // Update the cables.
        if (cablesToModify.size () > 0)
            replacePatchCableColor (cablesToModify, index);
        else if (!forced) // Don't set anything if we're not in global mode and there were no cables.
            return;


        curColorIndex = index;

        // Update VCV's data.
        updateCurrentColor ();
    }

    bool CableColorManager::checkUpdateHeldCable (rack::app::CableWidget* heldCable) {
        // There's no held cable or it's complete.
        if (heldCable == nullptr || heldCable->isComplete ())
            return false;

        // No new cable connected.
        if (cables_Handler == nullptr)
            cables_Handler = CableHandler::getHandler ();
        if (!cables_Handler->checkCableConnected ())
            return false;

        // The held cable has no connected ports. (Impossible?)
        if (heldCable->inputPort == nullptr && heldCable->outputPort == nullptr)
            return false;

        // Check if we're trying to duplicate a cable.
        if ((APP->window->getMods () & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
            auto portWidget = heldCable->inputPort != nullptr ? heldCable->inputPort : heldCable->outputPort;
            auto cableContainer = APP->scene->rack->getCableContainer ();
            // Check if there's any other cable at the connected end of the cable.
            for (auto it = cableContainer->children.rbegin (); it != cableContainer->children.rend (); it++) {
                auto curCable = dynamic_cast<rack::app::CableWidget*> (*it);
                assert (curCable != nullptr);
                if (curCable == heldCable || !curCable->isComplete ())
                    continue;
                if (curCable->inputPort == portWidget || curCable->outputPort == portWidget)
                    return false;
            }
        }

        return true;
    }

    void CableColorManager::updateCurrentColor () {
        if (curColorIndex >= colorCollection.count ())
            curColorIndex = colorCollection.count () - 1;

        // Handle the held cable.
        auto heldCable = APP->scene->rack->getIncompleteCable ();
        if (checkUpdateHeldCable (heldCable)) {
            auto cablesToModify = std::vector<rack::app::CableWidget*> ();
            cablesToModify.push_back (heldCable);
            replacePatchCableColor (cablesToModify, curColorIndex);

            if (!pluginSettings.chroma_Latch) {
                if (++curColorIndex >= colorCollection.count ())
                    curColorIndex = 0;
            }
        }
    }

    void CableColorManager::replacePatchCableColor (std::vector<rack::app::CableWidget*>& cables, uint32_t index) {
        if (index >= colorCollection.count () || cables.size () < 1)
            return;

        auto selectedColor = colorCollection [index];

        auto batchAction = new rack::history::ComplexAction ();
        batchAction->name = "change cable color";

        auto anyActionAdded = false;
        for (auto cable : cables) {
            if (!cable->isComplete ()) {
                cable->color = selectedColor.color;
                continue;
            }

            if (cable->getCable () == nullptr || selectedColor.color == cable->color)
                continue;

            auto cableAction = new rack::history::CableColorChange ();

            cableAction->oldColor = cable->color;
            cableAction->newColor = selectedColor.color;
            cableAction->setCable (cable);

            cable->color = selectedColor.color;

            batchAction->push (cableAction);
            anyActionAdded = true;
        }

        if (anyActionAdded)
            APP->history->push (batchAction);
        else
            delete batchAction;
    }

    void CableColorManager::replacePatchCableColorAll (std::vector<rack::app::CableWidget*>& cables, bool random) {
        if (cables.size () < 1 || colorCollection.count () < 1)
            return;

        auto batchAction = new rack::history::ComplexAction ();
        batchAction->name = "change cable color";

        auto prevIndex = 0;
        auto anyActionAdded = false;
        for (auto cable : cables) {
            if (cable->getCable () == nullptr)
                continue;

            auto selectedColor = random
                ? colorCollection [rack::random::u32 () % colorCollection.count ()].color
                : colorCollection [prevIndex++ % colorCollection.count ()].color;

            if (selectedColor == cable->color)
                continue;

            auto cableAction = new rack::history::CableColorChange ();
            cableAction->oldColor = cable->color;
            cableAction->newColor = selectedColor;
            cableAction->setCable (cable);

            cable->color = selectedColor;

            batchAction->push (cableAction);
            anyActionAdded = true;
        }

        if (anyActionAdded)
            APP->history->push (batchAction);
        else
            delete batchAction;
    }

    void CableColorManager::changeCollection (const CableColorCollection& collection, bool createHistory) {
        if (createHistory)
            APP->history->push (new HistoryReplaceCollection (colorCollection, collection));

        unsetLearnMode (); // Shouldn't be possible, but let's make sure anyway.
        colorCollection = collection;
        setCurrentColor (0, true);
    }

    void CableColorManager::clearColors (bool createHistory) {
        if (createHistory)
            APP->history->push (new DeleteAllColorsHistory (colorCollection));

        unsetLearnMode (); // Shouldn't be possible, but let's make sure anyway.
        colorCollection.clear ();
        setCurrentColor (0, true);
    }

    static void showLearnMessage (std::string keyName) {
        if (masterKeyContainer != nullptr) {
            masterKeyContainer->displayMessage (fmt::format (
                FMT_STRING ("Mapping '{}' key.\n\nWaiting for a key press.\nPress Esc to cancel."),
                keyName
            ));
        }
    }

    void CableColorManager::unsetLearnMode () {
        if (masterKeyContainer != nullptr)
            masterKeyContainer->closeMessage ();

        learnMode = LearnMode::Off;
        learnIndex = 0;
        learnKeyPtr = nullptr;
    }

    void CableColorManager::setLearnMode (std::string keyName, uint32_t colorIndex) {
        unsetLearnMode ();

        learnMode = LearnMode::LearnColor;
        learnIndex = colorIndex;
        showLearnMessage (keyName);
    }

    void CableColorManager::setLearnMode (std::string keyName, CableColorKey* key) {
        unsetLearnMode ();

        learnMode = LearnMode::LearnKeyPtr;
        learnKeyPtr = key;
        showLearnMessage (keyName);
    }

    void CableColorManager::addNewColor (CableColor color) {
        auto history = new AddNewColorHistory (colorCollection.count (), color);
        APP->history->push (history);

        colorCollection.addColor (color);
    }

    void CableColorManager::removeColor (uint32_t index) {
        if (index >= colorCollection.count ())
            return;

        auto history = new DeleteColorHistory (index, colorCollection [index]);
        APP->history->push (history);

        colorCollection.removeColor (index);
    }

    void CableColorManager::setColor (uint32_t index, NVGcolor color) {
        if (index >= colorCollection.count ())
            return;

        auto oldColor = colorCollection [index];
        auto newColor = oldColor;
        newColor.color = color;
        APP->history->push (new ChangeColorHistory ("change cable color color", index, oldColor, newColor));

        colorCollection.setColor (index, newColor);
    }

    void CableColorManager::setColorLabel (uint32_t index, std::string label) {
        if (index >= colorCollection.count ())
            return;

        auto oldColor = colorCollection [index];
        auto newColor = oldColor;
        newColor.label = label;
        APP->history->push (new ChangeColorHistory ("change cable color label", index, oldColor, newColor));

        colorCollection.setColor (index, newColor);
    }

    void CableColorManager::setColorKey (uint32_t index, CableColorKey key) {
        if (index >= colorCollection.count ())
            return;

        auto oldColor = colorCollection [index];
        auto newColor = oldColor;
        newColor.key = key;
        APP->history->push (new ChangeColorHistory ("change cable color key mapping", index, oldColor, newColor));

        colorCollection.setColor (index, newColor);
    }

    bool CableColorManager::handleKey (CableColorKey key) {
        if (isLearnMode ())
            return learnKey (key);

        auto cycleFunction = [&] (int dir) {
            auto colorCount = static_cast<int> (colorCollection.count ());
            auto newIndex = static_cast<int> (curColorIndex) + dir;
            if (newIndex >= colorCount)
                newIndex = 0;
            else if (newIndex < 0)
                newIndex = colorCount - 1;

            setCurrentColor (static_cast<uint32_t> (newIndex), false, false);
        };

        if (pluginSettings.chroma_LatchKey.matches (key)) {
            pluginSettings.chroma_Latch ^= true;
            return true;
        } else if (pluginSettings.chroma_CycleFwdKey.matches (key)) {
            cycleFunction (1);
            return true;
        } else if (pluginSettings.chroma_CycleBackKey.matches (key)) {
            cycleFunction (-1);
            return true;
        }

        for (uint32_t i = 0; i < colorCollection.count (); i++) {
            auto cableColor = colorCollection [i];

            if (!cableColor.key.matches (key))
                continue;

            setCurrentColor (i, false, true);
            return true;
        }

        return false;
    }

    bool CableColorManager::learnKey (CableColorKey key) {
        if (learnMode != LearnMode::Off && key.matches (CableColorKey (-1, GLFW_KEY_ESCAPE, 0))) {
            unsetLearnMode ();
            return true;
        }

        switch (learnMode) {
            case Off:
                return false;

            case LearnMode::LearnColor: {
                if (learnIndex >= colorCollection.count ()) {
                    // This should never happen...
                    unsetLearnMode ();
                    return false;
                }

                setColorKey (learnIndex, key);
                break;
            }

            case LearnMode::LearnKeyPtr: {
                if (learnKeyPtr != nullptr)
                    *learnKeyPtr = key;

                break;
            }
        }

        unsetLearnMode ();
        return true;
    }

    json_t* CableColorManager::dataToJson () const {
        auto managerJ = json_object ();

        json_object_set_new_int (managerJ, "curColorIndex", curColorIndex);
        json_object_set_new (managerJ, "colorCollection", colorCollection.dataToJson ());

        return managerJ;
    }

    bool CableColorManager::dataFromJson (json_t* managerJ) {
        if (!json_is_object (managerJ))
            return false;

        json_object_try_get_int (managerJ, "curColorIndex", curColorIndex);

        auto collectionJ = json_object_get (managerJ, "colorCollection");
        if (!colorCollection.dataFromJson (collectionJ))
            return false;

        updateCurrentColor ();

        return true;
    }
}