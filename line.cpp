#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <memory>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>

using namespace std;

const double PI = 3.14159265358979323846;

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

    void blendPixel(int x, int y, unsigned char color, double alpha)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;
        int idx = (y * width + x) * 3;
        pixels[idx] = static_cast<unsigned char>(pixels[idx] * (1.0 - alpha) + color * alpha);
        pixels[idx + 1] = static_cast<unsigned char>(pixels[idx + 1] * (1.0 - alpha) + color * alpha);
        pixels[idx + 2] = static_cast<unsigned char>(pixels[idx + 2] * (1.0 - alpha) + color * alpha);
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

// Interface for Polymorphic Line Renderers
class LineRenderer
{
public:
    virtual ~LineRenderer() = default;
    virtual string getName() const = 0;
    virtual void render(Canvas &canvas, Vec2 a, Vec2 b, double width, unsigned char color) = 0;
};

// 1. Classical Bresenham Line Drawing (1px, integer-only step logic, no AA)
class BresenhamRenderer : public LineRenderer
{
public:
    string getName() const override { return "Bresenham (1px, No AA)"; }

    void render(Canvas &canvas, Vec2 a, Vec2 b, double /*width*/, unsigned char color) override
    {
        int x0 = static_cast<int>(round(a.x));
        int y0 = static_cast<int>(round(a.y));
        int x1 = static_cast<int>(round(b.x));
        int y1 = static_cast<int>(round(b.y));

        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true)
        {
            canvas.setPixel(x0, y0, color, color, color);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }
};

// 2. Xiaolin Wu's Line Drawing (1px, subpixel precision with linear AA)
class XiaolinWuRenderer : public LineRenderer
{
public:
    string getName() const override { return "Xiaolin Wu (1px, linear AA)"; }

    void render(Canvas &canvas, Vec2 a, Vec2 b, double /*width*/, unsigned char color) override
    {
        auto ipart = [](double x) { return floor(x); };
        auto roundVal = [](double x) { return round(x); };
        auto fpart = [](double x) { return x - floor(x); };
        auto rfpart = [&](double x) { return 1.0 - fpart(x); };

        double x0 = a.x, y0 = a.y;
        double x1 = b.x, y1 = b.y;

        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep)
        {
            swap(x0, y0);
            swap(x1, y1);
        }
        if (x0 > x1)
        {
            swap(x0, x1);
            swap(y0, y1);
        }

        double dx = x1 - x0;
        double dy = y1 - y0;
        double gradient = (dx == 0.0) ? 1.0 : dy / dx;

        // First endpoint
        double xend = roundVal(x0);
        double yend = y0 + gradient * (xend - x0);
        double xgap = rfpart(x0 + 0.5);
        int xpxl1 = static_cast<int>(xend);
        int ypxl1 = static_cast<int>(ipart(yend));

        auto plot = [&](int x, int y, double c) {
            if (steep)
                canvas.blendPixel(y, x, color, c);
            else
                canvas.blendPixel(x, y, color, c);
        };

        plot(xpxl1, ypxl1, rfpart(yend) * xgap);
        plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
        double intery = yend + gradient;

        // Second endpoint
        xend = roundVal(x1);
        yend = y1 + gradient * (xend - x1);
        xgap = fpart(x1 + 0.5);
        int xpxl2 = static_cast<int>(xend);
        int ypxl2 = static_cast<int>(ipart(yend));
        plot(xpxl2, ypxl2, rfpart(yend) * xgap);
        plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);

        // Main loop
        for (int x = xpxl1 + 1; x < xpxl2; ++x)
        {
            plot(x, ipart(intery), rfpart(intery));
            plot(x, ipart(intery) + 1, fpart(intery));
            intery += gradient;
        }
    }
};

// 3. Classical Bresenham Sweep-Circle-Brush Line Drawing (Thick Line, no AA)
class BresenhamThickRenderer : public LineRenderer
{
private:
    void drawFilledCircle(Canvas &canvas, int cx, int cy, int radius, unsigned char color)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            for (int x = -radius; x <= radius; ++x)
            {
                if (x * x + y * y <= radius * radius)
                {
                    canvas.setPixel(cx + x, cy + y, color, color, color);
                }
            }
        }
    }

public:
    string getName() const override { return "Bresenham Thick (Circle Brush)"; }

    void render(Canvas &canvas, Vec2 a, Vec2 b, double width, unsigned char color) override
    {
        int radius = static_cast<int>(round(width / 2.0));
        int x0 = static_cast<int>(round(a.x));
        int y0 = static_cast<int>(round(a.y));
        int x1 = static_cast<int>(round(b.x));
        int y1 = static_cast<int>(round(b.y));

        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true)
        {
            drawFilledCircle(canvas, x0, y0, radius, color);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }
};

