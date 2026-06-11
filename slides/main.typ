#import "@preview/touying:0.7.3": *
#import themes.simple: *

#show: simple-theme.with(aspect-ratio: "16-9")

#set text(
  font: "Source Han Serif SC",
  size: 16pt,
  lang: "zh",
)

#let navy = rgb("#1f3552")
#let ink = rgb("#1f2933")
#let muted = rgb("#607080")
#let paper = rgb("#f7f4ee")
#let line = rgb("#d8cfc0")
#let accent = rgb("#b65f44")
#let green = rgb("#4f7f68")
#let blue = rgb("#3d6f91")

#set page(fill: paper, margin: 0.65in)
#set par(leading: 0.8em)
#show strong: set text(fill: navy, weight: "semibold")
#show heading: set text(fill: navy, weight: "bold")
#show list: set block(spacing: 0.55em)

#let tag(label, color: accent) = rect(
  fill: color.lighten(82%),
  stroke: color.lighten(35%),
  radius: 5pt,
  inset: (x: 9pt, y: 5pt),
  text(fill: color.darken(15%), size: 12pt, weight: "semibold", label),
)

#let card(title, body, color: navy) = rect(
  fill: white,
  stroke: line,
  radius: 6pt,
  inset: 13pt,
  width: 100%,
  [
    #text(fill: color, size: 17pt, weight: "bold")[#title]
    #v(0.35em)
    #text(fill: ink, size: 14.5pt)[#body]
  ],
)

#let flow(items) = grid(
  columns: items.len(),
  gutter: 0.22in,
  ..items.enumerate().map(((i, item)) => [
    #rect(
      fill: white,
      stroke: line,
      radius: 6pt,
      inset: 10pt,
      width: 100%,
      [
        #text(fill: accent, size: 22pt, weight: "bold")[#str(i + 1)]
        #v(0.25em)
        #text(fill: navy, size: 14pt, weight: "bold")[#item]
      ],
    )
  ]),
)

#let node(label, note: none, color: navy, fill: white) = rect(
  fill: fill,
  stroke: color.lighten(35%),
  radius: 6pt,
  inset: 10pt,
  width: 100%,
  [
    #text(fill: color, size: 14pt, weight: "bold")[#label]
    #if note != none [
      #v(0.2em)
      #text(fill: muted, size: 11.5pt)[#note]
    ]
  ],
)

#let arrow(label: none, color: muted) = align(
  center + horizon,
  [
    #text(fill: color, size: 20pt, weight: "bold")[→]
    #if label != none [
      #v(-0.25em)
      #text(fill: color, size: 9.5pt)[#label]
    ]
  ],
)

#let stack-cell(body, color: navy, fill: white) = rect(
  fill: fill,
  stroke: color.lighten(35%),
  radius: 4pt,
  inset: (x: 8pt, y: 5pt),
  width: 100%,
  align(center, text(fill: color, size: 12.5pt, weight: "semibold", body)),
)

= SNL 编译器核心实现

#v(0.7in)
#text(size: 25pt, fill: navy, weight: "bold")[从源码到 MIPS]

#v(0.35in)
#text(size: 15pt, fill: muted)[词法分析器 · 递归下降语法分析器 · LL(1) 语法分析器 · 语义分析器 · MIPS 代码生成 · 简单优化]

#v(0.8in)
#flow(("Token 序列", "AST", "符号与类型", "MIPS 汇编", "优化输出"))

== 汇报范围

#grid(
  columns: (1fr, 1fr, 1fr),
  gutter: 0.25in,
  card([词法分析器], [手写扫描器，识别 SNL 关键字、标识符、常量、运算符、界符和注释。], color: accent),
  card([语法分析器], [递归下降负责构造 AST；LL(1) 表驱动分析负责预测分析验证。], color: blue),
  card([语义分析器], [建立作用域符号表，完成类型检查、声明检查和调用检查。], color: green),
)

#v(0.25in)
#grid(
  columns: (1fr, 1fr),
  gutter: 0.25in,
  card([MIPS 生成], [根据 AST 与符号表生成数据段、文本段、控制流和表达式指令。], color: navy),
  card([代码生成优化], [在汇编发射前完成常量折叠、代数化简、死分支删除和冗余赋值删除。], color: accent),
)

