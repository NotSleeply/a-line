# a-line CPU 光栅化实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 生成一张 128×128 的 PPM 图片，画出一条宽度 5.25、长度 52.5、倾斜 30°、圆头抗锯齿的直线。

**Architecture:** 单文件 C++ 程序，纯标准库无依赖。Vec2 做向量运算，Canvas 管理像素缓冲，distanceToSegment 求点到线段距离，drawLine 用 SSAA 4x 超采样抗锯齿。

**Tech Stack:** C++17，纯标准库（iostream/fstream/cmath/algorithm/vector）

---

## 文件结构

- `line.cpp` — 唯一源文件，包含所有代码

---

## Task 1: 编写 line.cpp

**Files:**
- Create: `line.cpp`

- [ ] **Step 1: 写入代码**

```cpp
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <vector>

struct Vec2 {
    double x, y;

    Vec2 operator+(const Vec2& other) const {
        return {x + other.x, y + other.y};
    }

    Vec2 operator-(const Vec2& other) const {
        return {x - other.x, y - other.y};
    }

    Vec2 operator*(double k) const {
        return {x * k, y * k};
    }
};

static double dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static double length(Vec2 v) {
    return std::sqrt(dot(v, v));
}

static double clamp(double x, double l, double r) {
    return std::max(l, std::min(x, r));
}

// 点到线段的最短距离
static double distanceToSegment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = b - a;
    Vec2 ap = p - a;
    double t = dot(ap, ab) / dot(ab, ab);
    t = clamp(t, 0.0, 1.0);
    Vec2 closest = a + ab * t;
    return length(p - closest);
}

class Canvas {
public:
    Canvas(int w, int h)
        : width_(w), height_(h), pixels_(w * h * 3, 255) {}

    void setPixel(int x, int y, unsigned char r,
                  unsigned char g, unsigned char b) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        int idx = (y * width_ + x) * 3;
        pixels_[idx + 0] = r;
        pixels_[idx + 1] = g;
        pixels_[idx + 2] = b;
    }

    void save(const char* filename) {
        std::ofstream out(filename, std::ios::binary);
        out << "P6\n" << width_ << " " << height_ << "\n255\n";
        out.write(reinterpret_cast<char*>(pixels_.data()), pixels_.size());
    }

private:
    int width_;
    int height_;
    std::vector<unsigned char> pixels_;
};

static unsigned char blend(unsigned char bg, unsigned char fg, double alpha) {
    return static_cast<unsigned char>(bg * (1.0 - alpha) + fg * alpha);
}

// SSAA 4x 超采样抗锯齿
static void drawLine(Canvas& canvas, Vec2 a, Vec2 b,
                     double width, unsigned char color) {
    int W = 128, H = 128;
    double radius = width / 2.0;

    // 包围盒
    int minX = static_cast<int>(std::floor(std::min(a.x, b.x) - radius));
    int maxX = static_cast<int>(std::ceil(std::max(a.x, b.x) + radius));
    int minY = static_cast<int>(std::floor(std::min(a.y, b.y) - radius));
    int maxY = static_cast<int>(std::ceil(std::max(a.y, b.y) + radius));

    minX = std::max(0, minX);
    maxX = std::min(W - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(H - 1, maxY);

    // 2x2 超采样
    const int S = 2;
    double subPixelSize = 1.0 / S;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double totalAlpha = 0.0;

            for (int sy = 0; sy < S; ++sy) {
                for (int sx = 0; sx < S; ++sx) {
                    double px = x + (sx + 0.5) * subPixelSize;
                    double py = y + (sy + 0.5) * subPixelSize;
                    Vec2 p{px, py};

                    double dist = distanceToSegment(p, a, b);

                    // 圆头：检查端点到像素的距离
                    double distA = length(p - a);
                    double distB = length(p - b);
                    double distMin = std::min({dist, distA, distB});

                    if (distMin < radius) {
                        totalAlpha += 1.0;
                    } else if (distMin < radius + subPixelSize) {
                        totalAlpha += (radius + subPixelSize - distMin) / subPixelSize;
                    }
                }
            }

            double alpha = totalAlpha / (S * S);
            unsigned char v = blend(255, color, alpha);
            canvas.setPixel(x, y, v, v, v);
        }
    }
}

int main() {
    const int W = 128;
    const int H = 128;

    const double lineLength = 52.5;
    const double lineWidth  = 5.25;
    const double angle      = 30.0 * M_PI / 180.0;

    Vec2 center{64.0, 64.0};

    Vec2 dir{
        std::cos(angle),
        -std::sin(angle)
    };

    Vec2 a = center - dir * (lineLength / 2.0);
    Vec2 b = center + dir * (lineLength / 2.0);

    Canvas canvas(W, H);
    drawLine(canvas, a, b, lineWidth, 0);
    canvas.save("line.ppm");

    std::cout << "生成完成：line.ppm\n";
    return 0;
}
```

- [ ] **Step 2: 提交**

```bash
git add line.cpp
git commit -m "feat: 实现 CPU 光栅化画线程序"
```

---

## Task 2: 编译并运行

**Files:**
- Modify: `line.cpp`（已在上一步创建）

- [ ] **Step 1: 编译**

Run: `g++ -std=c++17 -O2 -o line line.cpp`
Expected: 编译成功，无输出

- [ ] **Step 2: 运行**

Run: `./line`（或 `line.exe` on Windows）
Expected: 输出 `生成完成：line.ppm`

- [ ] **Step 3: 验证文件存在**

Run: `ls -la line.ppm`
Expected: 文件大小 > 0

- [ ] **Step 4: 提交最终代码**

```bash
git add line.cpp
git commit -m "feat: 添加完整实现并验证输出"
```

---

## 验收标准

- `line.ppm` 生成，大小约 128×128 像素
- 背景纯白、线条纯黑
- 倾斜角约 30°，线宽约 5 像素
- 圆头可见，边缘无明显锯齿

---

## 自检

1. **Spec 覆盖：** 所有参数（128×128、52.5、5.25、30°、圆头、抗锯齿）均在代码中体现
2. **Placeholder 检查：** 无 TODO/TBD/实现稍后等内容
3. **类型一致性：** Vec2 操作符、Canvas::setPixel、drawLine 参数类型前后一致