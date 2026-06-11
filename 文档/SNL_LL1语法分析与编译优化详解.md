# SNL 编译器：LL(1) 语法分析与编译优化详解

---

## 第一部分：LL(1) 语法分析

### 1.1 什么是 LL(1)

**LL(1)** 三个字母的含义：

- 第一个 **L**：从左往右扫描输入（Left-to-right）
- 第二个 **L**：做最左推导（Leftmost derivation）
- **(1)**：每次做决定时只往前看 1 个 token

一句话概括：**LL(1) 解析器每次看到当前栈顶的非终结符，结合下一个 token（lookahead），就能唯一决定用哪条产生式展开，永远不需要回溯。**

### 1.2 两种实现方式

这个编译器里同时实现了**两种**解析器，用 `--recursive` 和 `--ll1` 切换：

| 方式 | 文件 | 思路 |
|---|---|---|
| 递归下降 | `src/parser.cpp` | 每个语法规则写成一个函数，函数之间互相调用 |
| LL(1) 表驱动 | `src/ll1_parser.cpp` | 用一个解析栈 + 一张预测分析表，机械化地驱动解析 |

递归下降的代码很直观——你看到 `parse_if_statement()` 这个函数，读进去就知道它在解析 if 语句。而 LL(1) 表驱动的方式更像一个"虚拟机"：一个 while 循环，不停地看栈顶和 lookahead，查表决定下一步做什么。

### 1.3 这个项目的 LL(1) 实现细节

#### 核心数据结构

文件位置：`src/ll1_parser.h:17-66`

```cpp
// 37 个非终结符
enum class NonTerminal {
    Program, ProgramHead, DeclarePart, TypeDec, TypeDecList, TypeName,
    BaseType, StructureType, ArrayType, RecordType, FieldDecList, FieldDec,
    IdList, IdListTail, VarDec, VarDecList, ProcDec, ProcDeclaration,
    ParamList, ParamMore, Param, ProgramBody, StmList, StmMore,
    StmMoreAfterSemi, Statement, IdStatementTail, VariableTail,
    FieldVar, FieldVarTail, ActParamList, ActParamMore,
    RelExp, CmpOp, Exp, ExpTail, Term, TermTail, Factor,
};

// 语法符号：可以是终结符，也可以是非终结符
struct Symbol {
    bool terminal;           // true = 终结符，false = 非终结符
    TokenType token;         // 如果是终结符，这是什么类型
    NonTerminal non_terminal; // 如果是非终结符，是哪个
};

// 产生式就是一个 Symbol 的序列（右部）
using Production = std::vector<Symbol>;
```

#### 预测分析表

位置：`src/ll1_parser.cpp:94-618` 的 `find_production()` 函数

传统教材里，预测分析表是一个二维数组：`table[NonTerminal][TokenType] = Production`。但这里没有真的建一张表，而是用了一个巨大的 **switch-case + if-else** 结构。每个非终结符是一个 case，里面根据 lookahead 的 token 类型返回对应的产生式。

举个例子——`Statement` 非终结符的"表项"（line 391-423）：

```cpp
case NT::Statement:
    if (lookahead == TT::If) {
        // 产生式: Statement -> if RelExp then StmList else StmList fi
        return production({make_n(NT::RelExp), make_t(TT::Then),
                           make_n(NT::StmList), make_t(TT::Else),
                           make_n(NT::StmList), make_t(TT::Fi)});
    }
    if (lookahead == TT::While) {
        // 产生式: Statement -> while RelExp do StmList endwh
        return production({...});
    }
    if (lookahead == TT::Read) {
        // 产生式: Statement -> read ( Id )
        return production({make_t(TT::Read), make_t(TT::LParen),
                           make_t(TT::Id), make_t(TT::RParen)});
    }
    // ... Write, Return, Id 的情况
    return nullptr;  // 表中没有这个条目 = 语法错误
```

如果某个非终结符 + lookahead 的组合在表中找不到产生式，返回 `nullptr`，解析器会报语法错误。

