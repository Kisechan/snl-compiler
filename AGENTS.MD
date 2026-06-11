# AGENTS.MD

本文件用于记录本项目中 Codex/Agent 的协作规则、当前项目计划，以及每一次代码实现前后的实施计划。

## 工作规则

1. 在用户明确要求实现代码之前，不主动编写项目代码。
2. 每一次开始代码实现前，必须先把本次实现计划记录到本文件的“实现计划日志”中。
3. 计划记录完成后，才能开始修改代码。
4. 每次实现前的计划至少包含：
   - 日期
   - 用户请求
   - 预计修改的文件或模块
   - 实现步骤
   - 测试或验证方式
   - 重要假设
5. 每次实现完成后，必须回到同一条计划记录中补充：
   - 实际修改内容
   - 已运行的测试或检查
   - 未解决问题或后续工作
6. 不要覆盖旧计划。新的实现计划必须追加到“实现计划日志”下面。
7. 如果后续任务改变了项目方向，必须同步更新“当前项目计划”。
8. 如果用户只要求规划、讨论或整理文档，不应顺手开始实现代码。

## 目录说明

- `初始文档/`：保存项目早期收集到的原始资料和参考资料，例如课程 PPT、PDF 提取出的 DOCX/Markdown、补充提取材料等。该目录主要作为资料来源归档，除非用户明确要求，不应随意改写其中内容。
- `文档/`：保存本项目后续整理出的正式项目文档、规划文档、设计说明、实现说明、测试说明和阶段总结。该目录中的内容可以随着项目推进持续补充和维护。
- `AGENTS.MD`：保存 Agent 协作规则、当前项目计划，以及每次代码实现前后的计划日志。

## 当前项目计划

项目名称：SNL 编译器课程项目。

目标档位：优秀档。

实现语言与构建方式：

- 使用 C++17。
- 推荐使用 CMake 管理项目构建。
- 最终产物是命令行编译器，程序名暂定为 `snlc`。

输入与输出：

- 输入：SNL 语言源程序。
- 词法分析输出：单词的内部表示序列，也就是 Token 序列。
- 语法分析输出：递归下降语法分析的语法错误信息和语法树。
- 语义分析输出：语义错误信息。
- 目标代码生成输出：可在 MARS 等模拟器上运行的 32 位 MIPS 汇编代码。

资料依据：

- PPT 是课程任务要求和 SNL 文法的主要依据。
- 从 PDF 提取出的 DOCX/Markdown 资料作为辅助依据，用于 Token 结构、AST 结构、符号表、语义分析和实现细节。
- 如果 PPT 与 PDF 提取资料冲突，除非用户另行说明，否则以 PPT 为准。

已确定的重要规则：

- 语法分析支持递归下降和 LL(1) 表驱动两种入口。
- 默认语法分析器为递归下降，命令行可通过 `--ll1` 选择 LL(1) 表驱动语法分析。
- `return` 按 PPT 规则处理，解析为 `return(expr)`；虽然 PDF 补充材料中提取到的是 `ReturnStm ::= RETURN`。
- Token 命名保持标准区分：
  - `LPAREN` / `RPAREN` 表示 `(` 和 `)`。
  - `LMIDPAREN` / `RMIDPAREN` 表示 `[` 和 `]`。
- 类型系统中应包含 `Char` 表达式类型，即使 PDF 中某个提取出的 `ExpType` 枚举没有列出 `Char`。
- `write(expr)` 应检查表达式是否合法，并尽量支持 `integer` 和 `char` 输出。
- PDF 中目标机主要是 TM 虚拟机；本项目因 PPT 要求选择 MIPS，因此 MIPS 栈帧、syscall、过程调用约定由本项目自行设计并写入文档。

计划模块：

- `token`：Token 类型、Token 值、源码位置。
- `lexer`：SNL 词法扫描器，处理注释、常量、分界符和词法错误。
- `ast`：语法树节点定义和语法树打印。
- `parser`：基于 PPT 文法的递归下降语法分析器。
- `diagnostic`：统一错误报告，包含行列号和阶段信息。
- `type`：语义类型模型，包括 integer、char、boolean、array、record、alias、procedure、error。
- `symbol_table`：作用域符号表和查找辅助函数。
- `semantic`：声明处理、类型检查、过程调用检查、表达式类型标注。
- `ir`：可选中间表示。如果直接生成 MIPS 更清晰，可以不做独立 IR。
- `mips`：面向 MARS 的 32 位 MIPS 代码生成。
- `main`：命令行入口。

预期命令行接口：

```text
snlc --tokens input.snl
snlc --ast input.snl
snlc --semantic input.snl
snlc --mips input.snl -o output.asm
snlc --all input.snl
```

测试分类：

- 正确 SNL 程序。
- 词法错误示例。
- 语法错误示例。
- 语义错误示例。
- MIPS 代码生成示例。

最终演示场景：

- 正确代码分析情况。
- 词法错误代码分析情况。
- 语法错误代码分析情况。
- 语义错误代码分析情况。
- 生成的 MIPS 目标代码在模拟器中运行并返回正确结果。

## 实现计划日志

### 2026-06-10 - 当前总体计划记录

用户请求：

- 先不实现代码。
- 创建并维护 `AGENTS.MD`。
- 要求以后每一次代码实现前都必须把改代码的计划记录下来。
- 把当前 SNL 编译器项目计划也记录下来。

预计修改的文件或模块：

- `AGENTS.MD`

实现步骤：

- 用中文记录项目协作规则。
- 记录当前 SNL 编译器总体计划。
- 记录当前已经确定的语言规则和实现取舍。
- 建立“实现计划日志”，作为后续每次代码实现前后的固定记录位置。

测试或验证方式：

- 确认 `AGENTS.MD` 存在于项目根目录。
- 确认文件中包含“工作规则”“当前项目计划”“实现计划日志”三部分。

重要假设：

- 当前阶段只做规划和项目规则整理，不实现编译器代码。
- 项目根目录为 `D:\snl_compiler`。
- 后续代码实现时，本文件是记录实施计划的准入口。

实际修改内容：

- 已将 `AGENTS.MD` 改写为中文。
- 已在“当前项目计划”中记录 SNL 编译器总体目标、模块拆分、命令行接口、测试分类和演示场景。
- 已在“实现计划日志”中记录当前总体计划。

已运行的测试或检查：

- 已确认 `AGENTS.MD` 位于 `D:\snl_compiler` 项目根目录。
- 已使用 UTF-8 读取文件，确认文件内容完整。

未解决问题或后续工作：

- 后续每次实现具体模块前，需要继续向本日志追加对应实现计划。

### 2026-06-10 - 目录职责说明补充

用户请求：

- 确认是否需要在 `AGENTS.MD` 中说明 `初始文档/` 和 `文档/` 两个文件夹分别是做什么的。

预计修改的文件或模块：

- `AGENTS.MD`

实现步骤：

- 增加“目录说明”章节。
- 说明 `初始文档/` 用于保存原始资料和参考资料。
- 说明 `文档/` 用于保存后续整理出的正式项目文档。
- 说明 `AGENTS.MD` 自身的用途。

测试或验证方式：

- 确认 `AGENTS.MD` 中包含“目录说明”章节。
- 确认文件仍可按 UTF-8 正常读取。

重要假设：

- 这是文档维护任务，不涉及编译器代码实现。

实际修改内容：

- 已添加“目录说明”章节。
- 已记录 `初始文档/`、`文档/`、`AGENTS.MD` 的职责边界。

已运行的测试或检查：

- 待确认文件内容读取无误。

