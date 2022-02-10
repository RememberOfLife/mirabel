#include "imgui.h"
#include "surena/games/havannah.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

#include "games/havannah.hpp"

namespace Games {

            Havannah::Havannah():
                BaseGameVariant("Standard")
            {}
            
            Havannah::~Havannah()
            {}

            surena::Game* Havannah::new_game()
            {
                return new surena::Havannah(size);
            }

            void Havannah::draw_options()
            {
                ImGui::InputScalar("size", ImGuiDataType_U32, &size);
                if (size < 4) {
                    size = 4;
                }
                if (size > 10) {
                    size = 10;
                }
            }

            void Havannah::draw_state_editor(surena::Game* game)
            {
                surena::Havannah* state = dynamic_cast<surena::Havannah*>(game);
                if (state == nullptr) {
                    return;
                }
                //TODO proper state editor
                const char* check_options[4] = {"-", "X", "O", "-"}; // needs 2 dashes for none AND invalid
                ImGui::Text("player to move: %s", check_options[state->player_to_move()]);
                ImGui::Text("result: %s", check_options[state->get_result()]);
            }

            const char* Havannah::description()
            {
                //TODO
                return "Havannah Description";
            }

}