// 4. Signed Distance Field (SDF) Capsule drawing with SSAA (Super-Sampling Anti-Aliasing)
template <int S = 2>
class SdfSsaaRenderer : public LineRenderer
{
public:
    string getName() const override
    {
        return "SDF Capsule SSAA " + to_string(S) + "x" + to_string(S);
    }

    void render(Canvas &canvas, Vec2 a, Vec2 b, double width, unsigned char color) override
    {
        double radius = width / 2.0;
        double subPixelSize = 1.0 / S;

        double minX = min(a.x, b.x) - radius - 1.0;
        double maxX = max(a.x, b.x) + radius + 1.0;
        double minY = min(a.y, b.y) - radius - 1.0;
        double maxY = max(a.y, b.y) + radius + 1.0;

        int startX = max(0, static_cast<int>(floor(minX)));
        int endX = min(canvas.width - 1, static_cast<int>(ceil(maxX)));
        int startY = max(0, static_cast<int>(floor(minY)));
        int endY = min(canvas.height - 1, static_cast<int>(ceil(maxY)));

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

                        double dist = distanceToSegment(p, a, b);

                        if (dist < radius)
                        {
                            totalAlpha++;
                        }
                        else if (dist < radius + subPixelSize)
                        {
                            totalAlpha += (radius + subPixelSize - dist) / subPixelSize;
                        }
                    }
                }

                if (totalAlpha > 0)
                {
                    double alpha = static_cast<double>(totalAlpha) / (S * S);
                    canvas.blendPixel(px, py, color, alpha);
                }
            }
        }
    }
};

// 5. SDF Capsule drawing with Analytical AA (No SSAA loop, smoothstep math on distance)
class SdfAnalyticalRenderer : public LineRenderer
{
public:
    string getName() const override { return "SDF Capsule Analytical AA (Smoothstep)"; }

    void render(Canvas &canvas, Vec2 a, Vec2 b, double width, unsigned char color) override
    {
        double radius = width / 2.0;

        double minX = min(a.x, b.x) - radius - 1.0;
        double maxX = max(a.x, b.x) + radius + 1.0;
        double minY = min(a.y, b.y) - radius - 1.0;
        double maxY = max(a.y, b.y) + radius + 1.0;

        int startX = max(0, static_cast<int>(floor(minX)));
        int endX = min(canvas.width - 1, static_cast<int>(ceil(maxX)));
        int startY = max(0, static_cast<int>(floor(minY)));
        int endY = min(canvas.height - 1, static_cast<int>(ceil(maxY)));

        for (int y = startY; y <= endY; ++y)
        {
            for (int x = startX; x <= endX; ++x)
            {
                Vec2 p(x + 0.5, y + 0.5);
                double d = distanceToSegment(p, a, b);

                // Analytical AA: transition region of 1.0 pixel
                double edge0 = radius - 0.5;
                double edge1 = radius + 0.5;
                double t = clamp((d - edge0) / (edge1 - edge0), 0.0, 1.0);
                double alpha = 1.0 - (t * t * (3.0 - 2.0 * t)); // smoothstep interpolation

                if (alpha > 0.0)
                {
                    canvas.blendPixel(x, y, color, alpha);
                }
            }
        }
    }
};

// 6. GPU-style Triangulated Rasterization (Capsule subdivided into a rectangle + two caps)
class GpuRasterRenderer : public LineRenderer
{
private:
    struct CoverageBuffer
    {
        int width, height;
        int S;
        vector<uint8_t> grid;

        CoverageBuffer(int w, int h, int s)
            : width(w), height(h), S(s), grid(w * s * h * s, 0) {}

        void setSubPixel(int sx, int sy)
        {
            if (sx < 0 || sx >= width * S || sy < 0 || sy >= height * S)
                return;
            grid[sy * width * S + sx] = 1;
        }

        bool getSubPixel(int sx, int sy) const
        {
            if (sx < 0 || sx >= width * S || sy < 0 || sy >= height * S)
                return false;
            return grid[sy * width * S + sx] != 0;
        }
    };

