#pragma once

#include <cstdint>

#include <SDL.h>

#include "prototype_util/direct_draw.hpp"

namespace STGui {

    class GuiElement {

      public:

        virtual ~GuiElement() = default;

        // SDL event gets recursively sent down the gui tree
        // used for detecting gui button presses via a mouse event
        //   on btn click, a new SDL event is pushed to the queue, which will get processed by the application
        // also used for updating gui elements via commands
        virtual void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) = 0;

        //TODO needs update method?

        // recursively position this element and all its children
        // called on every resizing of the root element; //TODO should be only called on elements that have requested layouting
        // the rectangle size given defines the possible space this element may occupy
        //   children of this element will get the same, or a subset, space to position themselves in
        virtual void layout(float x, float y, float w, float h) = 0;

        // recursively render this element and all its children
        virtual void render() = 0;
    };

    /*

    http://www.cmyr.net/blog/gui-framework-ingredients.html
    https://linebender.org/druid/widget.html
    http://www.cmyr.net/blog/druid-dynamism.html

    workflow for interaction
        - sdl mouse events
            - instead of static position and click mask
            - use for checking if button has been pressed
        - sdl user event with custom code
            - use for button inputs
            - button pushes its update event into the queue, with a button input event data struct that holds info about what click happened
            - (OTHER) or give the button a callback that it calls on click, also fine
        - create some gui command struct which gets sent down the gui tree, just like an sdl event would
            - if an element sees its id as the target element in the cmd struct then it reads it and updates itself accordingly
    */

    //TODO is the current layout call idea workable?

    //TODO elements:
    // alignments left,middle,right,top,bot,center
    /*
        - just needs a horizontal and a vertical grid
        - texture element
        - margin elemt so child elements can somehow overstep the space boundaries set by their parents
        - also, z position of chess pieces vs e.g. engine eval bar ?
        - does a button keep 4 colors for dead,normal,hover,click? or is the color updated via a command?
    */

    class ButtonCirc : public GuiElement {
      public:

        ButtonCirc();
        void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) override;
        void layout(float x, float y, float w, float h) override;
        void render() override;
    };

    class ButtonRect : public GuiElement {
      public:

        ButtonRect();
        void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) override;
        void layout(float x, float y, float w, float h) override;
        void render() override;
    };

    class ButtonHex : public GuiElement {
      public:

        ButtonHex();
        void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) override;
        void layout(float x, float y, float w, float h) override;
        void render() override;
    };

    class Padding : public GuiElement {
      public:

        Padding(float u);
        Padding(float h, float v);
        Padding(float l, float t, float r, float b);
        void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) override;
        void layout(float x, float y, float w, float h) override;
        void render() override;
    };

    class Panel : public GuiElement {
      public:

        Panel();
        void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) override;
        void layout(float x, float y, float w, float h) override;
        void render() override;
    };

    class Label : public GuiElement {
      public:

        Label();
        void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) override;
        void layout(float x, float y, float w, float h) override;
        void render() override;
    };

    class Grid : public GuiElement {
      public:

        Grid();
        void process_event(SDL_Event event, bool blockMouse, bool blockKeyboard) override;
        void layout(float x, float y, float w, float h) override;
        void render() override;
    };

    struct btn_circ {
        float x;
        float y;
        float r;
        DD::Color4i color_base;
        DD::Color4i color_hovered;
        DD::Color4i color_clicked;
        bool is_hovered;
        bool is_clicked;
        void update(float mX, float mY, uint32_t mouse_buttons);
        void render();
    };

    struct btn_rect {
        float x;
        float y;
        float w;
        float h;
        DD::Color4i color_base;
        DD::Color4i color_hovered;
        DD::Color4i color_clicked;
        bool is_hovered;
        bool is_clicked;
        void update(float mX, float mY, uint32_t mouse_buttons);
        void render();
    };

    struct btn_hex {
        float x;
        float y;
        float r;
        bool flat_top;
        DD::Color4i color_base;
        DD::Color4i color_hovered;
        DD::Color4i color_clicked;
        bool is_hovered;
        bool is_clicked;
        void update(float mX, float mY, uint32_t mouse_buttons);
        void render();
    };

} // namespace STGui
