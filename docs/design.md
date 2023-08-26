# mirabel API design

The mirabel project provides powerful APIs and utilities for creating all kinds of board games.  
All manner of games can be described by the general purpose game methods provided in [`game.h`](../includes/mirabel/game.h). This includes, but is not limited to, games containing:
* setup options
* randomness
* simultaneous moves
* hidden information
* legacies carrying over to future games and even attached to certain players
  * e.g. scores (reward based results) or evolving game materials
* teams
* (planned) custom and dynamic timecontrol stages and staging
* (planned) non-trivial draw/resign voting

To facilitate general usage with game agnostic tooling (e.g. visualization / AI) many of the more advanced features of the game methods API are guarded behind feature flags. Additionally the game methods can optionally be enriched with features aiding in automatic processing like state ids, board evals and further internal methods.  
For more information on the provided engine methods API see [`engine.h`](../includes/mirabel/engine.h).  
General purpose move histories for games adhering to the game methods API are provided in [`move_history.h`](../includes/mirabel/move_history.h).

## the game

### terminology
|term|description|example|
|---|---|---|
|random moves / randomness|Any type of event where the outcome is decided from a fixed set of results, by chance, offers random moves. This outcome may or may not be visible to all players.|Roll some dice in the open, or drawing a card from a shuffled deck.|
|simultaneous moves|Any state from which more than one player can move offers simultaneous moves. These moves may or may not be commutative.|Chosen actions are revealed by both players at once.|
|hidden information|Any game where there exists any valid state in which any information is conceiled from any set of players contains hidden information.|A deck of cards shuffled at the start of the game.||
|`player_id`|An 8-bit integer representing the id of a player participating in a game. For a game with N players, the ids are always numbered 1 to N. A maximum of 254 player ids can be assigned.||
|`PLAYER_NONE`|Special player id representing either none or all players. Never assigned to participants.||
|`PLAYER_ENV`|Special player id representing the environment. This player decides the outcome of random events and performs forcing moves on players when imposed by the environmental rules of the game.|The result of a coin flip is decided via a move by the environment, as is the showing action performed by an opponent when forcing them to show their hand cards in private.|
|move|Moves represent state transitions on the game board and its internal state. A move that encodes an action is part of exactly that action set.|Moves as the union of actions and concrete moves.|
|action|Actions represent sets of moves, i.e. sets of concrete moves (action instances / informed moves). Every action can be encoded as a move.|Draw *some* card from a hidden pile. Roll the die (irrespective of outcome).|
|concrete move|For every action, the concrete moves it encompasses determine the outcome of the action move. Every concrete move can be encoded as a move or reduced to an action, i.e. a move.|Roll a specific number with a die.|
|big move|A move that is larger than the 64-bit "small move" which is normally enough for most games. Use this when you require massive randomness or text inputs.|Choosing a natural language word in a word game.|
|options|Any information that the game requires to be available at time of creation, and which can not be changed for during the lifetime of the game. Options remain constant for all games of the same legacy, if applicable.|Board sizes, player counts.|
|state|A set of facts that represents the current setup of the "board". This is not a perfect representation of the game state history.|String equivalent of a screenshot of the board (and all players hands).|
|serialization|Comprehensive representation of the entire game as it exists. This includes options, legacy, state *and* any internal data that influences the game. That is, a deserialized game has to behave *exactly* like the one it was serialized from would.|Raw binary stream.|
|legacy|Carry over information from a previous game of this type.|Parts of a card deck get replaced/changed/added/removed over the course of multiple games. Point scores attached to players, supporting drop in/out play.|

### game methods

Goals served by the game methods:
* enforce game rules (illegal moves can not be played)
* provide uniform access to common game actions (e.g. get moves and make moves until game is done)
* ability to replay (physical) games with and without hidden information and randomness
* api designed for easy use with general purpose game playing AIs

A game is a struct owning its game specific data, and a pointer to the game methods used for interacting with it. It also hold general purpose information such as the sync_ctr for keeping games across multiple parties play-by-play compatible.

## examples

Examples for the more advanced features of the game methods api.

### RM+HM+SM basics

