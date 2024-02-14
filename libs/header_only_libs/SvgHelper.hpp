// Modified by Chronos "phantombeta" Ouroboros

#pragma once

#include <optional>
#include <regex>

#include "nanosvg.h"
#include "rack.hpp"

template <typename T>
struct SvgHelper {
private:
    std::shared_ptr<rack::window::Svg> svg;

    rack::app::ModuleWidget* moduleWidget() {
        auto t = static_cast<T*>(this);
        return static_cast<rack::app::ModuleWidget*>(t);
    }

public:
    void loadPanel(const std::string& filename) {
        auto panel = static_cast<rack::app::SvgPanel*>(moduleWidget()->getPanel());
        if (panel == nullptr) {
            panel = rack::createPanel(filename);
            moduleWidget()->setPanel(panel);
        } else {
            panel->setBackground(rack::window::Svg::load(filename));
        }

        svg = panel->svg;
    }

    void forEachShape(const std::function<void(NSVGshape*)>& callback) {
        if (svg == nullptr) {
            return;
        }

        auto shapes = svg->handle->shapes;
        for (NSVGshape* shape = shapes; shape != nullptr; shape = shape->next) {
            callback(shape);
        }
    }

    std::optional<rack::math::Vec> findNamed(std::string name) {
        std::optional<rack::math::Vec> result;

        if (svg == nullptr) {
            return result;
        }

        forEachShape([&](NSVGshape* shape) {
            if (std::string(shape->id) == name) {
                auto bounds = shape->bounds;
                result = rack::math::Vec();
                result->x = (bounds[0] + bounds[2]) / 2;
                result->y = (bounds[1] + bounds[3]) / 2;
                return;
            }
        });

        return result;
    }

    std::vector<rack::math::Vec> findPrefixed(std::string prefix) {
        std::vector<rack::math::Vec> result;

        if (svg == nullptr) {
            return result;
        }

        forEachShape([&](NSVGshape* shape) {
            if (std::string(shape->id).find(prefix) == 0) {
                auto bounds = shape->bounds;
                auto center = rack::math::Vec((bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2);
                result.push_back(center);
            }
        });

        return result;
    }

    std::vector<std::pair<std::vector<std::string>, rack::math::Vec>> findMatched(const std::string& pattern) {
        std::vector<std::pair<std::vector<std::string>, rack::math::Vec>> result;

        if (svg == nullptr) {
            return result;
        }

        std::regex regex(pattern);

        forEachShape([&](NSVGshape* shape) {
            auto id = std::string(shape->id);

            std::vector<std::string> captures;
            std::smatch match;

            if (std::regex_search(id, match, regex)) {
                for (unsigned int i = 1; i < match.size(); i++) {
                    captures.push_back(match[i]);
                }
                auto bounds = shape->bounds;
                auto center = rack::math::Vec((bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2);
                result.emplace_back(captures, center);
            }
        });

        return result;
    }

    void forEachPrefixed(std::string prefix, const std::function<void(unsigned int i, rack::math::Vec)>& callback) {
        if (svg == nullptr) {
            return;
        }

        auto positions = findPrefixed(prefix);
        for (unsigned int i = 0; i < positions.size(); i++) {
            callback(i, positions[i]);
        }
    }

    void forEachMatched(const std::string& regex, const std::function<void(std::vector<std::string>, rack::math::Vec)>& callback) {
        if (svg == nullptr) {
            return;
        }

        auto matches = findMatched(regex);
        for (const auto& match : matches) {
            callback(match.first, match.second);
        }
    }
};