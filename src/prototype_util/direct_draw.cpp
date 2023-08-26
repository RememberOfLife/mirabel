#include <cmath>

#include <SDL_opengl.h>

#include "prototype_util/direct_draw.hpp"

namespace DD {

    void Push()
    {
        glPushMatrix();
    }

    void Pop()
    {
        glPopMatrix();
    }

    void Translate(float x, float y)
    {
        glTranslatef(x, y, 0);
    }

    void Rotate(float r)
    {
        glRotatef((r / PI) * 180, 0, 0, 1);
    }

    void SetRGB(float r, float g, float b)
    {
        glClearColor(r, g, b, 1);
        glColor3f(r, g, b);
    }

    void SetRGB255(int r, int g, int b)
    {
        glClearColor(
            static_cast<float>(r) / 255,
            static_cast<float>(g) / 255,
            static_cast<float>(b) / 255,
            1
        );
        glColor3ub(r, g, b);
    }

    void SetRGBA(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        glColor4f(r, g, b, a);
    }

    void SetRGBA255(int r, int g, int b, int a)
    {
        glClearColor(
            static_cast<float>(r) / 255,
            static_cast<float>(g) / 255,
            static_cast<float>(b) / 255,
            static_cast<float>(a) / 255
        );
        glColor4ub(r, g, b, a);
    }

    void SetLineWidth(float w)
    {
        glLineWidth(w);
        line_width = w;
    }

    void DrawCircle(float x, float y, float r)
    {
        DrawRegularPolygon(64, x, y, r);
    }

    void DrawLine(float x1, float y1, float x2, float y2)
    {
        float line_length = hypot(x1 - x2, y1 - y2);
        Push();
        Translate(x1, y1);
        Rotate(atan2(y2 - y1, x2 - x1));
        Translate(0, -line_width / 2);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(0, 0);
        glVertex2f(0, line_width);
        glVertex2f(line_length, 0);
        glVertex2f(line_length, line_width);
        glEnd();
        Pop();
    }

    void DrawRectangle(float x, float y, float w, float h)
    {
        if (fill) {
            glRectf(x, y, x + w, y + h);
        } else {
            Push();
            Translate(x - line_width / 2, y - line_width / 2);
            glBegin(GL_TRIANGLE_STRIP);
            glVertex2f(0, 0);
            glVertex2f(line_width, line_width);
            glVertex2f(w + line_width * 2, 0);
            glVertex2f(w + line_width, line_width);
            glVertex2f(w + line_width * 2, h + line_width * 2);
            glVertex2f(w + line_width, h + line_width);
            glVertex2f(0, h + line_width * 2);
            glVertex2f(line_width, h + line_width);
            glVertex2f(0, 0);
            glVertex2f(line_width, line_width);
            glEnd();
            Pop();
        }
    }

    void DrawRegularPolygon(int n, float x, float y, float r)
    {
        if (fill) {
            glBegin(GL_POLYGON);
            for (int i = 0; i < n; i++) {
                float angle = i * 2 * PI / n;
                glVertex2f(x + r * cos(angle), y + r * sin(angle));
            }
            glEnd();
        } else {
            float r_outer = r + line_width / 2;
            float r_inner = r - line_width / 2;
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= n; i++) {
                float angle = i * 2 * PI / n;
                glVertex2f(x + r_outer * cos(angle), y + r_outer * sin(angle));
                glVertex2f(x + r_inner * cos(angle), y + r_inner * sin(angle));
            }
            glEnd();
        }
    }

    void Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void SetStroke()
    {
        fill = false;
    }

    void SetFill()
    {
        fill = true;
    }

} // namespace DD