The following are short explanations of how randomness, hidden information and simultaneous moves are represented in *open* i.e. non-discretized games. Almost all games not directly managed by an engine are of this type, so it is the most useful to understand first.

#### randomness

Randomness can be initiated e.g. by a players move like "flip a coin" or simply by advancing game state. Whenever a random decision is required, e.g. directly after a player made the move choosing to flip a coin, the `PLAYER_ENV` and only that player is to move.  
The outcome of the random decision is then made by a move from `PLAYER_ENV` choosing among the available moves for them. (Usually a shared trusted server does this.) The chosen move is made and if applicable distributed to the servers clients, play proceeds as normal.  
For random chance to decide an outcome, the moves available are weighted with their concrete chance of happening.

#### simultaneous moves

In a game with simultaneous moves it is legal to output more than one player id from `players_to_move`.  
If not otherwise specified (by using the `sync_ctr` feature flag) games with simultaneous moves, just like all other games, are totally ordered in their moves. That is moves are never commutative, even if the game would allow for it.

When the `sync_ctr` feature flag is used, the wrapper does not handle rejection of wrongly synced moves automatically via `is_legal_move` anymore. The sync counter of the game does still increase automatically every time a move is played. A game that has states where moves are truly commutative, and wants to provide them as such, then has to manually test in the `is_legal_move` function, if the sync counter of the move in question is within such a range of commutative-ness.

#### hidden information

Hidden information arises quickly. Flipping a coin in private, drawing a card from a shuffled deck to a private hand instead of showing it publicly. Hidden information can also be transformed, for example laying a card from an already hidden hand facedown on the table. For many such actions accountability is important, which is why there always has to be at least one game instance which does know all of the state. Usually this is the shared trusted server.

The imbalance of information is managed via the `move_to_action` function. In rare cases a complex game might want to use `sync_data` for convenience, but it is still only just as powerful as `move_to_action` combined with big moves is.

In short: when a player makes a move that introduces or transforms randomness or hidden information, only the players that will know this hidden information gets an equivalent move from `move_to_action` all others will get a placeholder action informing them that something of a specific type happened, but not what exactly.  
For example flipping a coin in private: the `player_env` makes a move which indicates the result, the coin flipping player gets passed the identity (i.e. the result of the coin flip), the other player gets a generic move "hidden result". This works similarly for a player laying down a card facedown and other such actions.

When a player reveals hidden information, often the information to be revealed is itself in the move the player makes. That means in a simple card game a player would choose a card to play from their hidden hand, and make a move that details exactly what the card is they want to play, not for example the index of the card in their hand. That way if the move already contains all the revealed info, `move_to_action` passes the identity to all players, and there is no need for `sync_data` or any complex information revealing strategies.

If this is not possible or wanted, `sync_data` can be used to transfer the required reveal information from the players having this information, to the players that will require it.  
For example if the move is instead: play the 3rd card from the left from my hidden hand faceup. Then this move itself would not tell the other players what the card actually is. After a move is made on the server board, and **before** its `move_to_action` transformation is sent to the other clients, the board has the chance to offer any `sync_data` it wants to send to any sets of players of its choosing.  
In this way the board could send a `sync_data` packet to the other clients, informing them that the 3rd card from the left for the player to move is "XYZ". This is then sent directly together with the move saying this player will play their 3rd card to the left, the exact value of which the clients now know, just in time before the move is made.

### options

Some games require information to be set up which can not change, easily or at all, after the set up. Among others this includes: board sizes, player counts, draft pools for components and (variant) rules.  
On creation a game that supports the options feature may be passed an options string containing game specific information about the set up. To make the game exchangeable a created game that supports options must always be able to export the ones it is using to a string.

**Note**: Player count is not actually a part of the option string, but required at the same moment, so it is useful to treat it like a sort of universal option.

//TODO give all examples with open games first and make an extra thing for discretized games and their closed examples

//TODO rework for possible inconsistencies

### public randomness
Two modes of operation:
* Undiscretized (Open)
  * Random moves are decided by a move from PLAYER_ENV.
* Discretized (Closed)
  * Random move variety is eliminated because the game has a discretization seed. PLAYER_ENV still moves, but there is only one move left.

For replaying (physical) games, open games are used. Make the appropriate randomly selected result move as `PLAYER_ENV`.