未解决问题或后续工作：

- 暂无。

### 2026-06-10 - MARS 控件默认值、默认样例、端口清理与 read_add 输入

用户请求：

- 将默认 MARS jar 路径改为空字符串。
- 非 MIPS samples 隐藏 MARS 路径、运行汇编勾选框等控件。
- 默认 sample 改为 `valid/basic_tokens.snl`。
- 右侧默认标签页改为 Tokens。
- 后续开发默认使用 5173 端口。
- 找到并关闭 5173 到 5175 的运行进程。
- 修复 `read_add` 示例运行 MARS 时缺少输入导致异常的问题。
- 为输入数据增加专门选项框。

预计修改的文件或模块：

- `AGENTS.MD`
- `web/server.mjs`
- `web/public/index.html`
- `web/public/app.js`
- `web/public/styles.css`

实现步骤：

- 关闭监听 5173、5174、5175 的旧 Node 服务进程。
- 前端默认选择 `valid/basic_tokens.snl`，默认 active tab 为 Tokens。
- MARS 路径输入默认空字符串。
- 非 MIPS 样例隐藏 MARS 路径、输入数据和运行汇编控件。
- 增加 MARS stdin 输入框，并在 `read_add` 示例默认填入整数输入。
- 服务端运行 MARS 时把输入数据写入子进程 stdin。
- MARS 路径为空且用户启用运行汇编时返回明确 Run 阶段错误。
- 使用 5173 启动并验证页面、API、Run 输出。

测试或验证方式：

- 运行端口清理检查。
- 运行 `node --check web/server.mjs`。
- 运行 `node --check web/public/app.js`。
- 运行 `git diff --check`。
- 通过 API 测试 `read_add` 输入数据运行结果。
- 通过浏览器验证默认 sample、默认 Tokens tab、MIPS 控件显隐和 Run 输出。

重要假设：

- `read_add` 的输入数据用单个整数即可，默认使用 `7`，预期输出 `12`。
- MARS stdin 使用换行分隔输入值。

实际修改内容：

- 已关闭 5173、5174、5175 上的旧 Node 服务进程。
- 已将 Web 工作台重新启动到默认端口 `http://127.0.0.1:5173`。
- 已将默认 sample 改为 `valid/basic_tokens.snl`。
- 已保持右侧默认标签页为 Tokens。
- 已将 MARS jar 路径默认值改为空字符串。
- 已将 MARS jar 路径、assembly input、Run assembly 控件设为 MIPS-only。
- 已为 `mips/read_add.snl` 默认填入 assembly input `7`。
- 已将服务端 MARS 运行改为通过 stdin 写入 assembly input。
- 已在 MARS 路径为空且启用 Run assembly 时返回明确 Run 阶段错误。
- 已更新 README 中 MARS 运行说明。

已运行的测试或检查：

- 已运行端口检查，确认 5173、5174、5175 旧进程已关闭。
- 已运行 `node --check web/server.mjs`，通过。
- 已运行 `node --check web/public/app.js`，通过。
- 已运行 `git diff --check`，通过。
- 已通过 API 验证 `read_add` 携带 `assemblyInput: "7"` 和 MARS jar 路径时，Run 阶段输出 `12`。
- 已通过 API 验证 MARS jar 路径为空时，Run 阶段输出 `file error: MARS jar path is required`。
- 已通过浏览器验证默认 sample 为 `valid/basic_tokens.snl`。
- 已通过浏览器验证默认 active tab 为 Tokens。
- 已通过浏览器验证非 MIPS sample 隐藏 MARS 路径、assembly input 和 Run assembly 控件。
- 已通过浏览器验证切换到 `mips/read_add.snl` 后 MIPS-only 控件显示，MARS 路径为空，assembly input 默认 `7`。
- 已通过浏览器 390px 宽度检查移动端默认状态，无横向溢出。
- 已检查浏览器 console error/warn 日志为空。
- 已确认当前监听端口只有 5173。

未解决问题或后续工作：

- 浏览器自动化运行时无法通过 Playwright `fill/type` 输入长路径，实际运行链路已通过 API 覆盖，页面控件显隐和默认值已通过浏览器覆盖。

### 2026-06-10 - MARS 汇编运行测试与前端接入

用户请求：

- 使用 `/Users/kisechan/Developer/Projects/snl/Mars for Compile 2022.jar` 测试运行 MIPS 汇编代码。
- 在前端接入运行汇编代码功能。
- 前端可设定 MARS jar 路径。

预计修改的文件或模块：

- `AGENTS.MD`
- `web/server.mjs`
- `web/public/index.html`
- `web/public/app.js`
- `web/public/styles.css`

实现步骤：

- 确认 Java 和 MARS jar 可运行。
- 用 `snlc --mips` 生成测试汇编。
- 调用 MARS jar 运行生成的汇编，确认输出。
- 服务端 `POST /api/compile` 支持 `runAssembly` 和 `marsPath`。
- 前端工具栏增加 MARS jar 路径输入和运行汇编开关。
- 输出区域增加 Run 标签页，展示汇编运行 stdout/stderr。

测试或验证方式：

- 运行 CMake 构建。
- 运行 MARS 命令行测试。
- 运行 `node --check web/server.mjs` 和 `node --check web/public/app.js`。
- 通过 HTTP API 测试 `runAssembly: true`。
- 通过浏览器验证 MIPS 示例可生成并运行汇编。

重要假设：

- MARS 以 `java -jar <marsPath> nc <asmPath>` 的命令行形式运行。
- 汇编运行只在 MIPS 示例启用。

实际修改内容：

- 已在 `web/server.mjs` 中增加 MARS 运行阶段。
- 已支持 `runAssembly` 和 `marsPath` 请求参数。
- 已使用 `java -jar <marsPath> nc <asmPath>` 运行生成的汇编。
- 已在服务端返回 `runOutput`，并把 Run 阶段追加到 stages。
- 已在 `web/public/index.html` 增加 MARS jar 路径输入和“运行汇编”开关。
- 已在 `web/public/app.js` 增加 Run 标签页、Run 流水线阶段、请求参数和默认结果页切换。
- 已在 `web/public/styles.css` 调整工具栏控件布局，支持新增路径输入和移动端显示。
- 已更新 `README.md`，说明前端可选择语法分析器、填写 MARS jar 路径并查看 Run 输出。

已运行的测试或检查：

- 已运行 `cmake --build build`，构建成功。
- 已运行 `./build/snlc --mips tests/mips/while_sum.snl --ll1 -o build/mars_while_sum.asm`，生成汇编成功。
- 已运行 `/opt/homebrew/opt/openjdk/bin/java -jar "/Users/kisechan/Developer/Projects/snl/Mars for Compile 2022.jar" nc build/mars_while_sum.asm`，输出 `15`。
- 已运行 `node --check web/server.mjs`，通过。
- 已运行 `node --check web/public/app.js`，通过。
- 已运行 `git diff --check`，通过。
- 已启动新版 Web 工作台到 `http://127.0.0.1:5174`。
- 已通过 HTTP API 验证 `runAssembly: true` 和 MARS jar 路径，Run 阶段成功输出 `15`。
- 已通过浏览器选择 LL(1)、填写 MARS jar 路径并运行默认 MIPS 示例，Run 标签页显示 `15`。
- 已检查浏览器控制台错误日志为空。
- 已用 390px 宽度检查移动端布局，新增控件和 Run 按钮均可见且在工具栏范围内。

未解决问题或后续工作：

- 当前 Java 默认路径为 `/opt/homebrew/opt/openjdk/bin/java`，可通过 `JAVA_BIN` 环境变量覆盖。