    void rasterizeTriangleToCoverage(CoverageBuffer &cov, Vec2 p0, Vec2 p1, Vec2 p2)
    {
        int S = cov.S;
        // Transform vertices to subpixel coordinates
        Vec2 s0(p0.x * S, p0.y * S);
        Vec2 s1(p1.x * S, p1.y * S);
        Vec2 s2(p2.x * S, p2.y * S);

        double minX = min({s0.x, s1.x, s2.x});
        double maxX = max({s0.x, s1.x, s2.x});
        double minY = min({s0.y, s1.y, s2.y});
        double maxY = max({s0.y, s1.y, s2.y});

        int startX = max(0, static_cast<int>(floor(minX)));
        int endX = min(cov.width * S - 1, static_cast<int>(ceil(maxX)));
        int startY = max(0, static_cast<int>(floor(minY)));
        int endY = min(cov.height * S - 1, static_cast<int>(ceil(maxY)));

        auto isInside = [](double px, double py, Vec2 v0, Vec2 v1, Vec2 v2) {
            double w0 = (px - v1.x) * (v2.y - v1.y) - (py - v1.y) * (v2.x - v1.x);
            double w1 = (px - v2.x) * (v0.y - v2.y) - (py - v2.y) * (v0.x - v2.x);
            double w2 = (px - v0.x) * (v1.y - v0.y) - (py - v0.y) * (v1.x - v0.x);
            return (w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0);
        };

        for (int y = startY; y <= endY; ++y)
        {
            for (int x = startX; x <= endX; ++x)
            {
                double px = x + 0.5;
                double py = y + 0.5;
                if (isInside(px, py, s0, s1, s2))
                {
                    cov.setSubPixel(x, y);
                }
            }
        }
    }

    void resolveCoverage(Canvas &canvas, const CoverageBuffer &cov, unsigned char color)
    {
        int S = cov.S;
        for (int y = 0; y < canvas.height; ++y)
        {
            for (int x = 0; x < canvas.width; ++x)
            {
                int hits = 0;
                for (int sy = 0; sy < S; ++sy)
                {
                    for (int sx = 0; sx < S; ++sx)
                    {
                        if (cov.getSubPixel(x * S + sx, y * S + sy))
                        {
                            hits++;
                        }
                    }
                }
                if (hits > 0)
                {
                    double alpha = static_cast<double>(hits) / (S * S);
                    canvas.blendPixel(x, y, color, alpha);
                }
            }
        }
    }

public:
    string getName() const override { return "GPU-style Triangulated Rasterizer (4x4 MSAA)"; }

    void render(Canvas &canvas, Vec2 a, Vec2 b, double width, unsigned char color) override
    {
        double radius = width / 2.0;
        Vec2 ab = b - a;
        double len = length(ab);
        if (len == 0.0)
            return;

        Vec2 u = ab * (1.0 / len);
        Vec2 n(-u.y, u.x);

        // Rectangle vertices
        Vec2 v0 = a + n * radius;
        Vec2 v1 = a - n * radius;
        Vec2 v2 = b - n * radius;
        Vec2 v3 = b + n * radius;

        int S = 4; // 4x4 subpixel grid
        CoverageBuffer cov(canvas.width, canvas.height, S);

        // Main line body
        rasterizeTriangleToCoverage(cov, v0, v1, v2);
        rasterizeTriangleToCoverage(cov, v0, v2, v3);

        // Subdivided circle caps (fans)
        const int N = 16;
        double theta = atan2(u.y, u.x);

        // Start cap (facing backwards)
        Vec2 prevVertexA = a + Vec2(cos(theta + PI / 2.0), sin(theta + PI / 2.0)) * radius;
        for (int i = 1; i <= N; ++i)
        {
            double angle = theta + PI / 2.0 + (PI * i) / N;
            Vec2 currVertexA = a + Vec2(cos(angle), sin(angle)) * radius;
            rasterizeTriangleToCoverage(cov, a, prevVertexA, currVertexA);
            prevVertexA = currVertexA;
        }

        // End cap (facing forwards)
        Vec2 prevVertexB = b + Vec2(cos(theta - PI / 2.0), sin(theta - PI / 2.0)) * radius;
        for (int i = 1; i <= N; ++i)
        {
            double angle = theta - PI / 2.0 + (PI * i) / N;
            Vec2 currVertexB = b + Vec2(cos(angle), sin(angle)) * radius;
            rasterizeTriangleToCoverage(cov, b, prevVertexB, currVertexB);
            prevVertexB = currVertexB;
        }

        resolveCoverage(canvas, cov, color);
    }
};

// 7. Multi-threaded CPU Parallel Rendered SDF Capsule (splits canvas into row ranges)
template <int S = 4>
class ParallelSdfSsaaRenderer : public LineRenderer
{
public:
    string getName() const override
    {
        return "SDF Capsule Parallel SSAA " + to_string(S) + "x" + to_string(S) + " (std::thread)";
    }

