# Task3 绘制一个奥运五环

题目要求：

1. 绘制一个奥运五环
2. 形状和颜色与示意图保持一致
3. 颜色重叠部分需要正确表现遮挡关系

附加题：

1. 拖动窗口大小时，如何让五环不变形

## 本题完成内容

- `main.cpp` 绘制了蓝、黑、红、黄、绿五个环
- 使用分段圆弧模拟前后遮挡关系，使重叠部分更接近题图
- 已处理窗口缩放，拖动窗口大小时五环保持比例不变形

## 运行方式

### macOS

```bash
cd LAB1/3
clang++ -std=c++11 -DGL_SILENCE_DEPRECATION main.cpp -framework OpenGL -framework GLUT -o main
./main
```

### Windows / MSYS2 + freeglut

```bash
g++ -std=c++11 -O2 -DFREEGLUT_STATIC -o main.exe main.cpp /ucrt64/lib/libfreeglut.a -lopengl32 -lglu32 -lgdi32 -lwinmm
main.exe
```

## 实现思路

五环主体使用粗线宽圆弧绘制。为了表现前后遮挡，不是简单地一次画完整圆，而是：

1. 先画出五个完整圆环
2. 在需要被遮挡的位置用白色圆弧做局部覆盖
3. 再把应该显示在前面的那段彩色圆弧重新画出来

这样可以得到更接近示意图的环套效果。

## 附加题：如何让五环在拖动窗口大小时不变形？

关键是 `reshape(int width, int height)` 回调函数。

程序中做了两件事：

1. 使用 `glViewport(0, 0, width, height)` 让绘图区始终覆盖整个窗口
2. 根据窗口宽高比动态调整 `glOrtho(...)` 的左右或上下范围

这样无论窗口变宽还是变高，坐标系都会按比例扩展，圆仍然保持为圆，不会被拉伸成椭圆。