### 2026-06-10 - Workbench 英文化、流水线布局调整与 Run 输出修复

用户请求：

- 将编译流水线状态放到 `SNL Compiler Workbench` 标题下方，占一整行。
- 将所有中文提示和页面文字改成英文版本，并让界面更协调。
- Debug 当前 Run 标签显示“暂无输出”的问题。

预计修改的文件或模块：

- `AGENTS.MD`
- `web/server.mjs`
- `web/public/index.html`
- `web/public/app.js`
- `web/public/styles.css`

实现步骤：

- 将 header 布局改为标题区在上、流水线状态条在下。
- 将 HTML、JS、服务端样例分组标签中的中文文案改为英文。
- 调整 pipeline 和工具栏样式，使五阶段状态在整行内协调显示。
- 修复 Run 标签输出：优先读取 Run stage，缺失时使用 `runOutput` 兜底。
- 用浏览器验证 MIPS 示例运行后 Run 标签显示 MARS 输出。

测试或验证方式：

- 运行 `node --check web/server.mjs`。
- 运行 `node --check web/public/app.js`。
- 运行 `git diff --check`。
- 启动 Web 服务并通过浏览器验证桌面和移动端布局。
- 点击 Run，确认 Run 标签显示 MARS 输出。

重要假设：

- 页面面向英文演示环境。
- Run 输出来自 MARS stdout/stderr，空输出时显示英文空状态。

实际修改内容：

- 已将 header 改为单列布局：标题和说明在上，pipeline 状态条在下方独占整行。
- 已将前端页面文案、aria label、空状态、错误提示、样例分组标签改为英文。
- 已将 pipeline 调整为整行自适应布局，五阶段状态可在桌面横向排列。
- 已修复 Run 面板输出逻辑：优先显示 Run stage 输出，缺失 stage 时用 `runOutput` 兜底。
- 已调整桌面和移动端工具栏布局，MARS 路径输入不再挤压或横向溢出。

已运行的测试或检查：

- 已运行 `node --check web/server.mjs`，通过。
- 已运行 `node --check web/public/app.js`，通过。
- 已运行 `git diff --check`，通过。
- 已启动当前版本 Web 工作台到 `http://127.0.0.1:5175`。
- 已通过浏览器验证 pipeline 位于标题下方并占整行。
- 已通过浏览器验证页面可见文本无中文残留。
- 已通过浏览器选择 LL(1) 并运行默认 MIPS 示例，Run 标签显示 `15`。
- 已确认 Run 面板不再显示 `No output available.`。
- 已检查浏览器 console error/warn 日志为空。
- 已用 390px 宽度验证移动端布局，MARS 路径输入、运行开关和按钮均可见且无横向溢出。

未解决问题或后续工作：

- 暂无。

### 2026-06-10 - LL(1) 批量测试与前端接入

用户请求：

- 重新编译项目。
- 使用 `tests/` 文件夹下的 SNL 代码测试 LL(1) parser。
- 将 LL(1) parser 功能接入前端页面。

预计修改的文件或模块：

- `AGENTS.MD`
- `web/server.mjs`
- `web/public/index.html`
- `web/public/app.js`
- `web/public/styles.css`

实现步骤：

- 重新运行 CMake 构建。
- 批量运行 `tests/valid`、`tests/syntax_errors`、`tests/semantic_errors`、`tests/mips` 下样例的 LL(1) 路径。
- 服务端 `POST /api/compile` 接收 parser mode 参数。
- 前端工具栏增加语法分析器选择控件。
- 前端运行编译时提交当前 parser mode。
- 本地启动 Web 工作台并验证页面选择 LL(1) 后各阶段调用成功。

测试或验证方式：

- 运行 `cmake --build build`。
- 运行 CLI 批量测试脚本覆盖 `tests/` 下样例。
- 运行 `node --check web/server.mjs` 和 `node --check web/public/app.js`。
- 启动 `node web/server.mjs`。
- 通过 HTTP API 测试 `parserMode: "ll1"`。
- 打开本地页面验证选择控件和 LL(1) 编译结果。

重要假设：

- 前端默认语法分析器保持 `recursive`。
- LL(1) 模式通过服务端传参调用现有 `snlc --ll1`。

实际修改内容：

- 已在 `web/server.mjs` 中为 `POST /api/compile` 增加 `parserMode` 参数处理。
- 已将 Syntax、Semantic、Codegen 阶段命令扩展为按 parser mode 传入 `--recursive` 或 `--ll1`。
- 已在 `web/public/index.html` 的工具栏增加“语法分析器”选择框。
- 已在 `web/public/app.js` 中保存 parser mode 状态，并随编译请求发送到服务端。
- 已在阶段摘要中显示当前语法分析器。
- 已在 `web/public/styles.css` 中调整工具栏布局，支持桌面并排和移动端纵向排列。

已运行的测试或检查：

- 已运行 `cmake --build build`，构建成功。
- 已批量运行 `tests/valid/*.snl` 的 `--ast --ll1`，均通过。
- 已批量运行 `tests/syntax_errors/*.snl` 的 `--ast --ll1`，均返回语法错误。
- 已批量运行 `tests/semantic_errors/*.snl` 的 `--semantic --ll1`，均返回语义错误。
- 已批量运行 `tests/mips/*.snl` 的 `--mips --ll1`，均生成非空 MIPS 汇编。
- 已批量运行 `tests/lexical_errors/*.snl` 的 `--ast --ll1`，均返回词法错误。
- 已运行 `node --check web/server.mjs`，通过。
- 已运行 `node --check web/public/app.js`，通过。
- 已运行 `git diff --check`，通过。
- 已启动 `node web/server.mjs`，访问地址为 `http://127.0.0.1:5173`。
- 已通过 HTTP API 请求验证 `parserMode: "ll1"`，返回 Lexical、Syntax、Semantic 成功。
- 已通过浏览器选择 LL(1) 并运行默认 MIPS 示例，返回 Lexical、Syntax、Semantic、Codegen 全部 success，MIPS 输出可见。
- 已检查浏览器控制台错误日志为空。
- 已用 390px 宽度检查移动端布局，示例选择、语法分析器选择和运行按钮均可见且在工具栏范围内。

未解决问题或后续工作：

- 暂无。

### 2026-06-10 - 增加 LL(1) 语法分析器选择

用户请求：

- 添加 LL(1) 语法分析功能。
- 支持用 `--ll1` 或 `--recursive` 选择语法分析器。
- `--recursive` 作为默认语法分析器。

预计修改的文件或模块：

- `AGENTS.MD`
- `CMakeLists.txt`
- `README.md`
- `src/main.cpp`
- `src/parser.h`
- `src/ll1_parser.h`
- `src/ll1_parser.cpp`

实现步骤：

- 将现有递归下降解析器保留为默认路径。
- 新增 `LL1Parser`，通过 LL(1) 表驱动方式验证 Token 序列。
- LL(1) 解析成功后复用现有递归下降解析器构造 AST，保证后续语义分析和 MIPS 生成使用相同 AST 结构。
- 在命令行入口解析 `--recursive` 和 `--ll1` 可选参数。
- 更新用法说明和 README。

测试或验证方式：

- 运行 CMake 构建。
- 运行 `--ast` 默认路径。
- 运行 `--ast ... --recursive`。
- 运行 `--ast ... --ll1`。
- 运行 `--semantic ... --ll1`。
- 运行语法错误样例，确认 `--ll1` 输出语法错误。
- 运行 `--mips ... --ll1 -o ...`。

重要假设：