    void render(Canvas &canvas, Vec2 a, Vec2 b, double width, unsigned char color) override
    {
        double radius = width / 2.0;
        double subPixelSize = 1.0 / S;

        double minX = min(a.x, b.x) - radius - 1.0;
        double maxX = max(a.x, b.x) + radius + 1.0;
        double minY = min(a.y, b.y) - radius - 1.0;
        double maxY = max(a.y, b.y) + radius + 1.0;

        int startX = max(0, static_cast<int>(floor(minX)));
        int endX = min(canvas.width - 1, static_cast<int>(ceil(maxX)));
        int startY = max(0, static_cast<int>(floor(minY)));
        int endY = min(canvas.height - 1, static_cast<int>(ceil(maxY)));

        int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0)
            numThreads = 4;

        vector<thread> threads;
        int totalRows = endY - startY + 1;
        if (totalRows <= 0)
            return;

        int rowsPerThread = (totalRows + numThreads - 1) / numThreads;

        for (int t = 0; t < numThreads; ++t)
        {
            int threadStartY = startY + t * rowsPerThread;
            int threadEndY = min(endY, threadStartY + rowsPerThread - 1);
            if (threadStartY > endY)
                break;

            threads.emplace_back([&canvas, a, b, radius, subPixelSize, startX, endX, threadStartY, threadEndY, color]() {
                for (int py = threadStartY; py <= threadEndY; ++py)
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

                                double dist = distanceToSegment(p, a, b);

                                if (dist < radius)
                                {
                                    totalAlpha++;
                                }
                                else if (dist < radius + subPixelSize)
                                {
                                    totalAlpha += (radius + subPixelSize - dist) / subPixelSize;
                                }
                            }
                        }

                        if (totalAlpha > 0)
                        {
                            double alpha = static_cast<double>(totalAlpha) / (S * S);
                            canvas.blendPixel(px, py, color, alpha);
                        }
                    }
                }
            });
        }

        for (auto &th : threads)
        {
            th.join();
        }
    }
};

int main()
{
    const int W = 128, H = 128;
    Vec2 center(64.0, 64.0);
    double angle = 60.0 * PI / 180.0;
    Vec2 dir(cos(angle), -sin(angle));
    double lineLen = 52.5;
    Vec2 a = center - dir * (lineLen / 2.0);
    Vec2 b = center + dir * (lineLen / 2.0);
    double thickness = 5.25;

    struct RendererJob
    {
        unique_ptr<LineRenderer> renderer;
        string filename;
    };

    vector<RendererJob> jobs;
    jobs.push_back({make_unique<BresenhamRenderer>(), "line_bresenham.ppm"});
    jobs.push_back({make_unique<XiaolinWuRenderer>(), "line_xiaolin_wu.ppm"});
    jobs.push_back({make_unique<BresenhamThickRenderer>(), "line_bresenham_thick.ppm"});
    jobs.push_back({make_unique<SdfSsaaRenderer<2>>(), "line_sdf_ssaa_2x2.ppm"});
    jobs.push_back({make_unique<SdfSsaaRenderer<4>>(), "line_sdf_ssaa_4x4.ppm"});
    jobs.push_back({make_unique<SdfAnalyticalRenderer>(), "line_sdf_analytical.ppm"});
    jobs.push_back({make_unique<GpuRasterRenderer>(), "line_gpu_raster.ppm"});
    jobs.push_back({make_unique<ParallelSdfSsaaRenderer<4>>(), "line_sdf_parallel_4x4.ppm"});

    cout << "=========================================================================\n";
    cout << "           C++ Line Drawing Multi-Technology Showcase & Benchmark        \n";
    cout << "=========================================================================\n";
    cout << left << setw(48) << "Renderer Method" << setw(15) << "Time (us)" << "Output File\n";
    cout << "-------------------------------------------------------------------------\n";

    for (auto &job : jobs)
    {
        Canvas canvas(W, H);

        auto start = chrono::high_resolution_clock::now();
        job.renderer->render(canvas, a, b, thickness, 0);
        auto end = chrono::high_resolution_clock::now();

        auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
        double microsecs = duration / 1000.0;

        canvas.save(job.filename.c_str());

        cout << left << setw(48) << job.renderer->getName()
             << setw(15) << fixed << setprecision(2) << microsecs
             << job.filename << "\n";
    }

    // Default fallback to line.ppm using 4x4 Parallel SDF Renderer for max quality
    {
        Canvas canvas(W, H);
        ParallelSdfSsaaRenderer<4> defaultRenderer;
        defaultRenderer.render(canvas, a, b, thickness, 0);
        canvas.save("line.ppm");
        cout << "-------------------------------------------------------------------------\n";
        cout << "Default fallback saved as line.ppm\n";
    }

    cout << "=========================================================================\n";
    return 0;
}