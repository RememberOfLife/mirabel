# mirabel

General purpose board game playing GUI.

## Future Features
* Engine Integration
* Online (Multiplayer) Play


additionally requires stb for compiling nanovg

import blocks style:
* all standard libs
* imports from dependencies in order [SDL, nanovg, imgui, surena]
* imports from own src tree
* import header for things implemented in this source file


https://stackoverflow.com/questions/28395833/using-sdl2-with-cmake

// usual gameboard colors
//rgb(201, 144, 73) "wood" normal
//rgb(240, 217, 181) "wood" light
//rgb(161, 119, 67) "wood" dark
//rgb(236, 236, 236) white
//rgb(210, 210, 210) white accent
//rgb(11, 11, 11) black
//rgb(25, 25, 25) black accent
//rgb(199, 36, 73) neon red/pink
//rgb(37, 190, 223) neon cyan
//rgb(120, 25, 25) dark red or use (141, 35, 35)
//rgb(24, 38, 120) dark blue

// sounds required:
// - card pickup
// - card placement
// - card shuffle light
// - card shuffle heavy
// - pencil on paper, one scratch
// - pencil on paper, multiple scratches
// - "wood/stone" piece placement
// - "wood/stone" piece capture
// - game start beep
// - game end jingle
// - low on time beep

gui:
http://www.cmyr.net/blog/gui-framework-ingredients.html
https://linebender.org/druid/widget.html
http://www.cmyr.net/blog/druid-dynamism.html

## issues
* when a frontend is started, all imgui windows get darker, doesnt happen with the game loader
* when starting the game chess, while its frontend is loaded, the screen flickers once

## todo
* change window title according to the loaded game and frontend
* nanovg context when passed should not stretch under the main menu bar area of the screen
* sound
  * sound menu for muting and volume
  * https://gist.github.com/armornick/3447121
  * https://metacpan.org/pod/SDL2::audio
  * https://github.com/jakebesworth/Simple-SDL2-Audio
* actually use clang-format to make everything look uniform
* main_ctrl should be a context object (low prio)
* put nanovg in another cmake target to hide its impl warnings
* put correct cpp standard version in the cmake

## ideas
* maybe make the state editor something like a toggle?
  * so that for e.g. chess it just enables unlocked dragging about of pieces, and provides a bar with generic pieces to choose from
  * that would require interaction between the frontend and the basegamevariant
* meta gui window snapping/anchoring?
* "moving" around the board by an offset using middle mouse dragging?
* filter games list with some categories/tags? (e.g. player count, randomness, hidden info, simul moves, abstract strategy)
* quick launcher menu:
  * "hav" matches to havannah and the standard variation and the first available frontend
  * "hav+10" should set the size option to 10 for the havannah that matched
    * how to make sure non kwargs get into the right place?
  * "ttt.u/3d" matches tictactoe.ultimate/TicTacToe3D

## problems
* make games,frontends,engines dynamically loadable as plugins
  * needs an extra window
* local docs / game rule window, per variant? images/graphic representations?
  * load rules from res?
* how to handle game notation window and past game states keeping? (definitely want to skip around in past states)
  * history manager is owned by the notation/history metagui window, which also offers loading+saving of notation files
  * is the history manager actually kept there? or in another place
    * could also send an event to set the displayed state
  * ==> history manager only sets guithread state, engine still calcs on the newest one, guithread events for new game moves get applied to the newest state (not shown), history manager has option to distribute viewing state to engine and network
* design networking structure for offline/online server play
  * SDL_net for tcp connections
* where to store state info from the engine like uci opts?
* what to do when engine is loaded but no game?
  * engine should not crash, just return garbage
* localization
* frontend should be able to easily change the cursor
* file paths should have some sort of file manager menu
  * https://codereview.stackexchange.com/questions/194553/c-dear-imgui-file-browser

### integration workflow
* ==> offline ai play:
  * engine has an option to enable auto search and auto move when certain players are playing
  * i.e. the engine always runs, but can be configured to only show hints when a certain player is playing
  * or configured to automatically start searching with timeout param, and also to automatically submit its move to the controller
* ==> server concept
  * server is a class inside the project, can also be hosted locally or spun up locally for the network
  * supports guest login, but also user accounts
* ==> manage games with randomness (also works for long term hidden state)
  * DEPENDS ON server architecture (also for offline play)
  * lobby setting, where the server auto decides random moves if set to host mode
    * or if the game is just mirroring another actual game then any lobby mod can input the random moves
  * every user in the lobby can also be set to be an unknown (e.g. mirroring a real person we don't know state about), i.e. their state will not be decided by the system
    * all real users input their info (e.g. dealt cards) and then can use play and ai
* ==> manage player specific views for games with simultaneous moves or hidden information
  * frontend requires info on what view it should render, i.e. render only hidden info or move placer for the player of that view
  * auto switch view to player_to_move / next player if setting for that is given
* ==> resources like textures
  * can be generated on first launch into some local cache directory
  * not available for sounds?
* ==> lobby logic
  * can always change gamestate when user has perms for this in the current lobby (all perms given to everybody in local server play "offline")
* ==> engine compatiblity
  * e.g. uci-engine is a wrapper for an executable that can be specified via an option
* ==> animation within frontends?
  * e.g. when loading a game, or making a move, etc..
  * frontend does not necessarily need to be newest state, it just exposes somewhere if it is ready to accept new moves (or not if it still animating)
    * if the frontend is passed a move even though it said it doesnt want to, then it should cancel the current animation and process the move (may animate that)
    * possibly requires an extra buffer for stored moves so we don't pollute the guithread eventqueue
  * how to smooth the animation for self played moves, i.e. when the user drops a drag-and-drop piece onto its target square
    * normally the move_event is pushed AFTER the inbox event_queue is processed (in the sdl event queue for inputs)
    * that way the piece will be reset for one frame until it is processed in the next one