== 编译流程总览

#v(0.2in)
#flow(("源程序", "词法分析", "语法分析", "语义分析", "MIPS 生成", "优化汇编"))

#v(0.45in)
#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag[输入与中间结果]
    - 源程序以文本形式读入。
    - 词法阶段输出 Token 序列。
    - 语法阶段输出统一 AST。
    - 语义阶段在 AST 上补充类型和符号编号。
  ],
  [
    #tag([后端使用的信息], color: blue)
    - 符号表提供变量大小、作用域和存储标签。
    - AST 节点描述语句和表达式结构。
    - 语义标注决定输出 syscall 类型和可赋值性。
  ],
)

== 词法分析器

#grid(
  columns: (0.9fr, 1.1fr),
  gutter: 0.35in,
  [
    #tag[实现位置]
    #text(fill: muted, size: 14pt)[src/lexer.cpp, src/token.cpp]

    #v(0.25in)
    - 采用单遍扫描，维护当前位置、行号和列号。
    - 使用关键字表区分保留字和普通标识符。
    - 整数常量、字符常量和标识符保留词素值。
    - 支持大括号注释跳过。
    - 发现非法字符、未闭合注释、非法字符常量时生成诊断。
  ],
  [
    #image("Lexer 扫描.png", height: 5.15in)
  ],
)

== 词法分析的设计重点

#grid(
  columns: (1fr, 1fr, 1fr),
  gutter: 0.25in,
  card([确定性扫描], [每次读取一个字符，根据字符类别进入对应识别过程；无回溯，状态简单。], color: accent),
  card([位置精确], [Token 和错误都携带行列号，便于命令行输出和后续定位。], color: blue),
  card([前端隔离], [词法错误存在时不进入语法阶段，避免后续阶段基于错误输入继续扩散。], color: green),
)

#v(0.38in)
#rect(
  fill: white,
  stroke: line,
  radius: 6pt,
  inset: 16pt,
  width: 100%,
  [
    #text(fill: navy, weight: "bold")[Token 类型覆盖]
    #v(0.25em)
    关键字、标识符、整数常量、字符常量、赋值符号、比较符号、算术运算符、括号、数组下标符号、分号、逗号、范围符号和文件结束符。
  ],
)

== 递归下降语法分析器

#grid(
  columns: (0.92fr, 1.08fr),
  gutter: 0.35in,
  [
    #image("递归下降分析器.png", height: 5.15in)
  ],
  [
    #tag([实现位置], color: blue)
    #text(fill: muted, size: 14pt)[src/parser.cpp, src/ast.cpp]

    #v(0.25in)
    - 每个主要非终结符对应一个解析函数。
    - 通过当前 Token 判断语句类型并分派。
    - 分析过程中直接构造 AST，节点类型包括程序、声明、语句和表达式。
    - 表达式按优先级分层处理：关系表达式、加减表达式、乘除项、因子。
    - 遇到语法错误时记录诊断，并尝试同步到分号或结构结束符。
  ],
)

== 递归下降分析结果

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag[AST 结构]
    - `ProK` 表示完整程序。
    - `PheadK` 保存程序名。
    - `TypeK`、`VarK`、`ProcDecK` 表示声明。
    - `StmLK`、`StmtK` 表示语句序列和具体语句。
    - `ExpK` 表示常量、变量和运算表达式。
  ],
  [
    #tag([优势], color: green)
    - 和文法结构对应直观，便于调试。
    - 构造 AST 过程自然嵌入语法分析。
    - 错误恢复逻辑可以按语句边界处理。
    - 后续语义分析和代码生成只依赖统一 AST。
  ],
)

== LL(1) 语法分析器

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag([核心机制], color: accent)
    - 使用预测分析栈模拟最左推导。
    - 栈底为文件结束符，起始非终结符为 Program。
    - 根据“非终结符 + 当前 Token”查询产生式。
    - 终结符匹配成功后前进到下一个 Token。
    - 无可用产生式或终结符不匹配时报告语法错误。
  ],
  [
    #tag([与递归下降的关系], color: blue)
    - LL(1) 先验证输入是否满足预测分析表。
    - 验证通过后复用递归下降分析器生成 AST。
    - 两种模式共用后续语义分析和代码生成。
  ],
)