#### 解析主循环

位置：`src/ll1_parser.cpp:42-81`

```
栈初始状态 = [EOF, Program]

while (栈不为空):
    symbol = 栈顶.pop()

    if (symbol 是终结符):
        if (当前 token 匹配):
            消费 token，继续
        else:
            报错，停止
    else:
        查表: production = find_production(symbol, 当前 token 类型)
        if (找到了):
            弹出 symbol，把产生式右部逆序压栈
        else:
            报错，停止
```

用一个具体例子来感受一下。假设我们在解析 `if x < 5 then ...`，此时栈顶是 `Statement`，当前 token 是 `if`：

1. 弹栈，发现 `Statement` 是非终结符
2. 查 `find_production(Statement, if)` → 找到产生式
3. 把产生式右部逆序压栈：`Fi, StmList, Else, StmList, Then, RelExp, If`
4. 栈顶变成 `If`（终结符），当前 token 是 `if` → 匹配，消费
5. 栈顶变成 `RelExp`（非终结符）→ 查表继续展开...

#### 特殊处理：ε 产生式

很多语法规则是可选的，比如类型声明、变量声明。LL(1) 表里用**空产生式**（空的 `Production` 向量）来表示"选 ε"。

以 `TypeDec` 为例（line 128-138）：

```cpp
case NT::TypeDec:
    if (lookahead == TT::Type) {
        // 有类型声明：TypeDec -> type TypeDecList
        return production({make_t(TT::Type), make_n(NT::TypeDecList)});
    }
    if (lookahead == TT::Var || lookahead == TT::Procedure || lookahead == TT::Begin) {
        // 没有类型声明：TypeDec -> ε
        return production({});  // 空产生式!
    }
```

当 lookahead 是 `var`、`procedure` 或 `begin` 时，选择空产生式，意思是"这里没有类型声明部分"，直接跳过。

#### LL(1) 解析器与 AST 的关系

关键点（line 25-40）：**LL(1) 解析器本身只做语法验证，不构造 AST。**

```cpp
ParserResult LL1Parser::parse() {
    if (!parse_table()) {
        // LL(1) 验证失败，直接返回错误
        return result;
    }
    // LL(1) 验证通过了，把 token 交给递归下降解析器来建 AST
    Parser recursive_parser(tokens_);
    result = recursive_parser.parse();
}
```

为什么这么设计？因为递归下降解析器里已经写好了构建 AST 的逻辑（每个 `parse_xxx()` 函数返回 `AstNode`），再在 LL(1) 表驱动里重写一套 AST 构造代码就是重复劳动。所以这里的做法是：LL(1) 做"安检"，通过了就让递归下降做"施工"，两个解析器输出一致的 AST。

这就解释了为什么验证时可以 `diff` 两个解析器的输出——它们应该生成完全相同的 MIPS 汇编：

```bash
./build/snlc --mips tests/mips/optimized_codegen.snl -o build/optimized_codegen.asm
./build/snlc --mips --ll1 tests/mips/optimized_codegen.snl -o build/optimized_codegen_ll1.asm
diff -u build/optimized_codegen.asm build/optimized_codegen_ll1.asm  # 应该没有差异
```

---

## 第二部分：编译优化

### 2.1 优化在哪里做

一句话：**所有优化都在 MIPS 代码生成阶段（`src/mips.cpp`），没有独立的中间表示（IR）层。**

```
源码 → 词法分析 → 语法分析(AST) → 语义分析 → [优化嵌入在]MIPS代码生成 → 汇编输出
```

这意味着优化是**边生成边做**的。这样的好处是实现简单，不需要维护一套中间语言；局限是像公共子表达式消除、循环不变式外提这类需要跨语句分析的优化就不好做——它们更适合等有了三地址码 IR 之后再做。

下面逐一讲解四类已经实现的优化。

### 2.2 优化一：删除冗余赋值

**代码位置：** `src/mips.cpp:141-161`

