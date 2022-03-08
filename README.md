# mirabel

General purpose board game playing GUI.

## Future Features
* Engine Integration
* Online/Offline Multiplayer


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

networking wrapper:
https://github.com/libsdl-org/SDL_net
^ make sure this actually has tls, otherwise use another lib

## issues


## todo
* add fullscreen toggle to main menu bar
* chess frontend sounds
* chess frontend animations
* change window title according to the loaded game and frontend
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
* path to res folder shouldnt be hardcoded
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
* reassignable keyboardshortcuts
* offer some global color palette from the mirabel settings (can be edited there), every frontend may use this if it wants / if the user sets an option to do so
* some sort of general config/settings meta storage which the application uses to store saved preferences for each game frontend and general etc..
* some sort of async loading threadpool, where the gui can enqueue long operations which then get executed on another thread
  * after enqueuing the gui gets back a struct where it can see if e.g. the object has finished constructing yet
  * use this to parallelize loading/saving of all sorts of assets (images,sound,configs,etc..)
  * also frontendwrappers could cache constructe frontends and just return the cached one, this would keep settings per runtime and reduce loading times after first loading
* button for screenshots? unsure if this is a useful feature though
* create icon, show it on the empty (default) frontend

## problems
* change how the frontends receive the nanovg context, they need it in the constructor already
  * also, how to design some system that supports failed construction? i.e. if options were malformed, like a passed resourcepath that doesnt contain resources
  * also loading should be parallel to the guithread, can be integrated into the isready/stateok flag that frontends may expose to tell the guithread if they failed their construction?
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
  * when connecting to the offline server, use some sort of passthrough for the networkthreads so the messages directly reach the in/out queues of the correspondant
  * network adapter has outqueue for sending and inqueue pointer to the guithread inbox
  * guithread also needs to hold some connection state for online/offline and network latency + heartbeat etc..
* is the server a separate executable to the client?
  * would enable building and using the server with just surena as a dependency
* where to store state info for things like:
  * engine uci opts
  * engine best moves and other infor like nps etc..
  * player names and other multiplayer info that comes in from the networkthread
* what to do when engine is loaded but no game?
  * engine should not crash, just return garbage
* localization
* frontend should be able to easily change the cursor
* file paths should have some sort of file manager menu
  * https://codereview.stackexchange.com/questions/194553/c-dear-imgui-file-browser
* how to notify players if the other party has made a move in a correspondence game?
  * maybe users can supply a webhook?
* how to manage teams in games, i.e. multiple people playing for a shared reward?
* how are guithread state variables exposed? e.g. where to store fullscreen and frame_work_time such that it can be read from the outside?

### integration workflow
* ==> offline ai play:
  * engine has an option to enable auto search and auto move when certain players are playing
  * i.e. the engine always runs, but can be configured to only show hints when a certain player is playing
  * or configured to automatically start searching with timeout param, and also to automatically submit its move to the controller
* ==> server concept
  * server is a class inside the project, can also be hosted locally or spun up locally for the network
  * supports guest login, but also user accounts
  * whole history rating
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
  * a lobby has a trusted/untrusted state, depending on if the game is loaded from a player or only stored in full on the server
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