- LL(1) 模式优先用于展示表驱动语法分析流程。
- AST 构造复用现有递归下降解析器，避免语义分析和代码生成路径重复维护。

实际修改内容：

- 已新增 `src/ll1_parser.h` 和 `src/ll1_parser.cpp`，实现 LL(1) 表驱动语法验证。
- 已将 `src/ll1_parser.cpp` 加入 CMake 构建。
- 已在 `src/main.cpp` 中接入 `--recursive` 和 `--ll1` 可选参数。
- 已保持 `--recursive` 为默认语法分析器。
- 已支持 `--ast`、`--semantic`、`--mips` 三个阶段选择 LL(1) 语法分析器。
- 已更新 `README.md` 的命令行示例。

已运行的测试或检查：

- 已运行 `cmake --build build`，构建成功。
- 已运行 `./build/snlc --ast tests/valid/full_syntax.snl`，退出码为 0。
- 已运行 `./build/snlc --ast tests/valid/full_syntax.snl --recursive`，退出码为 0。
- 已运行 `./build/snlc --ast tests/valid/full_syntax.snl --ll1`，退出码为 0。
- 已运行 `./build/snlc --ast --ll1 tests/valid/basic_tokens.snl`，退出码为 0。
- 已运行 `./build/snlc --semantic tests/valid/semantic_ok.snl --ll1`，退出码为 0。
- 已运行 `./build/snlc --semantic --ll1 tests/valid/semantic_nested_ok.snl`，退出码为 0。
- 已运行 `./build/snlc --ast tests/syntax_errors/missing_semi.snl --ll1`，返回语法错误并退出码为 1。
- 已运行 `./build/snlc --mips tests/mips/while_sum.snl --ll1 -o build/while_sum_ll1.asm`，退出码为 0，并生成 MIPS 汇编。
- 已批量运行 `tests/valid/*.snl` 的 `--ast --ll1`，均通过。
- 已批量运行 `tests/semantic_errors/*.snl` 的 `--semantic --ll1`，均返回语义错误。
- 已批量运行 `tests/syntax_errors/*.snl` 的 `--ast --ll1`，均返回语法错误。
- 已运行 `git diff --check`，未发现空白错误。

未解决问题或后续工作：

- LL(1) 模式当前负责表驱动语法验证，AST 构造继续复用递归下降解析器。

### 2026-06-10 - Web 演示：MIPS/Codegen 只对 MIPS 示例显示

用户请求：

- 解释 Tokens、AST、Semantic、MIPS、Diagnostics 五个标签页和 Lexical/Syntax/Semantic/Codegen 状态含义。
- 设置只有 MIPS 生成示例代码有 MIPS、Codegen 检查展示功能。

预计修改的文件或模块：

- `AGENTS.MD`
- `web/server.mjs`
- `web/public/app.js`

实现步骤：

- `POST /api/compile` 支持 `enableCodegen` 参数。
- 服务端只在 `enableCodegen` 为 true 时运行 `snlc --mips`。
- 前端根据当前示例是否属于 `mips` 分组决定是否显示 MIPS 标签页和 Codegen 状态。
- 前端运行编译时随请求发送 `enableCodegen`。
- 普通示例成功后默认切到 Semantic；MIPS 示例成功后默认切到 MIPS。

测试或验证方式：

- 运行 JS 语法检查。
- API 验证 `valid/full_syntax.snl` 携带 `enableCodegen:false` 时只返回 Lexical、Syntax、Semantic。
- API 验证 `mips/while_sum.snl` 携带 `enableCodegen:true` 时返回 Codegen 和 MIPS 汇编。
- 浏览器验证普通示例不显示 MIPS 标签和 Codegen 状态，MIPS 示例显示 MIPS 标签和 Codegen 状态。

重要假设：

- 是否启用 Codegen 由当前选择的示例分组决定；用户在 MIPS 示例基础上编辑源码时仍保留 Codegen 展示。

实际修改内容：

- 已在 `POST /api/compile` 中新增 `enableCodegen` 参数。
- 已将服务端编译阶段改为按 `enableCodegen` 选择是否运行 `snlc --mips`。
- 已将前端编译请求改为根据当前示例分组发送 `enableCodegen`。
- 已将前端 Tabs 改为普通示例显示 Tokens、AST、Semantic、Diagnostics；MIPS 示例显示 Tokens、AST、Semantic、MIPS、Diagnostics。
- 已将前端流水线状态改为普通示例显示 Lexical、Syntax、Semantic；MIPS 示例显示 Lexical、Syntax、Semantic、Codegen。
- 已保留 MIPS 示例编辑后的 Codegen 展示能力。

已运行的测试或检查：

- 已运行 `node --check web/server.mjs`，通过。
- 已运行 `node --check web/public/app.js`，通过。
- 已重启 `node web/server.mjs`，服务运行在 `http://127.0.0.1:5173`。
- 已请求 `POST /api/compile`，验证 `tests/valid/full_syntax.snl` 携带 `enableCodegen:false` 时只返回 Lexical、Syntax、Semantic 三个阶段。
- 已请求 `POST /api/compile`，验证 `tests/mips/while_sum.snl` 携带 `enableCodegen:true` 时返回 Lexical、Syntax、Semantic、Codegen 四个阶段，并返回 MIPS 汇编。
- 已使用内置 Playwright 验证 `valid/full_syntax.snl` 页面只显示 Tokens、AST、Semantic、Diagnostics，且流水线不显示 Codegen。
- 已使用内置 Playwright 验证 `mips/while_sum.snl` 页面显示 Tokens、AST、Semantic、MIPS、Diagnostics，且流水线显示 Codegen。
- 已确认浏览器控制台无错误。

未解决问题或后续工作：

- 暂无。

### 2026-06-10 - 第一轮实现：词法分析器优先

用户请求：

- 按已确认的“词法分析器优先”计划开始第一轮代码实现。
- 第一轮只做词法分析器可用闭环，不展开完整编译器。

预计修改的文件或模块：

- `AGENTS.MD`
- `CMakeLists.txt`
- `src/main.cpp`
- `src/token.h`
- `src/token.cpp`
- `src/lexer.h`
- `src/lexer.cpp`
- `src/diagnostic.h`
- `src/diagnostic.cpp`
- `tests/valid/basic_tokens.snl`
- `tests/lexical_errors/unclosed_comment.snl`
- `tests/lexical_errors/invalid_char.snl`
- `tests/lexical_errors/invalid_char_const.snl`
- `tests/lexical_errors/lone_colon.snl`

实现步骤：

- 建立必要的最小 C++17/CMake 工程。
- 定义 Token 类型、Token 数据结构和 Token 名称转换函数。
- 实现词法诊断结构，记录阶段、行列号和错误原因。
- 实现 Lexer，支持保留字、标识符、整数常量、字符常量、注释、运算符和分界符。
- 实现 `snlc --tokens input.snl` 命令行入口。
- 为未实现的 `--ast`、`--semantic`、`--mips`、`--all` 输出明确提示。
- 添加正常 Token 样例和基本词法错误样例。

测试或验证方式：

- 运行 `cmake -S . -B build`。
- 运行 `cmake --build build`。
- 运行 `build/snlc --tokens tests/valid/basic_tokens.snl`。
- 运行词法错误样例，确认错误信息包含阶段、行列号和原因，且退出码为非 0。

重要假设：

- 第一轮不实现 AST、Parser、Semantic、MIPS。
- Token 输出采用稳定文本格式：`line column TOKEN value`。
- 字符常量第一版只支持单个字母或数字，例如 `'a'`、`'7'`。
- 当前工具链使用本机已有的 CMake 和 MinGW g++。

