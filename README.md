# mirabel

General purpose board game playing GUI and server with some useful features.
* Online/Offline Multiplayer (no account required)
  * Self-hostable Server
* Linux + Web + Windows Builds (MSVC / MINGW at your choice)
* Plugin support for loading games, frontends, engines and application-interfaces.
  * Powerful API, games as rule engines.
  <!-- * Multithreaded asset loading. -->
  <!-- * Reuseable resources available. -->

Don't forget to clone submodules too by using:  
`git clone --recurse-submodules https://github.com/RememberOfLife/mirabel.git`

Future core features:
* History Manager for game state tracking and analysis.
* REPL for cli game playing, server management and testing.
* Engine Integration (needs to be updated to reflect newer project developments)

## usage

<!-- TODO provide mirabel web host: Web Client: [mirabel]()   -->
Native Client: `mirabel`  
Native Client (GL+imgui): `mirabel`  
<!-- Native Client (readline repl): `mirabel crl`   -->
<!-- TODO proper server syntax, own target or cli arg?: Server: `mirabel server`   -->
<!-- TODO: Test Suite -->

## plugins

The mirabel project provides powerful APIs and utilities for creating all kinds of board games.  
For more details regarding the various APIs available, see the [`mirabel/game.h`](./includes/mirabel/game.h) API [design](./docs/game_api_design.md) document.

## dependencies

All dependencies marked `[system]` are system packages/dependencies from your distributions repositories, all others come pre-bundled.
<!-- TODO where does libwebsocket, mysql, sqlite3 fit later on -->
* emscripten [system] (if used)
* GLEW [system/release]
* SDL
* OpenGL [system]
* SDL_net
* OpenSSL [system]
* nanovg (+ stb)
* imgui
* crossline
* rosalia

import blocks style:
* all standard libs
* standard libs, if any require special platform compatibility guards
* imports from dependencies in order as listed above
* imports from own src tree in source tree order (mirabel includes before src headers)
* import header for things implemented in this source file
