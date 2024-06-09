# mirabel rv2


## plan

build options:
* debug / release
* web (its ok if building for web only works on linux) / native (linux, mingw, msvc)

* [x] super basic opengl sdl app running on both native and emscripten
* [x] cmake integration to specify build type
* [ ] release builds for both, debug build for native, debug build for web is just O2 or O1 and sets the debug macro for log debugging?
  * [ ] add checkable debug macro
* [x] integrate imgui docking
* test various integrations, BOTH web and native flawlessly
  * [x] nanovg
  * [ ] proper html shell file
    * [ ] proper loading indicator while web engine is being loaded
    * [ ] only loaded on user click, i.e. press the activation button
      * get params: "MW-run" and "MW-run!"
      * [ ] can be overwritten using a get param
        * [ ] but then requires one click to activate to enable sound playing
          * [ ] this too can be ignored by upgrading the get param
    * [x] unified command line arguments passing
      * [ ] and a way to set the browser line, from within application
    * [ ] singlefile embedded html ready to ship and download on someones pc
  * [x] open websocket client (to echosocket)
  * [ ] pthread
    * [ ] for event based threads like networking
    * [ ] for long running looping threads that dont yield to the browser
  * [x] load shared library easily for native and web
    * [ ] make sure it has a loading bar!
    * [ ] downloaded through websocket inside of wasm, or outside and then offered? or separate thread inside?
  * [ ] user opens file
  * [ ] user saves file
  * [ ] access to browser local storage as persistent config?
  * [ ] cookieable? i.e. maybe somehow keep a users state?
  * [ ] work through deploy guide
    * [ ] handle zooming
    * [ ] handle errors, and oom
    * [ ] devicepixelratio for highdpi devices
    * [x] resizing browser window resizes window
    * [ ] fullscreen mode
  * [ ] performance: why is sokol so fast and we aren't?

* [ ] proper dev environment sets the correct macros and enables clang auto complete by compile commands, at least for native!

* [ ] mirabel html log on the right side and auto open on crash, also button from metagui log window, also button to close the html log
* [ ] web log text select needs to be cancelable


### infos
* make for emscripten: `emcmake cmake ..` and `emmake make -j`
* browser will only allow wss (and block ws) if page is https
  * so, need to steal site cert and name record from site to local machine?
  * or host on server properly, but that all kindof breaks quick simple hosting without security
* native SDL also builds SDL-test for some reason, disable that, we don't need it
* consider main module = 2 for dead code elimination in the future
* consider forcing stdlibs into main module EMCC_FORCE_STDLIBS since they may be missing for plugins that want them


### roadmap

* weniger god objekte

* try web threading, test websockets on threads
* look at: ocornut imgui testing framework
* grundstruktur client server
  * offline netmgr passthrough
* message passing client server
* lobby + session?
* state wrapper game + frontend

