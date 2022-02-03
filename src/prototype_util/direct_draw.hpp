#pragma once

#include <cstdint>
#include <math.h>

namespace DD {

    struct Color4i {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    struct vec2 {
        float x;
        float y;
    };

    // 2D only

    const float PI = M_PI;

    static bool fill = false;
    static float line_width = 1;

    // only works the projection matrix
    void Push();
    void Pop();

    void Translate(float x, float y);
    void Rotate(float r);

    void SetRGB(float r, float g, float b);
    void SetRGB255(int r, int g, int b);
    void SetRGBA(float r, float g, float b, float a);
    void SetRGBA255(int r, int g, int b, int a);

    void SetLineWidth(float w);

    void DrawCircle(float x, float y, float r);
    void DrawLine(float x1, float y1, float x2, float y2);
    void DrawRectangle(float x, float y, float w, float h);
    void DrawRegularPolygon(int n, float x, float y, float r);

    void Clear();
    void SetStroke();
    void SetFill();

    //TODO MeasureString
    //TODO DrawStringAnchored

    /*
    DrawPoint(x, y, r float64)
    DrawLine(x1, y1, x2, y2 float64)
    DrawRectangle(x, y, w, h float64)
    DrawRoundedRectangle(x, y, w, h, r float64)
    DrawCircle(x, y, r float64)
    DrawArc(x, y, r, angle1, angle2 float64)
    DrawEllipse(x, y, rx, ry float64)
    DrawEllipticalArc(x, y, rx, ry, angle1, angle2 float64)
    DrawRegularPolygon(n int, x, y, r, rotation float64)
    DrawImage(im image.Image, x, y int)
    DrawImageAnchored(im image.Image, x, y int, ax, ay float64)
    SetPixel(x, y int)

    MoveTo(x, y float64)
    LineTo(x, y float64)
    QuadraticTo(x1, y1, x2, y2 float64)
    CubicTo(x1, y1, x2, y2, x3, y3 float64)
    ClosePath()
    ClearPath()
    NewSubPath()

    Clear()
    Stroke()
    Fill()
    StrokePreserve()
    FillPreserve()

    DrawString(s string, x, y float64)
    DrawStringAnchored(s string, x, y, ax, ay float64)
    DrawStringWrapped(s string, x, y, ax, ay, width, lineSpacing float64, align Align)
    MeasureString(s string) (w, h float64)
    MeasureMultilineString(s string, lineSpacing float64) (w, h float64)
    WordWrap(s string, w float64) []string
    SetFontFace(fontFace font.Face)
    LoadFontFace(path string, points float64) error

    Clip()
    ClipPreserve()
    ResetClip()
    AsMask() *image.Alpha
    SetMask(mask *image.Alpha)
    InvertMask()
    */

}