#### example 1 (flip coin)
In this example a coin is flipped to decide which player goes first. However, the same working principle can be applied to all kinds of public randomness, like rolling dice at the start of a turn.

After creation the game is undiscretized by default. Because the coin flip is the first thing that happens in the game, `players_to_move` returns `{PLAYER_ENV}`. After discretization with a seed, all randomness is removed from the game, the results are then pre-determined.
* **Open**:
  Moves with `get_concrete_moves`: `{COIN_HEAD, COIN_TAIL}`.
* **Closed**:
  Moves with `get_concrete_moves`: `{COIN_TAIL}`.   (because the discretization collapsed to tail in this example).  
  Note that the random player still has to move, although only a single move is available for them. This assures that random events are still separated semantically from the rest of the gameplay. E.g. a possible frontend can separate and animate all moves.
When any move has been made the game proceeds as it normally would, using the move to determine which random result happened.

#### example 2 (roll dice)
In this example we assume a game where, among other things, a player may choose to roll a dice (the result of which is then somehow used in gameplay). We assume at least 1 player, with the id`P1`.

In both open and closed games we start with: `players_to_move` returns `{P1}` and `get_concrete_moves` returns `{MOVE_1, ..., ROLL_DIE, ..., MOVE_N}`.
When player `P1` makes the move `ROLL_DIE`, in both open and closed games, the next `players_to_move` are `{PLAYER_ENV}`. Just like the coin, the results for `get_concrete_moves` differ only slightly:
* **Open**: `{DIE_1, DIE_2, DIE_3, DIE_4, DIE_5, DIE_6}`
* **Closed**: `{DIE_4}`

### hidden randomness
This is the easiest form of hidden information.  
In this example a deck of cards is shuffled at the beginning of the game and players draw from it both faceup and facedown (into their hand).

//TODO need sync data events here too?, also explain sync data usage somewhere

#### example 1 (shuffle a deck)
Shuffling may work different ways, depending on the implementation, but is always representable by moves. Commonly the deck is either permuted in place (swap pairs of cards), or "drafted" from a start configuration to the shuffled deck (one by one).

Assuming N cards in the deck and a "draft" shuffle wherein the cards of the shuffled deck are selected one by one from an unshuffled initial deck. `PLAYER_ENV` moves N times.

**Open**  
Each time the available moves *include* all the remaining undrafted cards from the initial deck.  
*Additionally* at each choice point, there is also one more concrete move offered: `DRAFT_HIDDEN`.  
Drafting the hidden card semantically represents the *action* of the concrete moves drafting specific cards. I.e. drafting *any one* card but the spectator informing the game does not know which one was drafted.  
It is advisable to not make the digital shuffle implementation unneccessary complex, because if the game is replaying a "physical" game, then both shuffle methods should roughly match. Also, in an open game the shuffling process *may* be a mix between hidden drafts and known drafts. The game implementation *may* take advantage of this and track the possible state space for all players from there, but it does not have to, and could also just assume the deck as entirely unknown as soon as one hidden draft exists.

**Closed**  
Each time the only available move is the card pre-selected via the discretization seed. I.e. after making the only available move N times, the deck is "shuffled" according to the seed.  
If for example the closed board is a "server" board, that does not want to distribute the actually drafted cards to "clients", it only distributes the `move_to_action` result of the concrete draft move. I.e. it only distributes `DRAFT_HIDDEN` moves, because the action class of all concrete draft moves is `DRAFT_HIDDEN` the move for which does NOT show up in the `get_concrete_moves` list offered by the closed game.  
//TODO ideally want always applyable algo, e.g. where server ALWAYS distributes move->action actions

**In general** pre-shuffling a deck in this rarely advantageous, and most of the time it is easier to just delay the resolution of the randomness to a point where it is public. I.e. do not pre-shuffle the deck but just keep a knowledge of what cards it contains and then on draw resolve the randomness of which card was actually drawn. (See example 2.)

#### example 2 (draw card faceup from "shuffled" deck)
In this example we assume the easier case of drawing a card from a deck that was not actually shuffled card by card using moves, but is just represented by a list of all contained cards. On the draw action the result of the randomness if resolved.  
On the players turn `get_concrete_moves` shows the draw action as `{..., DRAW_FACEUP, ...}`. When the `DRAW_FACEUP` move is made `PLAYER_ENV` is to move, to select the outcome of the draw.

