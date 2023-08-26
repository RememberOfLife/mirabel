# surena

General purpose board game backend and some AI to go with it.
* Some [supported](#support) games out of the box
* Use game [plugins](#plugins) to play arbitrary games


## plugins
The surena project provides powerful APIs and utilities for creating all kinds of board games.  
For more details regarding the various APIs available, see the [design](./docs/design.md) doc.

### todos
* rework design doc for recent api changes and the final hidden info api currently in use
  * also go through game.h comment doc and make sure it all still fits
* use rerrorf in the game impls to clarify errors, and free it
* move history, add plugin functionality for e.g. (universal -> pgn -> universal)
  * want similar translation layer for (de)serialization?
* use readline in main https://stackoverflow.com/questions/2600528/c-readline-function
* add uci wrapper engine for general purpose chess engines (same for tak)
* document workflow for non perfect information games
* stop clang-format newline after structs unions etc.. if already within a struct, union, class, function!
* fill more feature flags for the example games provided in this project
