# Task1 运行提供的示例程序

建议阅读资料：

1. 《OpenGL 编程基础》第 1、2 章

## 本题完成内容

1. 编写并运行了一个基于 `freeglut` 的示例 OpenGL 程序
2. 程序可在窗口中绘制一个彩色三角形
3. 注册了窗口缩放回调，观察窗口大小变化时视口和投影的更新
4. 附加提供了一个教材风格的矩形示例程序，便于继续练习

## 文件说明

- `main.cpp`：本题主程序，建议优先运行
- `extra_example.cpp`：额外示例，可作为“尝试理解、运行其他 OpenGL 程序”的补充

## 运行方式

### macOS

```bash
cd LAB1/1
clang++ -std=c++11 -DGL_SILENCE_DEPRECATION main.cpp -framework OpenGL -framework GLUT -o main
./main
```

额外示例：

```bash
cd LAB1/1
clang++ -std=c++11 -DGL_SILENCE_DEPRECATION extra_example.cpp -framework OpenGL -framework GLUT -o extra_example
./extra_example
```

### Windows / MSYS2 + freeglut

```bash
g++ -std=c++11 -O2 -DFREEGLUT_STATIC -o main.exe main.cpp /ucrt64/lib/libfreeglut.a -lopengl32 -lglu32 -lgdi32 -lwinmm
main.exe
```

## 对问题（2）的回答

### 用鼠标改变窗口大小会发生什么？

拖动窗口边缘时，GLUT 会触发窗口重绘。当前程序里三角形会始终保持正确的显示范围，并尽量保持比例不失真；窗口变宽或变高时，左右或上下可视范围会随之调整。

如果没有处理窗口缩放，一般会出现两类问题：

1. 图形被拉伸、压缩
2. 只在窗口的一部分区域内绘制，剩余区域空白

### 哪个函数在影响整个过程？

核心是 `reshape(int width, int height)` 回调函数。它通过下面两步影响缩放过程：

1. `glViewport(...)`：决定 OpenGL 把图像画到窗口的哪一块区域
2. `glOrtho(...)`：重新设置投影范围，控制场景在新窗口尺寸下如何显示

程序中通过 `glutReshapeFunc(reshape);` 注册该函数，因此窗口大小变化时会自动调用它。

## 对问题（3）的补充

`extra_example.cpp` 提供了一个更接近教材入门风格的矩形绘制程序。它同样使用了 `display`、`reshape`、`init` 这几个最基本的 OpenGL/GLUT 结构，便于和 `main.cpp` 对照理解。