实际修改内容：

- 已建立最小 C++17/CMake 工程，生成命令行程序 `snlc`。
- 已实现 `token` 模块，包含 Token 类型、源码位置、词素、语义值和 Token 名称输出。
- 已实现 `diagnostic` 模块，支持阶段、行列号和错误信息格式化。
- 已实现 `lexer` 模块，支持保留字、标识符、整数常量、字符常量、注释、运算符、分界符和 EOF。
- 已实现 `snlc --tokens input.snl`，可输出 Token 序列；词法错误时仍输出已识别 Token，并返回非 0。
- 已为 `--ast`、`--semantic`、`--mips`、`--all` 添加未实现提示。
- 已添加一个正常词法样例和四个词法错误样例。

已运行的测试或检查：

- 已运行 `cmake -S . -B build`，配置成功；实际自动选择 Visual Studio 17 2022 生成器。
- 已运行 `cmake --build build`，构建成功，产物为 `build/Debug/snlc.exe`。
- 已运行 `build/Debug/snlc.exe --tokens tests/valid/basic_tokens.snl`，正常输出 Token 序列，退出码为 0。
- 已运行未闭合注释、非法字符、非法字符常量、单独冒号四个错误样例，均输出包含词法阶段、行列号和原因的诊断，退出码为 1。
- 已运行 `build/Debug/snlc.exe --ast tests/valid/basic_tokens.snl`，确认未实现命令返回明确提示，退出码为 2。

未解决问题或后续工作：

- 当前未实现语法分析、AST、语义分析和 MIPS 代码生成。
- Token 输出格式已固定为 `line column TOKEN value`，后续如需完全模拟课程表格格式，可再增加专门的格式化选项。
- CMake 本次实际使用 Visual Studio 工具链；如果后续要强制 MinGW，需要重新指定生成器或独立构建目录。

### 2026-06-10 - 第二轮实现：完整递归下降语法分析与 AST 输出

用户请求：

- 按已确认的“完整递归下降语法分析与 AST 输出”计划实现第二轮。
- 实现 `snlc --ast input.snl`，在词法通过后构造并打印 AST。

预计修改的文件或模块：

- `AGENTS.MD`
- `CMakeLists.txt`
- `src/main.cpp`
- `src/diagnostic.h`
- `src/diagnostic.cpp`
- `src/ast.h`
- `src/ast.cpp`
- `src/parser.h`
- `src/parser.cpp`
- `文档/SNL_PPT_语法产生式摘要.md`
- `tests/valid/full_syntax.snl`
- `tests/syntax_errors/missing_end.snl`
- `tests/syntax_errors/missing_semi.snl`
- `tests/syntax_errors/bad_statement.snl`
- `tests/syntax_errors/mismatched_paren.snl`

实现步骤：

- 新增 AST 节点结构和缩进树打印函数。
- 新增 Parser，按 PPT 文法实现递归下降语法分析。
- 采用 PPT 规则处理 `return(expr)`。
- 扩展诊断阶段，增加语法错误诊断。
- 将 `--ast` 接入命令行入口，词法错误时不进入语法分析。
- 更新 CMake 构建文件。
- 整理本轮采用的 PPT 语法产生式摘要。
- 添加完整语法正常样例和语法错误样例。

测试或验证方式：

- 运行 `cmake --build build`，必要时先运行 `cmake -S . -B build`。
- 回归运行 `--tokens` 正常样例和现有词法错误样例。
- 运行 `--ast` 正常样例，确认输出包含 `ProK`、`TypeK`、`VarK`、`ProcDecK`、`StmLK`、`StmtK`、`ExpK`。
- 运行语法错误样例，确认输出 `syntax error`、行列号、期望内容，且退出码为非 0。
- 运行 `--semantic`，确认仍返回未实现提示。

重要假设：

- 第二轮不做语义检查；未声明、类型不匹配、参数不匹配留到语义分析阶段。
- AST 使用更适合 C++ 后续遍历的节点列表结构，不复刻 PDF 的 `child[3] + sibling` 内存结构。
- Parser 采用手写递归下降，不引入 yacc/bison 或 LL(1) 表驱动。
- 语法树输出采用稳定文本格式，不新增 JSON 输出。
- 除非 parser 接入发现 Token 类型缺失，否则不调整词法规则。

实际修改内容：

- 已新增 `ast` 模块，包含 AST 节点类别、源码位置、显示信息、名字列表、子节点列表和缩进树打印函数。
- 已新增 `parser` 模块，输入 Token 序列并输出 `ParserResult`，支持 PPT 文法中的程序头、类型声明、变量声明、过程声明、参数、程序体、语句列表、if/while/read/write/return/call/assign、表达式、数组访问和记录字段访问。
- 已按 PPT 规则实现 `return(expr)`，没有采用 PDF 补充资料中的裸 `return`。
- 已扩展 `DiagnosticStage`，增加 `Syntax`，语法错误输出为统一诊断格式。
- 已接入 `snlc --ast input.snl`；词法错误存在时只输出词法错误，不进入语法分析。
- 已更新 CMake，将 `ast.cpp` 和 `parser.cpp` 纳入构建。
- 已整理 `文档/SNL_PPT_语法产生式摘要.md`，记录本轮采用的 PPT 文法。
- 已新增完整语法正常样例和四个语法错误样例。

已运行的测试或检查：

- 已运行 `cmake --build build`，构建成功，产物为 `build/Debug/snlc.exe`。
- 已运行 `build/Debug/snlc.exe --tokens tests/valid/basic_tokens.snl`，退出码为 0。
- 已回归运行未闭合注释、非法字符、非法字符常量、单独冒号四个词法错误样例，退出码均为 1，并输出词法错误位置和原因。
- 已运行 `build/Debug/snlc.exe --ast tests/valid/basic_tokens.snl`，退出码为 0，输出 AST。
- 已运行 `build/Debug/snlc.exe --ast tests/valid/full_syntax.snl`，退出码为 0，输出包含 `ProK`、`TypeK`、`VarK`、`ProcDecK`、`StmLK`、`StmtK`、`ExpK` 的 AST。
- 已运行 `missing_end`、`missing_semi`、`bad_statement`、`mismatched_paren` 四个语法错误样例，退出码均为 1，并输出 `syntax error`、行列号和期望信息。
- 已运行 `build/Debug/snlc.exe --semantic tests/valid/basic_tokens.snl`，确认仍返回未实现提示，退出码为 2。

未解决问题或后续工作：

- 当前只做语法结构检查，不做语义检查；未声明变量、类型不兼容、过程实参与形参不匹配等留到第三轮语义分析。
- 语法错误恢复仍是基础同步策略；例如缺少分号时可能出现少量连锁诊断，但能返回非 0 并指出首个关键语法位置。
- AST 打印格式已稳定用于演示和测试；后续语义分析可能需要在 AST 节点上补充类型、符号表入口或左值属性。

### 2026-06-10 - 第三轮实现：语义分析、符号表与后端属性

用户请求：

- 按已确认的“语义分析、符号表与后端属性”计划实现第三轮。
- 实现 `snlc --semantic input.snl`，流程为 lexer -> parser -> semantic。

预计修改的文件或模块：

- `AGENTS.MD`
- `CMakeLists.txt`
- `src/main.cpp`
- `src/diagnostic.h`
- `src/diagnostic.cpp`
- `src/ast.h`
- `src/type.h`
- `src/type.cpp`
- `src/symbol_table.h`
- `src/symbol_table.cpp`
- `src/semantic.h`
- `src/semantic.cpp`
- `tests/valid/semantic_ok.snl`
- `tests/semantic_errors/*.snl`

