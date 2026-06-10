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

- 语法分析只实现递归下降方法。
- 不实现 LL(1) 表驱动语法分析，除非用户后续重新要求。
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
