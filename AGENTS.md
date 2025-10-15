# Repository Guidelines

## Project Structure & Module Organization
- Root: Qt project file `T_DESIGNER.pro`, sources `.cpp/.h` 与 UI 表单 `.ui` 分布在根目录与子目录。
- Modules: `widget/`（自定义控件与对话框）、`BO/`（业务对象）、`DO/`（数据对象）。
- Resources: `image.qrc` 与图标（如 `T_DESIGNER.ico`）。
- Data/Tools: `Model.db`（示例数据），`generate_file_tree.py`（开发辅助脚本）。

## Build, Test, and Development Commands
- Out-of-source build（Windows, MinGW 示例）:
  - `mkdir build && cd build`
  - `qmake ..\T_DESIGNER.pro`
  - `mingw32-make -j`
- MSVC 工具链: 将 `mingw32-make` 替换为 `nmake`。
- Qt Creator: 直接打开 `T_DESIGNER.pro`，选择 Kit，`Run`。
- Run: 生成的可执行位于 `build\debug\` 或 `build\release\`（视 Kit 而定）。
- Clean: 在构建目录执行 `make clean` 或直接删除 `build/`。

## Coding Style & Naming Conventions
- 语言/框架: C++(Qt 5/6)。缩进 4 空格，UTF-8；保持现有换行风格（Windows 可 CRLF）。
- 命名: 类采用 PascalCase（如 `DialogConnectAttr`），方法/变量 lowerCamelCase，宏/常量 ALL_CAPS。
- 文件: 头/源文件同名（`.h`/`.cpp`）；UI 命名与类名对应（如 `dialogconnectattr.ui`）。
- 格式化: 建议使用 `clang-format`（Qt/Google 风格均可，保持一致）。

## Testing Guidelines
- 当前未内置测试框架。建议在 `tests/` 引入 Qt Test（文件命名 `*_test.cpp`）。
- 覆盖关键业务逻辑与自定义控件交互；将测试目标加入 `.pro` 后在本地构建运行。

## Commit & Pull Request Guidelines
- 提交信息: 支持中文/英文，推荐 Conventional Commits：`feat(scope): summary`，
  如 `feat(widget): add codecheckdialog`、`fix(BO): null check in worker`。
- PR 要求: 清晰描述、关联 Issue、涉及 UI 变更附截图；说明构建方式与验证步骤。

## Agent-Specific Instructions
- 本文件作用域为仓库全局；遵循“小改动、可回退”原则，避免无关重构。
- 不手动编辑生成文件（如 `ui_*.h`）；新增/移除源文件请同步更新 `T_DESIGNER.pro`。
- 大改动请分 PR（按模块：`widget/`、`BO/`、`DO/`）。需要脚本/工具，放入 `tools/` 或根目录并注明用途。

## Security & Configuration Tips
- 不提交敏感配置或本地路径；`Model.db` 仅用于开发演示，避免破坏性写入。
- 运行时依赖 Qt、SQLite（`sqlitedatabase.*`）与可能的 Z3（`z3solverthread.*`）；确保相关 DLL 在 `PATH` 中。