实现步骤：

- 新增类型系统，支持 integer、char、boolean、array、record、alias、procedure、error。
- 新增分层符号表，支持当前作用域重复定义检查、外层查找、记录字段查找。
- 在 AST 节点中补充语义属性字段，但保持 `--ast` 文本输出不变。
- 实现语义分析器，处理类型声明、变量声明、过程声明、参数、语句、表达式和变量访问。
- 为变量、参数和过程记录 level、offset、size、参数模式、局部数据区大小等后端预留属性。
- 接入 `snlc --semantic input.snl`，前序词法或语法错误存在时不进入语义分析。
- 添加合法语义样例和覆盖主要语义错误类别的错误样例。

测试或验证方式：

- 运行 `cmake --build build`。
- 回归运行 `--tokens`、`--ast` 现有正常样例和错误样例。
- 运行 `--semantic` 合法样例，确认退出码为 0，并输出符号表摘要。
- 运行 `--semantic` 语义错误样例，确认退出码为 1，并输出 `semantic error at line:column: ...`。
- 运行词法错误和语法错误样例，确认 `--semantic` 不进入后续阶段。

重要假设：

- 本轮不生成 IR 或 MIPS 汇编。
- 后端属性只保证语义阶段可记录和打印，不承诺最终 MIPS 栈帧布局已经定型。
- 类型别名按声明顺序解析，不支持前向引用。
- 过程名在分析过程体前登记到当前作用域，允许过程递归调用自身。
- 同一作用域内类型名、变量名、过程名共享命名空间，重名即重复定义。
- 后声明的兄弟过程不对前面的过程体可见。
- `return(expr)` 只检查表达式语义合法；SNL 过程无返回类型，本轮不要求返回值类型匹配。
- `write(expr)` 接受 integer 和 char。

实际修改内容：

- 已新增 `type` 模块，支持 integer、char、boolean、array、record、alias、procedure、error 类型，包含类型等价判断、别名解析、大小计算和类型格式化。
- 已新增 `symbol_table` 模块，支持分层作用域、当前作用域重复定义检查、外层查找、稳定 symbol id、level、offset、size、参数模式和过程局部大小。
- 已在 AST 节点上补充内部语义属性：推导类型、是否左值、绑定符号 id、变量访问类别；未改变 `--ast` 文本输出。
- 已新增 `semantic` 模块，处理类型声明、变量声明、过程声明、参数登记、语句检查、表达式检查、数组访问、记录字段访问和过程调用检查。
- 已接入 `snlc --semantic input.snl`；词法或语法阶段有错误时不会进入语义分析。
- 已更新 CMake，将 `type.cpp`、`symbol_table.cpp`、`semantic.cpp` 纳入构建。
- 已新增合法语义样例 `semantic_ok.snl`、`semantic_nested_ok.snl`。
- 已新增语义错误样例，覆盖重复定义、未声明标识符、无效类型名、赋值类型不匹配、比较类型不匹配、数组错误、记录错误和过程调用错误。

已运行的测试或检查：

- 已运行 `cmake --build build`，构建成功，产物为 `build/Debug/snlc.exe`。
- 已回归运行 `build/Debug/snlc.exe --tokens tests/valid/basic_tokens.snl`，退出码为 0。
- 已回归运行 `build/Debug/snlc.exe --ast tests/valid/full_syntax.snl`，退出码为 0。
- 已运行 `build/Debug/snlc.exe --semantic tests/valid/basic_tokens.snl`，退出码为 0。
- 已运行 `build/Debug/snlc.exe --semantic tests/valid/full_syntax.snl`，退出码为 0。
- 已运行 `build/Debug/snlc.exe --semantic tests/valid/semantic_ok.snl`，退出码为 0，并输出符号表摘要。
- 已运行 `build/Debug/snlc.exe --semantic tests/valid/semantic_nested_ok.snl`，退出码为 0，并验证嵌套过程作用域和非局部标识符查找。
- 已批量运行 `tests/semantic_errors/*.snl`，所有语义错误样例退出码均为 1。
- 已运行 `build/Debug/snlc.exe --semantic tests/lexical_errors/invalid_char.snl`，确认只输出词法错误并返回 1。
- 已运行 `build/Debug/snlc.exe --semantic tests/syntax_errors/missing_end.snl`，确认只输出语法错误并返回 1。
- 已运行 `build/Debug/snlc.exe --mips tests/valid/basic_tokens.snl` 和 `--all`，确认仍返回未实现提示，退出码为 2。

未解决问题或后续工作：

- 本轮不生成 IR 或 MIPS 汇编；下一轮可在现有符号表 level/offset/size 基础上设计 MIPS 后端或先设计 IR。
- 当前后端属性只作为语义阶段摘要和后端预留信息，最终 MIPS 栈帧布局尚未定型。
- 由于 PPT 文法中 if/while 条件是 `RelExp`，语法正确的条件天然推导为 boolean；“条件非 boolean”通常会表现为语法错误或比较操作数类型错误。
- AST 仍主要由 parser 的 `detail` 字符串承载语法细节；语义分析已集中解析这些字符串，后续若要增强维护性，可继续把 AST 节点细化为结构化字段。

### 2026-06-10 - 第四轮实现：MIPS 基础代码生成与 MARS 验证闭环

用户请求：

- 按已确认的“第四阶段：MIPS 基础代码生成与 MARS 验证闭环”计划实现。
- 实现 `snlc --mips input.snl -o output.asm`，让基础 SNL 程序生成可在 MARS 中运行的 32 位 MIPS 汇编。

预计修改的文件或模块：

- `AGENTS.MD`
- `CMakeLists.txt`
- `src/main.cpp`
- `src/mips.h`
- `src/mips.cpp`
- `tests/mips/basic_write.snl`
- `tests/mips/read_add.snl`
- `tests/mips/if_else.snl`
- `tests/mips/while_sum.snl`
- `tests/mips/char_write.snl`

实现步骤：

- 新增 `mips` 模块，基于语义分析后的 AST 和符号表直接生成 MIPS 汇编。
- 扩展命令行入口，支持 `snlc --mips input.snl -o output.asm`。
- 语义前置阶段有错误时不生成汇编。
- 第一版使用 `.data` 为主程序变量分配空间，生成 `.text`、`.globl main`、`main:` 和退出 syscall。
- 支持整数/字符常量、变量读写、`+ - * /`、`< =`、赋值、`read`、`write`、`if` 和 `while`。
- 新增基础 MIPS 样例，用 MARS jar 进行汇编和运行验证。

测试或验证方式：

- 运行 `cmake --build build`。
- 回归运行 `--tokens`、`--ast`、`--semantic` 现有正常样例。
- 回归运行现有词法、语法、语义错误样例，确认仍返回非 0。
- 运行 `--mips` 生成新增 MIPS 样例的汇编。
- 运行 `java -jar "C:\Users\duckNroast\Desktop\Mars for Compile 2022.jar" nc a ae1 output.asm`。
- 运行 `java -jar "C:\Users\duckNroast\Desktop\Mars for Compile 2022.jar" nc sm me ae1 se1 100000 output.asm`。

重要假设：

- 第四阶段目标是基础 MIPS 可运行闭环，不一次性完成全部后端。
- 本阶段不引入独立 IR。
- MIPS 汇编以 MARS 4.5 兼容为准。
- 当前语义阶段已有的类型、符号 id、offset、size 足够支撑第一版基础代码生成。
- 本阶段暂缓数组、记录、过程调用、参数传递、递归、嵌套过程静态链和 `var` 参数。

