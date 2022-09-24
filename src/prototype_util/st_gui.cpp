#include <cstdint>
#include <cmath>

#include <SDL2/SDL.h>

#include "direct_draw.hpp"
#include "prototype_util/direct_draw.hpp"

#include "prototype_util/st_gui.hpp"
#include "st_gui.hpp"

namespace STGui {

    void btn_circ::update(float mX, float mY, uint32_t mouse_buttons)
    {
        is_hovered = false;
        is_clicked = false;
        if (hypot(x - mX, y - mY) <= r) {
            is_hovered = true;
            if (mouse_buttons & SDL_BUTTON_LMASK) {
                is_clicked = true;
            }
        }
    }

    void btn_circ::render()
    {
        if (is_clicked) {
            DD::SetRGBA255(color_clicked.r, color_clicked.g, color_clicked.b, color_clicked.a);
        } else if (is_hovered) {
            DD::SetRGBA255(color_hovered.r, color_hovered.g, color_hovered.b, color_clicked.a);
        } else {
            DD::SetRGBA255(color_base.r, color_base.g, color_base.b, color_clicked.a);
        }
        DD::SetFill();
        DD::DrawCircle(x, y, r);
    }

    void btn_rect::update(float mX, float mY, uint32_t mouse_buttons)
    {
        is_hovered = false;
        is_clicked = false;
        if (mX >= x && mX <= x + w && mY >= y && mY <= y + h) {
            is_hovered = true;
            if (mouse_buttons & SDL_BUTTON_LMASK) {
                is_clicked = true;
            }
        }
    }

    void btn_rect::render()
    {
        if (is_clicked) {
            DD::SetRGBA255(color_clicked.r, color_clicked.g, color_clicked.b, color_clicked.a);
        } else if (is_hovered) {
            DD::SetRGBA255(color_hovered.r, color_hovered.g, color_hovered.b, color_clicked.a);
        } else {
            DD::SetRGBA255(color_base.r, color_base.g, color_base.b, color_clicked.a);
        }
        DD::SetFill();
        DD::DrawRectangle(x, y, w, h);
    }

    void btn_hex::update(float mX, float mY, uint32_t mouse_buttons)
    {
        is_hovered = false;
        is_clicked = false;
        // if (hypot(x-mX, y-mY) > r) {
        //     return;
        // }
        //TODO proper hexagon collision check
        if (hypot(x - mX, y - mY) <= r) {
            is_hovered = true;
            if (mouse_buttons & SDL_BUTTON_LMASK) {
                is_clicked = true;
            }
        }
    }

    void btn_hex::render()
    {
        if (is_clicked) {
            DD::SetRGBA255(color_clicked.r, color_clicked.g, color_clicked.b, color_clicked.a);
        } else if (is_hovered) {
            DD::SetRGBA255(color_hovered.r, color_hovered.g, color_hovered.b, color_clicked.a);
        } else {
            DD::SetRGBA255(color_base.r, color_base.g, color_base.b, color_clicked.a);
        }
        DD::SetFill();
        DD::Push();
        DD::Translate(x, y);
        if (!flat_top) {
            DD::Rotate(DD::PI / 2);
        }
        DD::DrawRegularPolygon(6, 0, 0, r);
        DD::Pop();
    }

} // namespace STGui
