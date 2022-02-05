# mirabel

import blocks style:
* all standard libs
* imports from dependencies
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
* actually use clang-format to make everything look uniform
* sound
* replace direct_draw with a maintained fork of [nanovg](https://github.com/inniyah/nanovg), or some other basic drawing library

## ideas
* meta gui window snapping/anchoring?

## problems
* make games,frontends,engines dynamically loadable as plugins
* engine compatiblity for arbitrary files?
* how does the built in engine deliver its moves to the enginethread
* how are options within the gamestate config window passed?
* local docs / game rule window, per variant? images/graphic representations?

# integration workflow

FIXES:
* ==> gamestate is passed to ctx on creation and loading of other games through ctx.loadgamestate(gamestate)
  * ctx keeps pointer to the gamestate it is supposed to render
  * watch out to make them all compatible by gametype they support
  * config window loading of other game or ctx puts the request into the postbox queue, so on next frame it is replaced
* ==> engine sends out events with best move updates, these get processed by the guithread and forwarded to the metagui and guistate
  * maybe save them somewhere in the guithread in some aux_info struct containing describables for the guistate to display and use
  * AUX_INFO struct also saves things like player_id <-> player_name mappings etc
* ==> metagui engine window is directly responsible for interacting with the engine via the controller.enginethread pointer
* ==> engine wrapper (enginethread) keeps a struct of configuration options with their string descriptors and everything, accessed by metagui for displaying, updating options done through events sent to the enginethread
* ==> seperate gamestate and guistate windows
  * can always change gamestate when user has perms for this in the current lobby (all perms given to everybody in local server play "offline")
  * can select from guistates compatible with the current gamestate
* ==> offline ai play:
  * engine has an option to enable auto search and auto move when certain players are playing
  * i.e. the engine always runs, but can be configured to only show hints when a certain player is playing
  * or configured to automatically start searching with timeout param, and also to automatically submit its move to the controller
* ==> engine compatibility
  * inbuilt engine works with everything
  * e.g. uci-engine is a wrapper for an executable that can be specified via an option
* ==> server concept
  * server is a class inside the project, can also be hosted locally or spun up locally for the network
  * supports guest login, but also user accounts
* ==> manage games with randomness (also works for long term hidden state)
  * DEPENDS ON server architecture (also for offline play)
  * lobby setting, where the server auto decides random moves if set to host mode
    * or if the game is just mirroring another actual game then any lobby mod can input the random moves
  * every user in the lobby can also be set to be an unknown (e.g. mirroring a real person we don't know state about), i.e. their state will not be decided by the system
    * all real users input their info (e.g. dealt cards) and then can use play and ai
* ==> games with simultaneous moves
  * game unions all possible moves from all outstanding players to move simultaneously, returns that as the valid_moves_list
  * when a player moves, their move is stored by the backend game into its accumulation buffer
    * when the last remaining player, of all those who move simultaneously, makes their move, the game processes the accumulation buffer and proceeds
* ==> manage player specific views for games with simultaneous moves or hidden information
  * frontend requires info on what view it should render, i.e. render only hidden info or move placer for the player of that view
  * auto switch view to player_to_move / next player if setting for that is given
* ==> resources like textures
  * can be generated on first launch into some local cache directory
  * not available for sounds?

---

main_ctrl should be a context object (low prio)