实际修改内容：

- 已新增 `mips` 模块，直接基于语义分析后的 AST 和符号表生成 MARS 兼容 MIPS 汇编。
- 已新增 `DiagnosticStage::Codegen`，用于报告本阶段暂不支持的后端结构。
- 已接入 `snlc --mips input.snl -o output.asm`，执行流程为 lexer -> parser -> semantic -> mips；前序阶段或后端阶段有错误时不写出汇编。
- 已生成 `.data`、`.text`、`.globl main`、`main:`、程序退出 syscall，并为主程序变量生成全局存储标签。
- 已支持整数/字符常量、主程序变量读写、`+ - * /`、`< =`、赋值、`read(integer)`、`write(integer)`、`write(char)`、`if` 和 `while`。
- 已新增五个 MIPS 验证样例：基础写整数、读入后计算、if/else、while 累加和字符输出。

已运行的测试或检查：

- 已运行 `cmake --build build`，构建成功，产物为 `build/Debug/snlc.exe`。
- 已运行 `snlc --mips` 为 `tests/mips/*.snl` 生成 `build/mips_tests/*.asm`。
- 已运行 MARS 汇编检查：`java -jar "C:\Users\duckNroast\Desktop\Mars for Compile 2022.jar" nc a ae1 *.asm`，五个样例均通过。
- 已运行 MARS 模拟执行：`basic_write` 输出 `14`，`char_write` 输出 `A`，`if_else` 输出 `10`，`read_add` 在输入 `7` 时输出 `12`，`while_sum` 输出 `15`。
- 已回归运行 `--tokens tests/valid/basic_tokens.snl`、`--ast tests/valid/full_syntax.snl`、`--semantic tests/valid/semantic_ok.snl` 和 `--semantic tests/valid/semantic_nested_ok.snl`，均退出码为 0。
- 已回归运行 `tests/lexical_errors/*.snl`、`tests/syntax_errors/*.snl`、`tests/semantic_errors/*.snl`，均保持非 0 退出码。
- 已运行 `--mips tests/valid/semantic_ok.snl -o build/mips_tests/semantic_ok_unsupported.asm`，确认数组、记录和过程调用等暂缓特性会输出 `codegen error`，且不写出汇编文件。
- 已运行 `git diff --check`，未发现空白错误。

未解决问题或后续工作：

- 本阶段暂不支持数组和记录地址计算。
- 本阶段暂不支持过程声明代码生成、过程调用、参数传递、递归、嵌套过程静态链和 `var` 参数。
- `read` 当前只支持主程序作用域的 integer 变量；char 输入可在后续阶段按需要补充。
- `--all` 仍未实现，后续演示整理阶段可接入完整流水线输出。

### 2026-06-10 - 第五轮实现：本地 Web 编译工作台

用户请求：

- 实现 SNL 编译器前端展示方案。
- 新增本地 Web 工作台，浏览器输入或选择 SNL 示例，Node 本地服务调用现有 `snlc`，展示 Token、AST、语义分析、符号表、MIPS 和错误诊断。

预计修改的文件或模块：

- `AGENTS.MD`
- `web/server.mjs`
- `web/public/index.html`
- `web/public/styles.css`
- `web/public/app.js`

实现步骤：

- 新增零依赖 Node HTTP 服务，默认监听 `127.0.0.1:5173`。
- 启动时检查 `build/snlc` 是否存在，缺失时输出 CMake 构建提示。
- 实现 `GET /api/samples`，读取 `tests/**/*.snl` 并按测试类别分组返回。
- 实现 `POST /api/compile`，写入临时 `.snl` 文件，依次调用 `--tokens`、`--ast`、`--semantic`、`--mips`。
- 解析现有诊断文本为 `stage / line / column / message` 结构。
- 新增深色单页前端，包含源码编辑区、示例选择、运行和重置按钮、流水线状态条、阶段 Tabs、诊断表和源码行高亮。
- 保持 C++ 编译器 CLI 作为唯一编译器接口。

测试或验证方式：

- 运行 `node web/server.mjs`。
- 请求 `GET /api/samples`，确认示例按 `valid`、`lexical_errors`、`syntax_errors`、`semantic_errors`、`mips` 分组返回。
- 请求 `POST /api/compile`，分别覆盖 `valid/full_syntax.snl`、`lexical_errors/invalid_char.snl`、`syntax_errors/missing_semi.snl`、`semantic_errors/assignment_mismatch.snl`、`mips/while_sum.snl`。
- 浏览器访问 `http://127.0.0.1:5173`，确认示例切换、运行、Tabs、长文本滚动、错误列表和源码高亮可用。
- 检查浏览器控制台无前端运行错误。

重要假设：

- 演示环境使用本机浏览器访问 `http://127.0.0.1:5173`。
- 前端采用原生 HTML/CSS/JavaScript 和 Node 内置模块，降低安装成本。
- `snlc` 已通过 CMake 构建到 `build/snlc`。
- MIPS 阶段只展示当前后端支持范围内的输出；遇到暂缓特性时展示 codegen 诊断。

实际修改内容：

- 已新增 `web/server.mjs`，使用 Node 内置 HTTP 服务提供静态页面、`GET /api/samples` 和 `POST /api/compile`。
- 已实现启动前 `build/snlc` 检查，缺失时输出 CMake 构建提示。
- 已实现样例分组读取，覆盖 `valid`、`lexical_errors`、`syntax_errors`、`semantic_errors`、`mips`。
- 已实现临时源码写入和分阶段编译调用，阶段顺序为 Lexical、Syntax、Semantic、Codegen；前序阶段失败时后续阶段标记为 skipped。
- 已实现诊断文本解析，返回 `stage`、`line`、`column`、`message`。
- 已新增 `web/public/index.html`、`web/public/styles.css`、`web/public/app.js`，实现深色 Web 工作台、源码编辑器、示例选择、运行和重置按钮、流水线状态条、阶段 Tabs、诊断表、源码行高亮和移动端布局。

已运行的测试或检查：

- 已运行 `node --check web/server.mjs`，通过。
- 已运行 `node --check web/public/app.js`，通过。
- 已运行 `node web/server.mjs`，服务启动到 `http://127.0.0.1:5173`。
- 已请求 `GET /api/samples`，确认五类样例均按分组返回。
- 已通过 `POST /api/compile` 覆盖 `tests/valid/full_syntax.snl`，结果为 Lexical、Syntax、Semantic 成功，Codegen 按当前后端支持范围返回错误诊断。
- 已通过 `POST /api/compile` 覆盖 `tests/lexical_errors/invalid_char.snl`，结果为 Lexical error，后续阶段 skipped。
- 已通过 `POST /api/compile` 覆盖 `tests/syntax_errors/missing_semi.snl`，结果为 Syntax error，后续阶段 skipped。
- 已通过 `POST /api/compile` 覆盖 `tests/semantic_errors/assignment_mismatch.snl`，结果为 Semantic error，Codegen skipped。
- 已通过 `POST /api/compile` 覆盖 `tests/mips/while_sum.snl`，结果为四阶段 success，并返回 MIPS 汇编。
- 已使用内置 Playwright 打开页面，选择 `mips/while_sum.snl` 并运行，确认自动切到 MIPS、`.data` 输出可见、浏览器控制台无错误。
- 已使用内置 Playwright 打开 `lexical_errors/invalid_char.snl` 并运行，确认自动切到 Diagnostics、错误文本可见、源码行高亮定位。
- 已保存并检查桌面截图 `/private/tmp/snl-workbench-desktop.png` 和移动端截图 `/private/tmp/snl-workbench-mobile.png`。
- 已运行 `git diff --check`，未发现空白错误。

