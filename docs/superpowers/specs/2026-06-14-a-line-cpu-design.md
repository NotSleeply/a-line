# a-line CPU 光栅化设计

## 目标

用纯 C++ 实现 CPU 光栅化，画一条满足以下条件的直线：

- 宽度：5.25
- 长度：52.5
- 倾斜角：30°
- 圆头
- 抗锯齿
- 输出为 .ppm 格式
- 不依赖任何第三方库

## 输出文件

- `line.ppm` — 最终图片，纯二进制 PPM 格式

## 核心参数（固定）

| 参数 | 值 |
|---|---|
| 图片尺寸 | 128×128 |
| 线长 | 52.5 |
| 线宽 | 5.25 |
| 倾斜角 | 30°（逆时针为正方向） |
| 前景色 | 黑色 (0,0,0) |
| 背景色 | 白色 (255,255,255) |

## 架构

### 数据结构

**Vec2**
- 字段：`double x, y`
- 操作：加减、点乘、标量乘、长度

**Canvas**
- 内部 buffer：`std::vector<unsigned char>`，RGB 三通道
- 接口：
  - `Canvas(int w, int h)` — 构造时填白色
  - `setPixel(int x, int y, unsigned char r/g/b)`
  - `getPixel(int x, int y)` — 返回 RGB 三元组，超出边界返回白色
- 超采样时按 2×2 子像素平均值填入最终 buffer

### 关键函数

**distanceToSegment(Vec2 p, Vec2 a, Vec2 b)**
- 求点 p 到线段 ab 的最短距离
- 公式：投影参数 t = clamp(dot(ap, ab) / dot(ab, ab), 0, 1)，最近点 = a + ab * t

**drawLine(Canvas&, Vec2 a, Vec2 b, double width, unsigned char color)**
- 线段两个端点 a、b，宽度 width（宽度含圆头）
- 遍历包围盒内所有像素
- 对每个像素分 2×2 超采样，分别求子像素中心点到胶囊体的距离
- 根据距离决定灰度值（边缘抗锯齿）
- 调用 Canvas::setPixel 写入

**main()**
1. 创建 `Canvas canvas(128, 128)`
2. 计算线段端点：以图片中心 (64, 64) 为中点，沿 30° 方向偏移 ±lineLength/2
3. 调用 `drawLine(canvas, a, b, 5.25, 0)`
4. `canvas.save("line.ppm")`

## 抗锯齿策略

采用 **超采样 (SSAA 4x)**：

- 最终像素拆成 2×2 共 4 个子像素
- 每个子像素独立判断是否在胶囊体内
- 取 4 个灰度值的平均值作为最终像素灰度
- 距离恰好在边缘时自然过渡，无需额外公式

## 胶囊体判断逻辑

线段 + 圆头统一抽象为"到线段 ab 的距离"：

- 矩形线身：像素到线段的垂直距离
- 圆头：端点为圆心、半径为 width/2 的圆
- 两者均通过 `distanceToSegment` 隐式覆盖（圆头退化为半径无穷小的端点时自然形成）

## 验收标准

生成 `line.ppm` 文件，满足：

1. 尺寸 128×128
2. 背景纯白，线条纯黑
3. 倾斜角约 30°
4. 线宽约 5.25 像素（目测）
5. 圆头清晰可见
6. 边缘无明显锯齿