# SNL Compiler

SNL 编译器课程项目，使用 C++17 和 CMake 实现。当前支持词法分析、递归下降语法分析、语义分析、符号表输出和基础 MIPS 代码生成。

## 快速启动

在项目目录 `code/` 下执行：

```bash
cmake -S . -B build
cmake --build build
node web/server.mjs
```

浏览器打开：

```text
http://127.0.0.1:5173
```

## CMake 编译

配置构建目录：

```bash
cmake -S . -B build
```

编译：

```bash
cmake --build build
```

编译产物默认位置：

```text
build/snlc
```

如果使用 Visual Studio、Xcode 或多配置生成器，产物可能位于类似路径：

```text
build/Debug/snlc
build/Debug/snlc.exe
```

## 命令行编译器用法

输出 Token 序列：

```bash
./build/snlc --tokens tests/valid/basic_tokens.snl
```

输出 AST：

```bash
./build/snlc --ast tests/valid/full_syntax.snl
```

显式使用递归下降语法分析器：

```bash
./build/snlc --ast tests/valid/full_syntax.snl --recursive
```

使用 LL(1) 表驱动语法分析器：

```bash
./build/snlc --ast tests/valid/full_syntax.snl --ll1
```

运行语义分析并输出符号表摘要：

```bash
./build/snlc --semantic tests/valid/semantic_ok.snl
```

语义分析也可以选择语法分析器：

```bash
./build/snlc --semantic tests/valid/semantic_ok.snl --ll1
```

生成 MIPS 汇编：

```bash
./build/snlc --mips tests/mips/while_sum.snl -o build/while_sum.asm
```

MIPS 生成同样支持 LL(1) 语法分析：

```bash
./build/snlc --mips tests/mips/while_sum.snl -o build/while_sum.asm --ll1
```

## 开启 Web 演示功能

先确保 `snlc` 已编译到 `build/snlc`：

```bash
cmake -S . -B build
cmake --build build
```

启动本地演示服务：

```bash
node web/server.mjs
```

启动成功后终端会显示：

```text
SNL Compiler Workbench: http://127.0.0.1:5173
```

浏览器访问：

```text
http://127.0.0.1:5173
```

页面功能：

- 选择 `tests/` 下的 SNL 示例程序。
- 编辑源码后点击 `Run`。
- 查看 Lexical、Syntax、Semantic、Codegen 四个阶段状态。
- 在 Tokens、AST、Semantic、MIPS、Diagnostics 标签页查看输出。
- 错误样例会自动显示诊断信息，并高亮源码行。

## 关闭 Web 演示功能

在运行 `node web/server.mjs` 的终端按：

```text
Ctrl+C
```

终端回到命令提示符后，演示服务已关闭。再次访问 `http://127.0.0.1:5173` 会连接失败。

## Web 演示可选配置

指定其他监听端口：

```bash
PORT=5174 node web/server.mjs
```

指定其他监听地址：

```bash
HOST=0.0.0.0 node web/server.mjs
```

指定 `snlc` 可执行文件路径：

```bash
SNLC_BIN=/absolute/path/to/snlc node web/server.mjs
```

多配置生成器示例：

```bash
SNLC_BIN=build/Debug/snlc node web/server.mjs
```

Windows 可执行文件示例：

```bash
SNLC_BIN=build/Debug/snlc.exe node web/server.mjs
```

## 测试样例目录

```text
tests/valid              正确样例
tests/lexical_errors     词法错误样例
tests/syntax_errors      语法错误样例
tests/semantic_errors    语义错误样例
tests/mips               MIPS 生成样例
```

演示页面的示例选择器会自动读取这些 `.snl` 文件。
