# Task2 绘制一个实心的圆

题目要求：

1. 绘制一个实心圆
2. 圆分成若干个扇形
3. 每个扇形使用不同颜色

附加题：

1. 如何让圆转起来

## 本题完成内容

- `main.cpp` 绘制了一个由 8 个彩色扇形组成的实心圆
- 每个扇形颜色不同，整体效果接近题图
- 已实现旋转动画
- 支持键盘开关旋转、调节转速、重置角度

## 运行方式

### macOS

```bash
cd LAB1/2
clang++ -std=c++11 -DGL_SILENCE_DEPRECATION main.cpp -framework OpenGL -framework GLUT -o main
./main
```

### Windows / MSYS2 + freeglut

```bash
g++ -std=c++11 -O2 -DFREEGLUT_STATIC -o main.exe main.cpp /ucrt64/lib/libfreeglut.a -lopengl32 -lglu32 -lgdi32 -lwinmm
main.exe
```

## 按键说明

- `Space`：开始或暂停旋转
- `+` / `=`：加快旋转
- `-` / `_`：减慢旋转
- `r` / `R`：重置角度和速度
- `Esc`：退出

## 实现思路

实心圆采用 `GL_TRIANGLE_FAN` 绘制。每个扇形都由圆心和相邻两条半径围成，因此可以按角度把整个圆均匀切分成多个区段，并为每个区段单独指定颜色。

## 附加题：如何让圆转起来？

方法是在绘制前对模型做旋转变换，并且周期性更新旋转角度。

程序中使用了两部分：

1. `glRotatef(rotationAngle, 0.0f, 0.0f, 1.0f);`
   这一步让圆绕 `z` 轴旋转
2. `glutTimerFunc(...)`
   定时更新 `rotationAngle`，再调用 `glutPostRedisplay()` 触发重绘

这就是红宝书中常见的动画基本思路：修改状态，再不断重绘。
