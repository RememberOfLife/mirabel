#include <cstdint>

#include "imgui.h"

#include "surena/engines/randomengine.hpp"
#include "surena/engines/singlethreadedmcts.hpp"
#include "surena/engine.hpp"

#include "engines/engine_catalogue.hpp"
#include "games/game_catalogue.hpp"

#include "engines/builtin_surena.hpp"

namespace Engines {

    Builtin_Surena::Builtin_Surena():
        Engine("surena <builtin>")
    {}

    bool Builtin_Surena::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return true;
    }

    surena::Engine* Builtin_Surena::new_engine()
    {
        return new surena::SinglethreadedMCTS();
    }

    void Builtin_Surena::draw_loader_options()
    {
        //TODO draw combo box for different engines surena offers
        ImGui::TextDisabled("<TODO loader options>");
    }

    void Builtin_Surena::draw_state_options(surena::Engine* engine)
    {
        if (engine == NULL) {
            ImGui::TextDisabled("<no game running>");
            return;
        }
        //TODO what goes here for builtin?
        ImGui::Text("bestmove: %s", engine->player_to_move() ? engine->get_move_string(engine->get_best_move()).c_str() : "-");
        ImGui::Text("search constraints:");
        ImGui::InputScalar("timeout (ms)", ImGuiDataType_U64, &constraints_timeout);
        if (ImGui::Button("Start Search")) {
            engine->search_start(constraints_timeout);
        }
        if (ImGui::Button("Start Stop")) {
            engine->search_stop();
        }
    }

    const char* Builtin_Surena::description()
    {
        return "surena <builtin>";
    }

}
