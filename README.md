# Xiangqi3D (OpenGL 3D 中国象棋)

一个**基于 OpenGL 3.3 Core** 的 3D 中国象棋示例项目：
- 棋盘/棋子优先使用你提供的 3D 模型（Assimp 导入）
- 若模型缺失：棋盘使用简易立方体 + 网格线，棋子使用圆柱体
- 单人轮流控制红黑双方（红先走）
- 完整基本规则：将/士/象/马/车/炮/兵 走法、塞象眼、蹩马腿、炮架、九宫限制、象不过河、两将照面、不能走后仍被将军
- 吃子后有“缩小下沉淡出”效果

## 运行效果与操作
- **左键**：选择棋子 / 落子 / 吃子
- **右键拖动**：旋转视角（轨道相机）
- **滚轮**：缩放
- **R**：重开
- **Esc**：退出

---

## 构建环境
- CMake >= 3.20
- C++17 编译器（MSVC / clang / gcc）
- 系统 OpenGL 驱动
- Git（用于 FetchContent 拉取依赖）
- Python3（用于 glad2 生成 OpenGL Loader）

> 如果你所在网络无法访问 GitHub（例如出现 `Failed to connect to github.com` / `Connection was reset`），
> 推荐使用 **MSYS2 pacman / vcpkg** 安装依赖，并开启 `-DXIANGQI3D_USE_SYSTEM_DEPS=ON`（见下方）。

### Python 依赖（glad2）
本项目使用 glad 的 CMake 集成在**配置阶段**生成 OpenGL 3.3 Core loader。

```bash
pip install glad2 jinja2
```

> 如果你不想用 Python 生成 glad，也可以把 glad 预生成源码放进工程并修改 CMakeLists（这里保持“纯 FetchContent”方式）。

---

## 构建与运行

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Windows / MSYS2：不访问 GitHub 的构建方式（推荐）
在 **MSYS2 MinGW64** 终端里安装依赖：

```bash
pacman -S --needed \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-glfw \
  mingw-w64-x86_64-glm \
  mingw-w64-x86_64-assimp \
  mingw-w64-x86_64-freetype \
  mingw-w64-x86_64-glad
```

然后用 Ninja 构建：

```bash
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release -DXIANGQI3D_USE_SYSTEM_DEPS=ON
cmake --build build-ninja
./build-ninja/bin/Xiangqi3D.exe
```

构建完成后运行：
- Linux/macOS：`./build/bin/Xiangqi3D`
- Windows：`build\bin\Release\Xiangqi3D.exe`（或对应配置目录）

> 构建时会自动把 `assets/` 复制到可执行文件同目录下。

---


---

## 许可证
示例项目代码可自由用于课程/作业/个人项目；第三方依赖遵循各自许可证。
