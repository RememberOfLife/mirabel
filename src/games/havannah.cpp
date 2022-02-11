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

            void Havannah::draw_state_editor(surena::Game* abstract_game)
            {
                surena::Havannah* game = dynamic_cast<surena::Havannah*>(abstract_game);
                if (game == nullptr) {
                    return;
                }
                //TODO proper state editor
                // white is actually displayed red per default, but because that might be configurable it is kept uniform here
                const char* check_options[4] = {"-", "WHITE", "BLACK", "-"}; // needs 2 dashes for none AND invalid
                ImGui::Text("player to move: %s", check_options[game->player_to_move()]);
                ImGui::Text("result: %s", check_options[game->get_result()]);
                //TODO expose winningcondition
            }

            const char* Havannah::description()
            {
                //TODO
                return "Havannah Description";
            }

}