**做了什么：** 在要生成 `x := expr` 的 MIPS 代码之前，先判断这个赋值是不是"做了等于没做"。如果是，**直接跳过，不生成任何代码**。

**判断逻辑在 `expression_preserves_storage()`（line 503-533）：**

```cpp
bool expression_preserves_storage(const AstNode& storage,
                                   const AstNode& expression) const {
    // 情况 1: x := x  左右两边是同一个变量 → 肯定冗余
    if (same_storage(storage, expression)) {
        return true;
    }

    // 情况 2: 表达式是 Op 运算，递归检查
    // x := (x + 0) → 看左边是不是 x+0，右边是不是常数0
    if ((op == "+" || op == "-") && right_constant == 0) {
        return expression_preserves_storage(storage, left);
    }
    // x := (0 + x) → 看右边是不是 0+x，左边是不是常数0
    if (op == "+" && left_constant == 0) {
        return expression_preserves_storage(storage, right);
    }
    // x := x * 1 或 x := x / 1
    if ((op == "*" || op == "/") && right_constant == 1) {
        return expression_preserves_storage(storage, left);
    }
    // x := 1 * x
    if (op == "*" && left_constant == 1) {
        return expression_preserves_storage(storage, right);
    }
}
```

**具体例子：**

| 源码语句 | 检测到的模式 | 优化结果 |
|---|---|---|
| `x := x` | `same_storage` 返回 true | 不生成任何代码 |
| `x := x + 0` | 递归检测：x+0 的右操作数是0，然后 left 是 x，匹配 storage | 不生成任何代码 |
| `x := x - 0` | 同上 | 不生成任何代码 |
| `x := x * 1` | 递归检测：右操作数是1 | 不生成任何代码 |
| `x := x / 1` | 递归检测：右操作数是1 | 不生成任何代码 |
| `x := x + 1` | 右操作数不是0也不是1 | **正常生成代码** |

注意 `* 0` 不会被删掉——`x := x * 0` 改变了 x 的值（变成0），不是冗余赋值。

**`same_storage()` 是怎么判断"同一个变量"的？** （line 488-501）

它检查两件事：
1. 节点的 `detail` 字段格式都是 `"x IdV"`（即都是简单变量引用，不是数组或记录字段）
2. 语义分析阶段填充的 `semantic_symbol_id` 相同，或者变量名字相同

### 2.3 优化二：常量条件分支消除

**代码位置：** `src/mips.cpp:203-246`

**做了什么：** 在生成 if 或 while 语句的代码之前，先尝试对条件表达式做编译期求值。如果条件在编译期就能确定真假，就直接生成对应的代码，跳过不可能执行的分支。

#### 对 if 语句（line 203-224）

```cpp
const std::optional<int> condition = constant_value(*statement.children[0]);
if (condition) {
    // 条件值在编译期就知道了！
    if (*condition != 0) {
        // 条件恒真 → 只生成 then 分支
        emit_statement_list(*statement.children[1]);
    } else {
        // 条件恒假 → 只生成 else 分支
        emit_statement_list(*statement.children[2]);
    }
    return;
}
// 条件不是常量 → 正常生成 if-then-else 的跳转代码
```

**具体例子：**

```pascal
if 1 = 0 then
    x := 99
else
    x := x
fi
```

编译器计算出 `1 = 0` 是假（值为 0），于是：
- 只生成 `x := x` 的代码（else 分支）
- 再结合优化一：`x := x` 又是冗余赋值 → **最终不生成任何代码！**

如果条件恒真，比如 `if 1 < 2 then ...`，就只生成 then 分支的代码，else 分支完全消失。

#### 对 while 语句（line 227-246）

```cpp
const std::optional<int> condition = constant_value(*statement.children[0]);
if (condition && *condition == 0) {
    // 条件恒假 → 循环永远不会执行 → 啥代码都不生成
    return;
}
// 条件不是常量假 → 正常生成 while 循环代码
```

**具体例子：**

```pascal
while 0 < 0 do
    x := x + 1
endwh
```

