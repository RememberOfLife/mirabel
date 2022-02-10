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
//rgb(120, 25, 25) dark red
//rgb(24, 38, 120) dark blue

// sounds required:
// - pencil on paper
// - "wood/stone" piece placement
// - "wood/stone" piece capture

hexagons:
https://stackoverflow.com/questions/42903609/function-to-determine-if-point-is-inside-hexagon
http://www.playchilla.com/how-to-check-if-a-point-is-inside-a-hexagon

gui:
http://www.cmyr.net/blog/gui-framework-ingredients.html
https://linebender.org/druid/widget.html
http://www.cmyr.net/blog/druid-dynamism.html

## todo
* frontend config should do loading/unloading like the game (i.e. with dedicated buttons) instead of immediately upon selection, enables options before loading the actual frontend
* actually use clang-format to make everything look uniform
* sound
* main_ctrl should be a context object (low prio)

## ideas
* meta gui window snapping/anchoring?

## problems
* make games,frontends,engines dynamically loadable as plugins
* local docs / game rule window, per variant? images/graphic representations?
  * load rules from res?
* how to handle game notation window and past game states keeping? (definitely want to skip around in past states)
  * history manager is owned by the notation/history metagui window, which also offers loading+saving of notation files
  * is the history manager actually kept there? or in another place
    * could also send an event to set the displayed state
  * ==> history manager only sets guithread state, engine still calcs on the newest one, guithread events for new game moves get applied to the newest state (not shown), history manager has option to distribute viewing state to engine and network
* design networking structure for offline/online server play
  * SDL_net for tcp connections

### integration workflow
* ==> engine sends out events with best move updates, these get processed by the guithread and forwarded to the metagui and guistate
  * maybe save them somewhere in the guithread in some aux_info struct containing describables for the guistate to display and use
  * AUX_INFO struct also saves things like player_id <-> player_name mappings etc
* ==> metagui engine window is directly responsible for interacting with the engine via the controller.enginethread pointer
* ==> engine wrapper (enginethread) keeps a struct of configuration options with their string descriptors and everything, accessed by metagui for displaying, updating options done through events sent to the enginethread
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
  * engine catalogue
    * every engine wrapper has a function gamevariant_compatible(base_game_name, variant*)
  * inbuilt engine works with everything
    * one of the "options" for the built in engine is a variant, i.e. minimax/mcts/etc
  * e.g. uci-engine is a wrapper for an executable that can be specified via an option
* ==> animation within frontends?
  * e.g. when loading a game, or making a move, etc..
  * frontend does not necessarily need to be newest state, it just exposes somewhere if it is ready to accept new moves (or not if it still animating)
    * if the frontend is passed a move even though it said it doesnt want to, then it should cancel the current animation and process the move (may animate that)
    * possibly requires an extra buffer for stored moves so we don't pollute the guithread eventqueue