#v(0.32in)
#flow(("输入 Token", "预测分析栈", "产生式选择", "匹配终结符", "复用 AST 构造"))

== LL(1) 文法改写

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag([左递归消除], color: accent)
    - 原文法中 `Exp → Exp + Term` 和 `Term → Term * Factor` 存在直接左递归。
    - LL(1) 无法处理左递归，必须改写为右递归 + 空产生式形式。
    - 改写后展开的第一个符号不再是自身，消除死循环。
  ],
  [
    #tag([改写后的等价文法], color: blue)
    - `Exp → Term ExpTail`
    - `ExpTail → + Term ExpTail | - Term ExpTail | ε`
    - `Term → Factor TermTail`
    - `TermTail → * Factor TermTail | / Factor TermTail | ε`
    - `Factor → ( Exp ) | IntConst | CharConst | Id VariableTail`
  ],
)

#v(0.28in)
#rect(
  fill: white,
  stroke: line,
  radius: 6pt,
  inset: 14pt,
  width: 100%,
  [
    #text(fill: navy, weight: "bold")[改写原则]
    #v(0.2em)
    引入尾递归符号（ExpTail / TermTail）将原左递归结构转为右递归。优先级由文法层次决定：Factor → Term → Exp，比较运算在 RelExp 层统一处理。
  ],
)

== LL(1) 预测分析表

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag([实现方式], color: accent)
    - 预测分析表没有使用二维数组，而是实现为一个 `switch-case` 函数 `find_production()`。
    - 输入为当前非终结符 + lookahead Token 类型。
    - 每个 `case` 分支按 lookahead 值返回对应的产生式右部。
    - 找不到匹配时返回 `nullptr`，触发语法错误。
  ],
  [
    #tag([设计考量], color: blue)
    - switch-case 结构使预测关系一目了然，每个 case 即代表表中一项。
    - C++ 的 static local 变量使产生式在首次匹配时构造，后续调用直接返回指针。
    - 免去初始化大二维数组的代码，且编译期即可发现遗漏分支。
  ],
)

#v(0.28in)
#grid(
  columns: (1fr, 0.16fr, 1fr),
  gutter: 0.12in,
  node([非终结符], note: [e.g., NT::Program], color: navy),
  arrow(label: [lookup]),
  node([产生式右部], note: [Symbol 序列], color: accent, fill: accent.lighten(88%)),
)

== LL(1) 核心算法

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag([数据结构], color: accent)
    #v(0.15em)
    ```cpp
    struct Symbol {
        bool terminal;
        TokenType token;
        NonTerminal non_terminal;
    };
    using Production = std::vector<Symbol>;
    ```
    - Symbol 统一表示终结符和非终结符，通过 `terminal` 字段区分。
    - Production 即产生式右部的符号序列。
  ],
  [
    #tag([核心循环: parse_table()], color: blue)
    - 初始化栈：`[EOF, Program]`
    - 循环：弹出栈顶 → 判断符号类型
    - 终结符：匹配当前 Token，成功则前进；失败则报错
    - 非终结符：查 `find_production()`，将右部逆序压入栈
    - $epsilon$ 产生式：右部为空 vector，不压入任何符号
    - 栈为空且 Token 已读完 → 成功
  ],
)

#v(0.28in)
#rect(
  fill: white,
  stroke: line,
  radius: 6pt,
  inset: 14pt,
  width: 100%,
  [
    #text(fill: navy, weight: "bold")[模拟过程示例: `read(x)`]
    #v(0.15em)
    `[EOF, Program]` → `[EOF, ProgramHead, DeclarePart, ProgramBody, ., EOF]` → ... → `[..., Id, IdStatementTail]` 匹配 `read`? 否 → 展开 `Statement` → `Read ( Id )` → 匹配 `read`, `(`, `x`, `)` ✓
  ],
)

== 语义分析器

#grid(
  columns: (0.95fr, 1.05fr),
  gutter: 0.35in,
  [
    #image("语义分析器.png", height: 5.05in)
  ],
  [
    - 以 AST 为输入，自顶向下处理声明和语句。
    - 建立基础类型、数组类型、记录类型、过程类型和别名类型。
    - 在表达式节点上写入语义类型、是否左值、符号编号和变量种类。
    - 对赋值、读写、条件、循环和过程调用执行静态检查。
    #image("语义分析器说明.png", height: 45%)
  ],
)