`0 < 0` 是假 → 循环体永远不会进入 → **整段 while 代码被删掉**，最终汇编里什么都没有。

注意：如果条件是恒真（比如 `while 1 = 1 do ...`），目前仍然会生成完整的 while 循环（会导致无限循环）。这是有意为之的——恒真的循环有时候是程序员故意的，编译期把它优化掉可能会改变程序语义。

### 2.4 优化三：常量表达式折叠（Constant Folding）

**代码位置：** `src/mips.cpp:254-258` 调用 `emit_expression()` 时的第一步，以及 `constant_value()` 函数（line 437-486）。

**做了什么：** 如果一个表达式（或子表达式）的所有操作数都是常量，就在编译期把它算出来，用 `li $t0, 结果` 替代一长串的加减乘除指令。

#### constant_value() 的核心逻辑

```cpp
std::optional<int> constant_value(const AstNode& expression) const {
    // 终端情况 1: 字符常量 → 转成 ASCII 值
    if (detail 以 "Const '" 开头) {
        return expression.detail[7] 的 unsigned char 值;
    }

    // 终端情况 2: 整数常量 → 直接转成 int
    if (detail 以 "Const " 开头) {
        return stoi(detail.substr(6));
    }

    // 递归情况: 操作符节点 → 先求左右子树的值
    if (detail 以 "Op " 开头) {
        left = constant_value(左子树);
        right = constant_value(右子树);
        if (left 和 right 都有值) {
            计算 left op right，返回结果;
        }
    }

    // 操作数中有变量 → 无法在编译期求值
    return std::nullopt;
}
```

**具体例子：**

```pascal
x := 2 + 3 * 4
```

AST 结构大概是：

```
  Op +
  ├── Const 2
  └── Op *
      ├── Const 3
      └── Const 4
```

生成代码时：
1. `emit_expression()` 先调用 `constant_value()` 对整个 `2 + 3 * 4` 求值
2. `constant_value` 递归进入 `Op +`：
   - 左操作数 `Const 2` → 返回 `2`
   - 右操作数 `Op *` → 递归进入：
     - `Const 3` → 返回 `3`
     - `Const 4` → 返回 `4`
     - `3 * 4 = 12`
   - `2 + 12 = 14`
3. 得到 `14` → 直接生成 `li $t0, 14`

**对比没有优化时**，这个表达式会生成：
```asm
    li $t0, 2        # Const 2
    sw $t0, 0($sp)   # 压栈
    li $t0, 3        # Const 3
    sw $t0, 0($sp)   # 压栈
    li $t0, 4        # Const 4
    lw $t1, 0($sp)   # 弹栈
    mul $t0, $t1, $t0  # 3 * 4 = 12
    lw $t1, 0($sp)   # 弹栈
    addu $t0, $t1, $t0 # 2 + 12 = 14
```

有优化后，这 9 条指令变成 1 条 `li $t0, 14`。

**支持的运算：**

| 运算符 | 计算方式 | 特殊处理 |
|---|---|---|
| `+` | `left + right` | - |
| `-` | `left - right` | - |
| `*` | `left * right` | - |
| `/` | `left / right` | 除数为 0 时返回 `nullopt`（不折叠，保持运行时除零行为） |
| `<` | `left < right ? 1 : 0` | - |
| `=` | `left == right ? 1 : 0` | - |

**为什么不折叠除零？** 因为在编译期触发除零是一种未定义行为，而原程序在运行时触发除零可能被操作系统捕获或导致程序终止。折叠除零等于改变了程序的运行时行为，不应该做。

### 2.5 优化四：代数恒等式化简

**代码位置：** `src/mips.cpp:274-277` 和 `emit_simplified_expression()` 函数（line 324-377）。

**做了什么：** 当表达式包含变量、不能完全折叠为常量时，利用代数恒等式来简化代码。这发生在 `emit_expression()` 遇到 `Op` 节点时：

