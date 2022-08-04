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
* imports from own src tree in source tree order (mirabel includes before src headers)
* import header for things implemented in this source file

## resources

Some frontends may require loading resources, e.g. textures and sounds. These resources are placed in the `res/` folder.
* General purpose fonts, textures and sounds go into `res/fonts/{fontname}/`, `res/textures/` and `res/sounds/` respectively.
* Game specific resources go into `res/games/{gamename}/`. Multiple frontends may share these game specific resources.
* Frontend specific resources go into `res/frontends/{frontendname}/`.

All plugins are folder contained.

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
  * coins (multiple sounds)
* textures / icons
  * general purpose icon set, coin(s) and other money representations and magic icons and secrets (e.g. oath) etc..? (icons via go?)
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
* usability: when connecting to an invalid host address/port combination the connecting timeout can be ridiculously long
* graphics: when havannah game ends by network the hovered tile does not reset, probably goes for other games too
* security: when the event struct is sent over the network, uninitialized padding bytes are sent too, leaks info
* network: if we try to send data on a client connection that just closed, segfault
  * could send an event to the send queue to make it deconstruct and release a closed connection, just as in the network client the sendqueue should be the only one editing that info
  * in both client and server watch out that the recv client isnt using the sock while send queue deconstructs it
* bug: can not select among multiple frontends, try with fallback text
* switching from a frontend that once had a game running to another game crashes the frontend by set_game, not realiably reproducable so far

## todo
* unify resource storage, likely resource repo
* docking imgui windows in top or left breaks x and y for drawing
* game config window display a move list somewhere?
* when starting a game, start the default frontend for it automatically, i.e. last used if multiple
* probably drop description etc from frontend and games?
* config get an extra config folger (not res!), also, save metagui windows
* (create) use and send surena game sync counter
* add option to use different imgui font
* closing the network adapter should be asynchronous, we send it a shutdown event, it sends us back when its ready for collection / joining
  * closing status line in the connection window
* there is a lot of reuse in the networking code, maybe reduce it through some event methods
* add option to disable timeoutcrash thread for debugging (maybe disable by default, look at diy signal handlers)
* server should be seperate executable, use temp server lib for building both client and server, make sure server runs headless
  * server already has dependencies on code that also does graphics, i.e. the game catalogue also serves imgui game configs
* network client should try reconnecting itself on lost connection, cache access params for that
* use proper directory where the binary is located to infer default resource paths, ofc should also be passable as a config
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

## ideas
* global overlay notifications, (top-right, centered (e.g. mc title), sticky, progress..)
  * want notifications IN mirabel, or should mirabel issue to the environment notification service?
* need some unit tests for things like move_history, config_registry etc?
* general purpose job queue for background processes
* draw and resign are events
  * player offering draw may set timeout, can not be taken back, on timeout it auto expires
* maybe replace SDL_net with another cpp raw networking lib (https://github.com/SLikeSoft/SLikeNet) so we dont have to download opengl on a server just for it
  * for now just try to spoof sdl for sdl net lib with some fake sdl functions it can use
  * or just copy all the files into this project along with the license
  * required features:
    * ipv6, timeout on hostname resolution, checksockets works on errored out sockets with infinite timeout
* lobbies for hidden info / random move games could provide functionality to make their outcomes and playout provably fair
  * https://crypto.stackexchange.com/questions/99927/provably-fair-card-deck-used-by-client-and-server
* maybe put default metagui shortcuts somewhere else, this way frontends arent blocked as much from using ctrl
* server supports a single lobby mode
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
  * does this also just serve as a general command menu?
* reassignable keyboardshortcuts
* offer some global color palette from the mirabel settings (can be edited there), every frontend may use this if it wants / if the user sets an option to do so
  * global dark theme toggle
* some sort of general config/settings meta storage which the application uses to store saved preferences for each game frontend and general etc..
* some sort of async loading threadpool, where the gui can enqueue long operations which then get executed on another thread
  * after enqueuing the gui gets back a struct where it can see if e.g. the object has finished constructing yet
  * use this to parallelize loading/saving of all sorts of assets (images,sound,configs,etc..)
  * also frontendwrappers could cache constructe frontends and just return the cached one, this would keep settings per runtime and reduce loading times after first loading
* button for screenshots
  * definitely need a feature to export focused graphics from games so commentators/analysts can compose game "diagrams"
* create icon, show it on the empty (default) frontend
* combobox for gamevariant board implementation (e.g. bitboards, havannah eval persistent storage, etc..)
* semver for all the components
* ability to play a plugin game without the server loading it, by just forwarding everything to the trusted host player
* frontend should be able to easily change the cursor, offer some util at least locally

## problems
* history manager should only set the guithread state, client will still use newest one for networking, except if user distributed to network?
  * on which state should the engine search, main line or selected state
  * in general what is the network interaction for the history manager
* local docs / game rule window, per variant? images/graphic representations?
  * load rules from res?
* localization
* file paths should have some sort of file manager menu
  * https://codereview.stackexchange.com/questions/194553/c-dear-imgui-file-browser
* how to notify players if the other party has made a move in a correspondence game?
  * maybe users can supply a webhook?
* how to manage teams in games, i.e. multiple people playing for a shared reward?
* how are guithread state variables exposed? e.g. where to store fullscreen and frame_work_time such that it can be read from the outside?

### integration workflow
* ==> tripcodes
  * hiddenfeature, add '#' to the username e.g. "guestname#138754"
    * add this to the string filter etc..
    * sets password for guest name
    * can also work as coupon mechanism for account creation
  * or just dont do it at all
  * authn event is a valid response to auth info, i.e. the server has automatically logged in the client as assigned guest
* ==> offline ai play:
  * engine has an option to enable auto search and auto move when certain players are playing
  * i.e. the engine always runs, but can be configured to only show hints when a certain player is playing
  * or configured to automatically start searching with timeout param, and also to automatically submit its move to the controller
* ==> server concept
  * whole history rating
* ==> manage games with randomness (also works for long term hidden state)
  * DEPENDS ON server architecture (also for offline play)
  * lobby setting, where the server auto decides random moves if set to host mode
    * or if the game is just mirroring another actual game then any lobby mod can input the random moves
  * every user in the lobby can also be set to be an unknown (e.g. mirroring a real person we don't know state about), i.e. their state will not be decided by the system
    * all real users input their info (e.g. dealt cards) and then can use play and ai
* ==> resources like textures
  * can be generated on first launch into some local cache directory
  * not available for sounds?
* ==> lobby logic
  * can always change gamestate when user has perms for this in the current lobby (all perms given to everybody in local server play "offline")
* ==> animation within frontends?
  * how to smooth the animation for self played moves, i.e. when the user drops a drag-and-drop piece onto its target square
    * normally the move_event is pushed AFTER the inbox event_queue is processed (in the sdl event queue for inputs)
    * that way the piece will be reset for one frame until it is processed in the next one
    * FIX: do what lichess does, just dont animate drag-and-drop pieces and only animate pinned and enemy moves
* ==> networking structure for offline/online server play
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
