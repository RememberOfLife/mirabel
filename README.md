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
* how are options withing the gamestate config window passed?

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

---

frontends/chess
class chess  {
    bool supportsgamevariant(game_variant* gv) {if instanceof<chessstandard> || instanceof<chess960variant>}  
}

main_ctrl should be a context object (low prio)
