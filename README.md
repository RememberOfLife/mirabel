# mirabel

General purpose board game playing GUI and server with some useful features.
* Online/Offline Multiplayer
* Engine Integration
* Windows Builds

Future features will include:
* Support for games using random moves, hidden information and simultaneous moves.
* Plugin support for loading more games, frontends and engines.

## dependencies

* SDL (+ OpenGL)
* SDL_net
* OpenSSL
* nanovg (+ stb + GLEW)
* imgui
* surena

import blocks style:
* all standard libs
* imports from dependencies in order as listed above
* imports from own src tree in source tree order
* import header for things implemented in this source file

## resources

Some frontends may require loading resources, e.g. textures and sounds. These resources are placed in the `res/` folder.
* General purpose fonts, textures and sounds go into `res/fonts/{fontname}/`, `res/textures/` and `res/sounds/` respectively.
* Game specific resources go into `res/games/{gamename}/`. Multiple frontends may share these game specific resources.
* Frontend specific resources go into `res/frontends/{frontendname}/`.

### todos
Collect more general resources:
* sounds
  * card pickup
  * card placement
  * card shuffle light
  * card shuffle heavy
  * pencil on paper, one scratch
  * pencil on paper, multiple scratches
  * "wood/stone" piece placement
  * "wood/stone" piece capture
  * game start beep
  * game end jingle
  * low on time beep
* textures / icons
* usual gameboard colors (these will actually be placed in the config for the global palette in the future):
  * rgb(201, 144, 73) "wood" normal
  * rgb(240, 217, 181) "wood" light
  * rgb(161, 119, 67) "wood" dark
  * rgb(236, 236, 236) white
  * rgb(210, 210, 210) white accent
  * rgb(11, 11, 11) black
  * rgb(25, 25, 25) black accent
  * rgb(199, 36, 73) neon red/pink
  * rgb(37, 190, 223) neon cyan
  * rgb(120, 25, 25) dark red or use rgb(141, 35, 35)
  * rgb(24, 38, 120) dark blue

## issues
* security: incoming packets from the user on the server need to be sanitized
  * e.g. currently user can make server run out of memory and even just ouright force exit it
* security/performance: a network adapter expecting more overhang data, can be tricked into listening infinitely on a dead socket
  * e.g. send an event containing 1000B of raw data (but actually only send 1024B), server reads 1024B, tries to listen for 8B overhang and gets stuck
* usability: when connecting to an invalid host address/port combination the connecting timeout can be ridiculously long
* usability: all graphical scalings (imgui+frontends) are way too small on high resolution displays
* graphics: when havannah game ends by network the hovered tile does not reset, probably goes for other games too

## todo
* put connection+chat window into the main menu bar
* use data structs for events using the event.raw_data, then give them all a constructor from the pointer to the raw data
  * i.e. the game load struct builds itself from the raw data, and outputs pointers into it for the names and options
* fix EVENT_TYPE_GAME_LOAD to encode options for remote loading
  * append some game specific options struct behind the game base+variant names, give pointer to this struct to the new_game() function
* server should be seperate executable, use temp server lib for building both client and server, make sure server runs headless
  * server already has dependencies on code that also does graphics, i.e. the game catalogue also serves imgui game configs
* there is a lot of reuse in the networking code, maybe reduce it through some event methods
* openssl for networking
* make watchdog work with arbitrary queues and proper cond var
  * let there be one watchdog thread that knows multiple queues registered to it?
  * or should every object that wants a watchdog create its own watchdog runner?
* make event queue a proper producer-consumer semaphore
* any way to differentiate between closed connections from error or proper closing? i.e. recv always gives 0 no matter what, can't read server NOK
* network client should try reconnecting itself on lost connection, cache access params for that
* better ai integration
* add fullscreen toggle to main menu bar
* rework everything to use nanovg and remove DD, it is no longer required
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
* place stb in deps? also genrally decide if we'd rather link everything static or dynamically?

## ideas
* maybe put default metagui shortcuts somewhere else, this way frontends arent blocked as much from using ctrl
* ??? is the offline server even required? events are mirrored and the client holds all state for itself aswell anyway
* server supports a single lobby mode, this could also act as the offline server
* clientconfig and serverconfig struct to hold things like the palette and configurable settings + their defaults
* logging wrapper functions for the server so that the offline server logs into the corresponding metagui logger, but the standalone one logs to stdout
  * do debug log statements get a macro? feels like they bloat performance at runtime otherwise
  * should log level be variable at runtime?
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
  * global dark theme toggle
* some sort of general config/settings meta storage which the application uses to store saved preferences for each game frontend and general etc..
* some sort of async loading threadpool, where the gui can enqueue long operations which then get executed on another thread
  * after enqueuing the gui gets back a struct where it can see if e.g. the object has finished constructing yet
  * use this to parallelize loading/saving of all sorts of assets (images,sound,configs,etc..)
  * also frontendwrappers could cache constructe frontends and just return the cached one, this would keep settings per runtime and reduce loading times after first loading
* button for screenshots? unsure if this is a useful feature though
* create icon, show it on the empty (default) frontend
* combobox for gamevariant board implementation (e.g. bitboards, havannah eval persistent storage, etc..)
* semver for all the components

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
  * can start with views for e.g. chess
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
    * FIX: do what lichess does, just dont animate drag-and-drop pieces and only animate pinned and enemy moves
* ==> networking structure for offline/online server play
  * guithread also needs to hold some connection state for online/offline and network latency + heartbeat etc..
  * networkadapter is different for client vs server:
    * client holds only the server connection socket
      * update last_interaction_time every time socket action happens, if it didnt happen for some time, send heartbeat with appropriate timeout for closing
    * server holds a potentially very large list of sockets that have connected to it
      * when connections cant be increased to fit more users, remove inactive connections, then remove guests (but only if new conn is user), then refuse
      * how to handle server side heartbeat? i.e. how to notice if a client has timed out
        * hold a linked list of all client_connections and on incoming socket action update the last_interaction_time and move it to the back
        * then every once in a while we only need to check from the front of the list to get the most stale connections and potentially cull them
      * if required, the socket sets and their connections could also be multithreaded (e.g. 1 for server sock and workers for every N client socks)
    * offline adapter passthrough
      * client/server has a sendqueue, set to networkadapters sendqueue, or client/server in offline passthrough
    * all events contain all the necessary info, e.g. lobby ids etc
    * ==> when scheduling work from network events into server worker threads keep track who got which lobby the last time and give it to them the next time agains, makes for better caching
