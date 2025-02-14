#include <content/views/view_pattern_data.hpp>

#include <hex/api/content_registry.hpp>

#include <hex/providers/memory_provider.hpp>

#include <pl/patterns/pattern.hpp>
#include <wolv/utils/lock.hpp>

namespace hex::plugin::builtin {

    ViewPatternData::ViewPatternData() : View::Window("hex.builtin.view.pattern_data.name", ICON_VS_DATABASE) {
        // Handle tree style setting changes
        EventSettingsChanged::subscribe(this, [this] {
            m_treeStyle = ContentRegistry::Settings::read("hex.builtin.setting.interface", "hex.builtin.setting.interface.pattern_tree_style", 0);
            for (auto &drawer : m_patternDrawer.all())
                drawer->setTreeStyle(m_treeStyle);

            m_rowColoring = ContentRegistry::Settings::read("hex.builtin.setting.interface", "hex.builtin.setting.interface.pattern_data_row_bg", false);
            for (auto &drawer : m_patternDrawer.all())
                drawer->enableRowColoring(m_rowColoring);
        });

        EventPatternEvaluating::subscribe(this, [this]{
            (*m_patternDrawer)->reset();
        });

        EventPatternExecuted::subscribe(this, [this](auto){
            (*m_patternDrawer)->reset();
        });

        m_patternDrawer.setOnCreateCallback([this](prv::Provider *, auto &drawer) {
            drawer = std::make_unique<ui::PatternDrawer>();

            drawer->setSelectionCallback([](Region region){ ImHexApi::HexEditor::setSelection(region); });
            drawer->setTreeStyle(m_treeStyle);
            drawer->enableRowColoring(m_rowColoring);
        });
    }

    ViewPatternData::~ViewPatternData() {
        EventSettingsChanged::unsubscribe(this);
        EventPatternEvaluating::unsubscribe(this);
        EventPatternExecuted::unsubscribe(this);
    }

    void ViewPatternData::drawContent() {
        // Draw the pattern tree if the provider is valid
        if (ImHexApi::Provider::isValid()) {
            // Make sure the runtime has finished evaluating and produced valid patterns
            auto &runtime = ContentRegistry::PatternLanguage::getRuntime();

            const auto height = std::max(ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().FramePadding.y * 2, ImGui::GetTextLineHeightWithSpacing() * 5);
            if (!runtime.arePatternsValid()) {
                (*m_patternDrawer)->draw({ }, nullptr, height);
            } else {
                // If the runtime has finished evaluating, draw the patterns
                if (TRY_LOCK(ContentRegistry::PatternLanguage::getRuntimeLock())) {
                    (*m_patternDrawer)->draw(runtime.getPatterns(), &runtime, height);
                }
            }
        }
    }

}
