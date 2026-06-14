#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdint>

using namespace std;

struct Vec2
{
    double x, y;
    Vec2() : x(0), y(0) {}
    Vec2(double x_, double y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2 &other) const
    {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2 operator-(const Vec2 &other) const
    {
        return Vec2(x - other.x, y - other.y);
    }

    Vec2 operator*(double s) const
    {
        return Vec2(x * s, y * s);
    }
};

double dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

double length(Vec2 v)
{
    return sqrt(dot(v, v));
}

double clamp(double v, double lo, double hi)
{
    return max(lo, min(hi, v));
}

double distanceToSegment(Vec2 p, Vec2 a, Vec2 b)
{
    Vec2 ab = b - a;
    Vec2 ap = p - a;
    double t = dot(ap, ab) / dot(ab, ab);
    t = clamp(t, 0.0, 1.0);
    Vec2 closest = a + ab * t;
    return length(p - closest);
}

class Canvas
{
public:
    int width, height;
    vector<uint8_t> pixels;

    Canvas(int w, int h) : width(w), height(h), pixels(w * h * 3, 255) {}

    void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;
        int idx = (y * width + x) * 3;
        pixels[idx] = r;
        pixels[idx + 1] = g;
        pixels[idx + 2] = b;
    }

    void save(const char *filename)
    {
        ofstream f(filename, ios::binary);
        f << "P6\n"
          << width << " " << height << "\n255\n";
        f.write(reinterpret_cast<char *>(pixels.data()), pixels.size());
        f.close();
    }
};

unsigned char blend(unsigned char bg, unsigned char fg, double alpha)
{
    return static_cast<unsigned char>(bg * (1 - alpha) + fg * alpha);
}

void drawLine(Canvas &canvas, Vec2 a, Vec2 b, double width, unsigned char color)
{
    const int S = 2;
    double halfWidth = width / 2.0;
    double subPixelSize = 1.0 / S;

    double minX = min(a.x, b.x) - halfWidth - 1;
    double maxX = max(a.x, b.x) + halfWidth + 1;
    double minY = min(a.y, b.y) - halfWidth - 1;
    double maxY = max(a.y, b.y) + halfWidth + 1;

    int startX = static_cast<int>(floor(minX));
    int endX = static_cast<int>(ceil(maxX));
    int startY = static_cast<int>(floor(minY));
    int endY = static_cast<int>(ceil(maxY));

    startX = max(0, startX);
    endX = min(canvas.width - 1, endX);
    startY = max(0, startY);
    endY = min(canvas.height - 1, endY);

    for (int py = startY; py <= endY; ++py)
    {
        for (int px = startX; px <= endX; ++px)
        {
            int totalAlpha = 0;

            for (int sy = 0; sy < S; ++sy)
            {
                for (int sx = 0; sx < S; ++sx)
                {
                    double subX = px + (sx + 0.5) / S;
                    double subY = py + (sy + 0.5) / S;
                    Vec2 p(subX, subY);

                    double distSeg = distanceToSegment(p, a, b);
                    double distA = length(p - a);
                    double distB = length(p - b);
                    double dist = min(distSeg, min(distA, distB));

                    if (dist < halfWidth)
                    {
                        totalAlpha++;
                    }
                    else if (dist < halfWidth + subPixelSize)
                    {
                        totalAlpha += (halfWidth + subPixelSize - dist) / subPixelSize;
                    }
                }
            }

            if (totalAlpha > 0)
            {
                double alpha = static_cast<double>(totalAlpha) / (S * S);
                int idx = (py * canvas.width + px) * 3;
                unsigned char bg = canvas.pixels[idx];
                unsigned char v = blend(bg, color, alpha);
                canvas.pixels[idx] = v;
                canvas.pixels[idx + 1] = v;
                canvas.pixels[idx + 2] = v;
            }
        }
    }
}

int main()
{
    const int W = 128, H = 128;
    Vec2 center(64.0, 64.0);
    double angle = 30.0 * 3.14159265358979323846 / 180.0;
    Vec2 dir(cos(angle), -sin(angle));
    double lineLen = 52.5;
    Vec2 a = center - dir * (lineLen / 2.0);
    Vec2 b = center + dir * (lineLen / 2.0);

    Canvas canvas(W, H);
    drawLine(canvas, a, b, 5.25, 0);

    canvas.save("line.ppm");
    cout << "生成完成：line.ppm" << endl;

    return 0;
}