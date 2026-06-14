# a-line

## 背景

来源与知乎提问的回答：[想用 C++ 写一个小项目，有什么好的创意？](https://www.zhihu.com/question/275291023)

### 回答： [韦易笑](https://www.zhihu.com/question/275291023/answer/1994723085987882225)

用代码画一条直线：
画图软件最基础的一件事情，画得出来你可以 piss off 99% 的客户端程序员要求很简单，
正确绘制一条宽度为 5.25 长度位 52.5 的倾斜 30 度的一条直线，圆头抗锯齿，
包括 CPU 方法和 GPU 方法两种实现，不依赖第三方库：

![a](public/a.jpg)

效果如上图。

PS：可以用 MSAA/SSAA，但不用的话加分（实际项目里做这个的确不会用）。

## 思路

一条“有宽度的线”本质上不是线，而是一个**胶囊形状**：

```text
圆头 + 矩形线身 + 圆头
```

你可以把它看成：

```text
以线段为中心轴，半径为 width / 2 的区域
```

所以问题变成：

对于图片里的每个像素，判断它离这条线段有多远。

如果距离小于半径，像素应该被涂黑。

如果距离刚好在边缘附近，就根据距离给一个透明度，实现抗锯齿。

---

CPU 版本可以这样做：

1. 创建一张白色图片，比如 128×128。
2. 确定线段的起点和终点。
3. 遍历所有像素。
4. 计算当前像素中心点到线段的最短距离。
5. 根据距离决定像素颜色。
6. 输出成 `.ppm` 图片，不依赖任何第三方库。

关键公式是：

```cpp
// 点 p 到线段 ab 的距离
Vec2 ab = b - a;
Vec2 ap = p - a;

double t = dot(ap, ab) / dot(ab, ab);
t = clamp(t, 0.0, 1.0);

Vec2 closest = a + ab * t;
double dist = length(p - closest);
```

然后：

```cpp
double radius = width / 2.0;

if (dist <= radius) {
    // 在线里面
}
```

抗锯齿可以用：

```cpp
double alpha = clamp(radius + 0.5 - dist, 0.0, 1.0);
```

`alpha` 越接近 1，越黑；越接近 0，越白。

---

一个最小可运行版本如下，输出 `line.ppm`：

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

double dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

double length(Vec2 v) {
    return std::sqrt(dot(v, v));
}

double clamp(double x, double l, double r) {
    return std::max(l, std::min(x, r));
}

double distanceToSegment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = b - a;
    Vec2 ap = p - a;

    double t = dot(ap, ab) / dot(ab, ab);
    t = clamp(t, 0.0, 1.0);

    Vec2 closest = a + ab * t;
    return length(p - closest);
}

int main() {
    const int W = 128;
    const int H = 128;

    const double lineLength = 52.5;
    const double lineWidth = 5.25;
    const double angle = 30.0 * M_PI / 180.0;

    Vec2 center = {64.0, 64.0};

    Vec2 dir = {
        std::cos(angle),
        -std::sin(angle)
    };

    Vec2 a = center - dir * (lineLength / 2.0);
    Vec2 b = center + dir * (lineLength / 2.0);

    double radius = lineWidth / 2.0;

    std::vector<unsigned char> image(W * H * 3, 255);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Vec2 p = {x + 0.5, y + 0.5};

            double dist = distanceToSegment(p, a, b);

            double alpha = clamp(radius + 0.5 - dist, 0.0, 1.0);

            unsigned char value = static_cast<unsigned char>(255 * (1.0 - alpha));

            int idx = (y * W + x) * 3;
            image[idx + 0] = value;
            image[idx + 1] = value;
            image[idx + 2] = value;
        }
    }

    std::ofstream out("line.ppm", std::ios::binary);
    out << "P6\n" << W << " " << H << "\n255\n";
    out.write(reinterpret_cast<char*>(image.data()), image.size());

    std::cout << "生成完成：line.ppm\n";
    return 0;
}
```

---

这个项目适合大一做，但建议分阶段：

第一阶段：输出一张纯色 `.ppm` 图片。

第二阶段：画普通直线，不考虑宽度。

第三阶段：画有宽度的线。

第四阶段：加圆头。

第五阶段：加抗锯齿。

第六阶段：封装成一个小型 2D 渲染器，比如：

```cpp
Canvas canvas(128, 128);
canvas.clear(255, 255, 255);
canvas.drawLine({20, 100}, {100, 20}, 5.25);
canvas.save("output.ppm");
```

如果你能把这个项目写清楚，确实比做一个普通的学生管理系统更能锻炼 C++、数学、图形学和工程封装能力。

## 快速启动

```sh
g++ line.cpp -o line -O2
./line
```
