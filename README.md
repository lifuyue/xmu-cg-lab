# xmu-cg-lab
你🦐的计算机图形学实验

## 当前目录说明

- `LAB0/`：旧版实验内容归档
- `LAB1/`：新版实验目录
- `LAB1/1/`：Task1 运行提供的示例程序
- `LAB1/2/`：预留
- `LAB1/3/`：预留

## GitHub Actions 生成 Windows 可执行文件

仓库包含一个 GitHub Actions 工作流，用于编译 `LAB1/1/main.cpp`：

- 文件：`.github/workflows/build-windows.yml`
- 功能：在 GitHub 的 `windows-latest` 运行器上编译 `main.cpp`
- 产物：尽量生成单文件 `main.exe`，并作为 `windows-main` artifact 上传

使用方法：

1. 把仓库推到 GitHub
2. 打开仓库的 `Actions`
3. 运行 `Build Windows Executable`，或者直接 push 到 `main` / `master`
4. 在该次工作流的 `Artifacts` 中下载 `windows-main`

下载后在 Windows 上解压，双击 `main.exe` 即可运行。