```cpp
if (starts_with(expression.detail, "Op ")) {
    if (emit_simplified_expression(expression)) {
        return;   // 化简成功，已经生成了更短的代码
    }
    // 化简不了，走正常的栈式运算生成
}
```

#### 化简规则一览

| 原始表达式 | 化简为 | 说明 |
|---|---|---|
| `x + 0` | `x` | 任何数加 0 等于本身 |
| `0 + x` | `x` | 0 加任何数等于本身 |
| `x - 0` | `x` | 任何数减 0 等于本身 |
| `x * 1` | `x` | 乘 1 是恒等式 |
| `1 * x` | `x` | 同上 |
| `x * 0` | `li $t0, 0` | 任何数乘 0 等于 0 |
| `0 * x` | `li $t0, 0` | 同上 |
| `x / 1` | `x` | 除 1 是恒等式 |
| `x = x` | `li $t0, 1` | 同一个变量跟自己做相等比较，恒真 |
| `x < x` | `li $t0, 0` | 同一个变量跟自己做小于比较，恒假 |

判断"x = x"和"x < x"用的是 `same_storage()` 函数——只要左右操作数指向同一个符号就行。

**注意化简和常量折叠的区别：**

- **常量折叠**在 `emit_expression()` 的**最开始**就做了（line 255）。如果整个表达式都是常量，根本不会走到 `emit_simplified_expression()`。
- **代数化简**处理的是"常量 + 变量"混在一起的表达式，比如 `a := b * 1 + 0`。

以 `a := b * 1 + 0` 为例，看代码生成过程：

1. 先对整棵 `Op +` 调用 `constant_value()` → 因为含变量 `b`，返回 `nullopt`（折叠失败）
2. 进入 `emit_simplified_expression()`：
   - 检查 `Op +`，右操作数是 `Const 0` → `x + 0` 模式匹配！生成 `b * 1` 的代码
   - 递归生成 `b * 1`：
     - `constant_value` 失败（含变量 `b`）
     - `emit_simplified_expression` 检查 `Op *`，右操作数是 `Const 1` → `x * 1` 模式匹配！生成 `b` 的代码
     - `b` 是变量 → `lw $t0, _snl_var_0_b`
3. 最终产物：只有一条 `lw` 指令，后面接 `sw` 存给 `a`

### 2.6 辅助函数汇总

所有优化都依赖于这几个辅助函数，它们都在 `src/mips.h:40-43` 声明，在 `src/mips.cpp` 后半部分实现。

| 函数 | 位置 | 作用 |
|---|---|---|
| `constant_value()` | `mips.cpp:437` | 递归计算表达式的编译期常量值，返回 `optional<int>` |
| `same_storage()` | `mips.cpp:488` | 判断两个 AST 节点是否引用同一个变量 |
| `expression_preserves_storage()` | `mips.cpp:503` | 递归判断表达式是否保持变量值不变（用于检测冗余赋值） |
| `emit_simplified_expression()` | `mips.cpp:324` | 检测代数恒等式模式并生成简化代码，返回 `true` 表示化简成功 |

### 2.7 优化流程总结

`emit_expression()` 函数里的决策流程如下：

```
emit_expression(expr)
  │
  ├─ 1. constant_value(expr) 能求值？
  │     └─ 是 → li $t0, 结果 → 结束
  │
  ├─ 2. 是字符常量？
  │     └─ 是 → li $t0, ASCII值 → 结束
  │
  ├─ 3. 是整数常量？
  │     └─ 是 → li $t0, 值 → 结束
  │
  ├─ 4. 是 Op 操作符？
  │     ├─ emit_simplified_expression(expr) 化简成功？
  │     │     └─ 是 → 结束（已生成简化代码）
  │     │
  │     └─ 否 → 正常栈式运算：
  │             左子树 → 压栈 → 右子树 → 弹栈 → 运算 → 结束
  │
  └─ 5. 是变量引用？
        └─ lw $t0, 变量标签 → 结束
```

### 2.8 测试用例详解

测试文件：`tests/mips/optimized_codegen.snl`