**Open**: `PLAYER_ENV` offers all cards as concrete moves, choose one by random or select one to replay from physical.  
**Closed**: `PLAYER_ENV` offers the one card that is determined to be draw by using the discretization seed.

//TODO want to example the pre-shuffled case as well? what would be the use case even?

#### example 3 (draw card facedown from "shuffled" deck)
This can also be rephrased to drawing the card from the *not* pre-shuffled deck into the players secret hand. Or even not revealing the card to any player and just placing it on the table for later reveal.  
In this example we assume a *not* pre-shuffled deck, i.e. lazy resolution, and players drawing from it to their secret hand by way of a `DRAW_FACEDOWN` move on their turn.  
//TODO
this creates hidden info as well  
player chooses action to draw facedown  
open: player rand offers all the cards available (on client that is all* minus some that could be card counted away, but thats optional ;; on server it shows all that can actually be drawn)
close: ??
?? how is the chosen info distributed to the player in belongs to? sync data?

#### example 4 (flip coin in secret)
//TODO want this?

### revealing hidden information (e.g. play card from secret hand)
A more complicated form of hidden information.  
//TODO
how the move is done changes how this behaviour would work:
- player chooses action to play card from hand (e.g. by idx in their hand) -> somehow need to sync back to the other boards WHAT card had been played
- player makes move that includes info about the card they are playing -> just send the move to other boards

### transforming hidden information (e.g. play card from secret hand *facedown*)
The most complicated form of hidden information.  
//TODO

### simultaneous moves
A relatively uncomplicated form of hidden information, but still with some important insights with regard to syncing information across players.  
ABC //TODO (with/without moved indicator for opponent)  
//TODO types (no)sync,unordered,ordered,want_discard,reissue,never_discard

#### example 1 (gated)
//TODO all SM players have to move before game continues, no interplay between moves

**Basic Idea**: The game returns multiple players to move, and for each player their available moves. If a player makes a move up to a "barrier" at which all players need to resolve then they are removed from the players to move until all other players have done so as well. E.g. everyone plays a card simultaneously: everyone is to move, once a card is played they are no longer to move and the game buffers their move, once all have moved the game resolves the effect "as if everyone moved at once" now that it knows all the moves. Addendun: if e.g. all players play two cards at once: a player is only removed from the players to move once they have chosen both cards.

#### example 2 (synced)
//TODO interplay not allowed for now b/c total move order

**Basic Idea**: The game returns multiple players to move, and for each player their available moves. If a player makes a move in a synced SM scenario, they are likely still to move, as are the other players, but the available moves for the other players might have changed. This is what the sync counter is used for. I.e. simultaneous moves not interpreted as "all move at once", but as "everyone can start moving at any time, but only one can move at a time".

### teams
This is not actually a feature in itself. Teams can be naturally represented using moves and the set structure of the results the game has after it is finished. Information sharing between team in a hidden information game is usually done by use of `sync_data` or the easier to use big moves in combination with an intelligent `move_to_action` function.

### legacy
For games using the legacy feature, two types of legacy are differentiated. The environment legacy, which may encompass things such as a shared world map or deck of cards that changes and retains its changed state from game to game. And the player legacies, which can be naturally used to represent all manor of different point scoring methods across games or just some player specific starting cards carried to the next game.  
To facilitate the use of legacy for point scoring the `s_get_legacy_results` game function takes in multiple legacies and determines a set of total winners.  
Player legacies stay attached to that player and are only passed to the game on creation, if that player participates. That means a theoretical three player card game could be played by four people. One rotating player sits out the current round while the other three play. This is a sort of non-tournament multi-game scoring.

Once a game with legacy is done, the perspectives for ALL players, including the `PLAYER_ENV` should be exported, because they will all each be needed when next passed into a game create.

### timecontrol
//TODO

### draw/resign
//TODO

## utilities
For implementation convenience feel free to use the general purpose `C` APIs provided in [`rosalia`](https://github.com/RememberOfLife/rosalia).
