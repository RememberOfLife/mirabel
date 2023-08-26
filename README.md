# mirabel

General purpose board game playing GUI and server with some useful features.
* Online/Offline Multiplayer (no account required)
  * Self-hostable Server
* Linux + Windows Builds (MSVC / MINGW at your choice)
* Engine Integration
* Plugin support for loading more games, frontends and engines.
  * Powerful API
  * Multithreaded asset loading.
  * Reuseable resources available.

Don't forget to clone submodules too by using:  
`git clone --recurse-submodules https://github.com/RememberOfLife/mirabel.git`

Future features:
* History Manager for game state tracking and analysis.
* Support for games using random moves, hidden information and simultaneous moves.

## dependencies
All dependencies marked `[system]` are system packages/dependencies from your distributions repositories, all others come pre-bundled.
* GLEW [system/release]
* SDL
* OpenGL [system]
* SDL_net
* OpenSSL [system]
* nanovg (+ stb)
* imgui
* rosalia

import blocks style:
* all standard libs
* standard libs, if any require special platform compatibility guards
* imports from dependencies in order as listed above
* imports from own src tree in source tree order (mirabel includes before src headers)
* import header for things implemented in this source file