```pascal
program optimizedCodegen
var integer x;
begin
    x := 2 + 3 * 4;       -- 行 4
    x := x + 0;            -- 行 5
    if 1 = 0 then          -- 行 6
        x := 99            -- 行 7
    else
        x := x             -- 行 9
    fi;
    while 0 < 0 do         -- 行 11
        x := x + 1         -- 行 12
    endwh;
    write(x)               -- 行 14
end.
```

逐行分析优化过程：

**行 4：`x := 2 + 3 * 4`**
- 优化三（常量折叠）：`2 + 3 * 4` 在编译期算出 `14`
- 生成：`li $t0, 14` + `sw $t0, _snl_var_0_x`

**行 5：`x := x + 0`**
- 优化一（冗余赋值删除）：`expression_preserves_storage()` 检测到 `x + 0` 保持 `x` 的值不变
- 结果：**不生成任何代码**

**行 6-10：`if 1 = 0 then x := 99 else x := x fi`**
- 优化二（常量分支消除）：`1 = 0` 编译期求值为 0（假）
- 只走 else 分支：`x := x`
- 优化一（冗余赋值删除）：`x := x` 又是冗余赋值
- 结果：**整个 if 语句不生成任何代码**

**行 11-13：`while 0 < 0 do x := x + 1 endwh`**
- 优化二（常量分支消除）：`0 < 0` 编译期求值为 0（假）
- 结果：**整段 while 循环被删除**

**行 14：`write(x)`**
- 正常生成 write 代码
- `x` 的值来自行 4 的赋值，即 `14`
- 生成：`lw $t0, _snl_var_0_x` → `move $a0, $t0` → `li $v0, 1` → `syscall`

**最终生成的汇编（精华版）：**

```asm
.data
_snl_var_0_x: .word 0

.text
.globl main
main:
    li $t0, 14
    sw $t0, _snl_var_0_x
    lw $t0, _snl_var_0_x
    move $a0, $t0
    li $v0, 1
    syscall
    li $v0, 10
    syscall
```

只有 8 条指令。如果没有优化，行 4 的 `2 + 3 * 4` 就需要 9 条指令，加上 if/while 的分支跳转，会多出十几条指令。

---

## 第三部分：两种解析器的关系

你可能注意到了：LL(1) 表驱动解析器只做**验证**，不构造 AST。验证通过后，把 token 交给递归下降解析器来建 AST。所以不管用 `--recursive` 还是 `--ll1`，生成的 MIPS 代码是**完全一样的**。

这也是一种常见的工程实践——先用 LL(1) 表驱动验证一遍来证明你的语法确实是 LL(1) 的（能放进预测分析表而没有任何冲突），然后实际构建 AST 的任务交给递归下降（因为它构建 AST 的代码写起来更自然）。

验证方法：
```bash
./build/snlc --mips tests/mips/optimized_codegen.snl -o out1.asm
./build/snlc --mips --ll1 tests/mips/optimized_codegen.snl -o out2.asm
diff out1.asm out2.asm  # 应该没有输出 = 完全一致
```

---

## 第四部分：当前优化的局限性

这些优化都是在 MIPS 代码生成阶段做的，有一些天然的限制：

1. **没有数据流分析**：不知道变量在什么时候被定义、什么时候被使用，所以只能在"看得到"的范围内做优化（单条语句级别）

2. **没有基本块概念**：不知道控制流图长什么样，所以做不到跨语句的优化

3. **限于简单变量**：`same_storage()` 只认 `x IdV` 格式的简单变量，数组和记录字段的等价性检测不了

4. **未来可以扩展的方向**：
   - **公共子表达式消除（CSE）**：`a := b + c; d := b + c` → 第二行可以直接用 `a`，需要 IR 层
   - **循环不变式外提（LICM）**：把循环内不随迭代变化的部分提到循环外面，需要 IR 层
   - **死代码删除（DCE）**：删掉计算结果永远不会被使用的代码，需要跨语句的数据流分析
