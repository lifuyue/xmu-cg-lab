# 实验6：Ray Tracing 光线跟踪

本目录实现实验 6 的 Whitted Style 光线跟踪填空任务，包含：

- `src/camera.hpp`：根据成像平面坐标生成从相机出发的光线。
- `src/raytracer.hpp`：实现阴影判断、镜面反射递归、折射递归。
- `src/ppm.hpp`：输出 `output.ppm`，便于 IrfanView 或其他图像查看器打开。

## 本地编译

需要 C++17 和 Eigen3。

```bash
g++ -std=c++17 -O2 -I/opt/homebrew/include/eigen3 src/exp6.cpp -o exp6_raytracing
./exp6_raytracing
```

运行后会在当前目录生成 `output.ppm`。

## GitHub Actions

仓库的 `.github/workflows/build-windows.yml` 会在 Windows runner 上编译：

- `exp6_raytracing.exe`
- `exp6_output.ppm`

两个文件会随 `windows-executables` artifact 一起上传。