== 符号表与作用域

#grid(
  columns: (0.35fr, 0.95fr),
  gutter: 0.35in,
  [
    #tag([符号表内容], color: blue)
    - 符号种类：类型、变量、过程、参数。
    - 符号属性：名称、类型、作用域层级、偏移、大小、是否 var 参数。
    - 作用域以栈方式进入和退出。
    - 查找时先查当前作用域，再沿父作用域向外查找。
    - 同一作用域内重复定义会被诊断。
  ],
  [
    #image("符号表.png", width: 100%)
  ],
)

== 语义检查覆盖范围

#grid(
  columns: (1fr, 1fr, 1fr),
  gutter: 0.25in,
  card([声明检查], [重复定义、未声明标识符、非法类型名。], color: accent),
  card([类型检查], [赋值两侧类型、算术操作数、比较操作数、条件表达式类型。], color: blue),
  card([复合结构], [数组下标必须为整数，记录字段必须存在。], color: green),
)

#v(0.25in)
#grid(
  columns: (1fr, 1fr),
  gutter: 0.25in,
  card([过程调用], [检查被调用对象是否为过程，参数数量、参数类型和 var 参数可赋值性。], color: navy),
  card([后端标注], [为表达式节点保存类型和符号编号，使代码生成不再重复做名字解析。], color: accent),
)

== 基础 MIPS 代码生成

#grid(
  columns: (0.5fr, 0.95fr),
  gutter: 0.35in,
  [
    #tag([实现位置], color: navy)
    #text(fill: muted, size: 14pt)[src/mips.cpp]

    #v(0.25in)
    - 根据全局符号生成 `.data` 段变量标签。
    - 主程序生成 `.text` 段和 `main` 入口。
    - 表达式结果统一放在 `$t0`。
    - 二元表达式用栈临时保存左操作数。
    - `read` 和 `write` 通过 MIPS syscall 实现。
    - `if` 和 `while` 使用自动生成的唯一标签组织跳转。
  ],
  [
    #image("MIPS 实现.png", width: 100%)
  ],
)

== MIPS 生成覆盖的语言结构

#grid(
  columns: (1fr, 1fr, 1fr),
  gutter: 0.25in,
  card([数据对象], [主程序全局变量，按符号表大小分配 `.word` 或 `.space`。], color: blue),
  card([表达式], [整数、字符、变量、加减乘除、小于比较和等于比较。], color: accent),
  card([语句], [赋值、读入、输出、条件分支和 while 循环。], color: green),
)

#v(0.35in)
#rect(
  fill: white,
  stroke: line,
  radius: 6pt,
  inset: 16pt,
  width: 100%,
  [
    #text(fill: navy, weight: "bold")[当前边界]
    #v(0.25em)
    MIPS 后端聚焦基础可运行闭环，局部变量、过程调用栈帧、复杂数组和记录地址计算仍可作为后续扩展方向。
  ],
)

== 代码生成优化

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag([优化位置], color: accent)
    - 优化放在 MIPS 发射前。
    - 不改变词法、语法和语义分析结果。
    - 直接减少最终汇编指令数量。
    - 适合当前尚未引入独立四元式 IR 的工程结构。
  ],
  [
    #tag([优化内容], color: green)
    - 常量表达式折叠。
    - 代数恒等式化简。
    - 冗余赋值删除。
    - 常量条件分支消除。
    - 恒假循环删除。
  ],
)

#v(0.35in)
#flow(("AST 表达式", "常量求值", "恒等式判断", "语句可达性", "精简 MIPS"))

== 优化判定流程

#grid(
  columns: (1fr, 0.16fr, 1fr, 0.16fr, 1fr, 0.16fr, 1fr),
  gutter: 0.12in,
  node([语义后 AST], note: [类型与符号编号已标注], color: navy),
  arrow(),
  node([常量求值], note: [递归判断表达式是否可折叠], color: accent),
  arrow(),
  node([恒等式匹配], note: [识别零、一、自身比较], color: blue),
  arrow(),
  node([发射或跳过], note: [生成更短 MIPS 或删除语句], color: green),
)