未解决问题或后续工作：

- Browser 插件工具本轮未暴露可调用接口，浏览器验证采用内置 Playwright 作为 fallback。
- `full_syntax.snl` 包含数组、记录和过程调用，当前 MIPS 后端会在 Codegen 阶段展示暂缓特性的诊断；这是后端支持范围的现状。
- `web/` 当前为零依赖原生实现，后续若要增加更强编辑体验，可单独引入 Monaco 或 CodeMirror。

### 2026-06-10 - README 补充：编译与演示功能说明

用户请求：

- 在 README 中写明演示功能开启、关闭和 CMake 编译方法。

预计修改的文件或模块：

- `AGENTS.MD`
- `README.md`

实现步骤：

- 新增项目 README。
- 记录 CMake 配置与编译命令。
- 记录 `snlc` 命令行用法。
- 记录 Web 演示功能启动命令、访问地址和关闭方式。
- 说明 `SNLC_BIN`、`HOST`、`PORT` 可选环境变量。

测试或验证方式：

- 确认 README 命令路径与当前项目结构一致。
- 运行 Markdown 基本内容检查。

重要假设：

- 文档面向 macOS/Linux shell；Windows 用户可按同等 CMake 和 Node 命令替换路径。

实际修改内容：

- 已新增 `README.md`。
- 已记录快速启动命令：CMake 配置、编译、启动 Web 服务。
- 已记录命令行编译器的 `--tokens`、`--ast`、`--semantic`、`--mips` 用法。
- 已记录 Web 演示功能开启方式、访问地址和关闭方式。
- 已记录 `PORT`、`HOST`、`SNLC_BIN` 可选配置。
- 已记录测试样例目录含义。

已运行的测试或检查：

- 已读取 `README.md`，确认内容完整。
- 已检查 `CMakeLists.txt`、`web/server.mjs`、`tests/valid`、`tests/mips` 路径存在。

未解决问题或后续工作：

- 暂无。

### 2026-06-11 - 第六轮实现：完整 MIPS 栈式后端一次性扩展

用户请求：

- 按“完整 MIPS 后端一次性实现计划”实现。
- 将当前只支持主程序全局变量的 MIPS 后端扩展到覆盖现有 lexer/parser/semantic 已支持的 SNL 子集。

预计修改的文件或模块：

- `AGENTS.MD`
- `src/mips.h`
- `src/mips.cpp`
- `src/semantic.cpp`
- `tests/mips/*.snl`
- `文档/MIPS_栈帧与调用约定.md`

实现步骤：

- 重构 MIPS 生成器，扫描主程序和过程 AST，建立过程标签、作用域、父作用域和局部区元数据。
- 设计并实现统一栈帧布局，保存旧 `$fp`、`$ra` 和静态链，使用 `$fp` 相对地址访问参数和局部变量。
- 实现统一左值地址生成，支持全局变量、局部变量、值参数、`var` 参数、数组元素和记录字段。
- 实现过程代码生成、过程调用、值参数传递、`var` 参数传地址、递归调用和嵌套过程静态链访问。
- 扩展 `read/write`、赋值、表达式、`if/while`、`return(expr)` 的代码生成，使其使用统一寻址与调用约定。
- 如语义阶段缺少后端必要绑定信息，补充内部语义属性，不改变 `--ast` 文本输出。
- 新增覆盖局部变量、数组、记录、过程调用、嵌套过程、递归和 `var` 参数的 MIPS 样例。
- 新增 MIPS 栈帧与调用约定文档。

测试或验证方式：

- 运行 `cmake --build build`。
- 回归运行 `--tokens`、`--ast`、`--semantic` 正常样例。
- 回归运行词法、语法、语义错误样例，确认仍返回非 0。
- 对所有 `tests/mips/*.snl` 生成汇编。
- 使用 MARS 对生成汇编执行汇编检查和模拟运行，核对输出。
- 对需要输入的样例提供固定输入并核对输出。
- 运行 `git diff --check`。

重要假设：

- “完整后端”指覆盖当前项目已实现语义分析能接受的 SNL 能力，不额外扩展语法或语义。
- 后端仍不引入独立 IR，直接基于语义后的 AST 和符号表生成 MIPS。
- 目标运行环境继续以 MARS 兼容 32 位 MIPS 为准。
- SNL 过程无返回值，`return(expr)` 只计算表达式并提前返回当前过程。

实际修改内容：

- 已重构 `src/mips.h` 和 `src/mips.cpp`，新增过程元数据、栈帧信息、作用域到过程映射、过程符号到标签映射和当前过程上下文。
- 已实现统一左值地址生成，支持全局变量、局部变量、值参数、`var` 参数、数组元素和记录字段。
- 已实现过程代码生成、过程调用、值参数传递、复合值参数拷贝、`var` 参数传地址、递归调用和嵌套过程 static link。
- 已扩展赋值、表达式、`read`、`write`、`if`、`while`、`return(expr)` 的 MIPS 生成，使其使用统一栈帧和寻址逻辑。
- 已新增 MIPS 样例：局部变量遮蔽、记录内数组和 `var` 参数、数组值参数、递归求和、嵌套过程非局部访问。
- 已新增 `文档/MIPS_栈帧与调用约定.md`，记录当前栈帧布局、参数传递、static link 和变量寻址规则。
- 本轮未修改 `src/semantic.cpp`；现有语义属性和后端按作用域查找已足够支撑本轮代码生成。

已运行的测试或检查：

- 已运行 `cmake --build build`，构建成功，产物为 `build/Debug/snlc.exe`。
- 已回归运行 `--tokens tests/valid/basic_tokens.snl`、`--ast tests/valid/full_syntax.snl`、`--semantic tests/valid/semantic_ok.snl`、`--semantic tests/valid/semantic_nested_ok.snl`，均退出码为 0。
- 已批量运行 `tests/lexical_errors/*.snl`、`tests/syntax_errors/*.snl`、`tests/semantic_errors/*.snl`，均保持非 0 退出码。
- 已对所有 `tests/mips/*.snl` 生成 `build/mips_tests/*.asm`，均成功。
- 已使用 MARS 对所有生成的 MIPS 样例执行汇编检查，均通过。
- 已使用 MARS 模拟运行基础样例：`basic_write` 输出 `14`，`char_write` 输出 `A`，`if_else` 输出 `10`，`while_sum` 输出 `15`，`read_add` 输入 `7` 输出 `12`。
- 已使用 MARS 模拟运行新增复杂样例：`local_shadow` 输出 `73`，`record_array_var_param` 输出 `45Z`，`value_array_param` 输出 `88`，`recursive_sum` 输出 `15`，`nested_nonlocal` 输出 `7`。
- 已验证 `tests/valid/semantic_ok.snl` 可生成并运行，输入 `5` 输出 `a`；`tests/valid/semantic_nested_ok.snl` 输出 `2`。
- 已验证 `tests/valid/full_syntax.snl` 可生成 MIPS 并通过 MARS 汇编检查。
- 已运行 `git diff --check`，未发现空白错误。

未解决问题或后续工作：

- 当前后端覆盖现有语法和语义阶段支持的主要 SNL 子集；如果后续语义阶段新增更多运算符或更复杂的记录嵌套字段，需要同步扩展 MIPS 地址生成。
- 记录/数组整体比较没有作为演示用例纳入；当前推荐在语义阶段继续约束比较表达式面向标量类型。