#v(0.35in)
#grid(
  columns: (1fr, 1fr),
  gutter: 0.28in,
  [
    #tag([表达式级优化], color: accent)
    - 常量子树直接折叠为一个立即数。
    - 加零、减零、乘一、除一直接使用原操作数。
    - 乘零直接生成零值。
    - 自身相等为真，自身小于为假。
  ],
  [
    #tag([语句级优化], color: green)
    - 赋值右侧与左侧等价时整条赋值删除。
    - 常量 `if` 只保留可达分支。
    - 恒假 `while` 直接删除循环。
    - 优化失败时回退到普通 MIPS 生成逻辑。
  ],
)

== 优化规则示意

#table(
  columns: (1.1fr, 1.35fr, 1.35fr),
  inset: 9pt,
  stroke: line,
  fill: (_, y) => if y == 0 { accent.lighten(78%) } else { white },
  [类别], [识别对象], [生成策略],
  [常量折叠], [所有操作数都是常量的表达式], [直接生成立即数加载],
  [代数化简], [加零、减零、乘一、除一、乘零], [跳过二元运算或生成零值],
  [冗余赋值], [左值与右侧表达式保持同一存储值], [整条赋值不发射汇编],
  [常量分支], [条件表达式可在编译期确定], [只生成可达分支],
  [恒假循环], [循环条件编译期为假], [不生成循环标签与循环体],
)

#v(0.3in)
#grid(
  columns: (1fr, 0.15fr, 1fr, 0.15fr, 1fr),
  gutter: 0.12in,
  node([原始 AST], note: [包含可优化表达式], color: navy),
  arrow(label: [分析]),
  node([优化判定], note: [常量 / 恒等式 / 可达性], color: accent, fill: accent.lighten(88%)),
  arrow(label: [输出]),
  node([精简汇编], note: [更少加载、运算和跳转], color: green, fill: green.lighten(88%)),
)

== 优化效果

#grid(
  columns: (1fr, 1fr, 1fr),
  gutter: 0.25in,
  card([常量折叠], [编译期可求值的整数和字符表达式，直接变为立即数加载。], color: accent),
  card([代数化简], [加零、减零、乘一、除一、乘零等表达式不再生成完整二元运算。], color: blue),
  card([控制流精简], [常量 `if` 只生成可达分支，恒假 `while` 不生成循环体。], color: green),
)

#v(0.35in)
#rect(
  fill: white,
  stroke: line,
  radius: 6pt,
  inset: 16pt,
  width: 100%,
  [
    #text(fill: navy, weight: "bold")[验证样例]
    #v(0.25em)
    `tests/mips/optimized_codegen.snl` 用于覆盖常量折叠、恒假分支、恒假循环和无效赋值删除；递归下降与 LL(1) 模式生成结果保持一致。
  ],
)

== 测试与验证

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag([测试分类], color: blue)
    - `tests/valid`：正确程序。
    - `tests/lexical_errors`：词法错误。
    - `tests/syntax_errors`：语法错误。
    - `tests/semantic_errors`：语义错误。
    - `tests/mips`：MIPS 生成和运行样例。
  ],
  [
    #tag([验证方式], color: accent)
    - 词法阶段检查 Token 输出和错误诊断。
    - 语法阶段比较 AST 与预期结构。
    - 语义阶段检查符号表和错误报告。
    - 后端阶段生成 MIPS 汇编。
    - LL(1) 与递归下降模式做一致性验证。
  ],
)

== 总结

#grid(
  columns: (1fr, 1fr),
  gutter: 0.32in,
  [
    #tag[已完成]
    - SNL 前端基础功能完整。
    - 两种语法分析模式均已实现。
    - 语义分析支持作用域、类型和调用检查。
    - 基础 MIPS 后端形成可运行闭环。
    - 简单优化减少目标代码冗余。
  ],
  [
    #tag([后续方向], color: green)
    - 引入四元式或三地址码中间表示。
    - 在 IR 层实现公共表达式消除。
    - 支持循环不变式外提。
    - 完善过程调用栈帧和局部变量后端。
    - 扩展数组和记录的地址计算。
  ],
)
