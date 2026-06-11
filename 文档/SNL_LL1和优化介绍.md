# SNL 编译器：LL(1) 语法分析 & 编译优化 要点介绍

---

## 一、LL(1) 语法分析

### 对文法的处理

LL(1) 不能处理左递归，所以在文法层面做了改写：

```
原文法（有左递归）           改写后（LL(1) 可用）
Exp  → Exp + Term | ...     Exp      → Term ExpTail
                            ExpTail  → + Term ExpTail | - Term ExpTail | ε
Term → Term * Factor | ...  Term     → Factor TermTail
                            TermTail → * Factor TermTail | / Factor TermTail | ε
```

Exp 展开后第一个符号不再是 Exp 自己，消除了死循环。

### 实现方式

预测分析表没有用二维数组，而是写成了一个巨大的 `switch-case` 函数——`find_production()`。输入是非终结符 + lookahead，输出对应的产生式。

### 核心算法

一个栈 `[EOF, Program]` + 一个 while 循环：

- 栈顶是**终结符** → 匹配当前 token，消费
- 栈顶是**非终结符** → 查 `find_production()`，把产生式右部**逆序**压入栈
- 找到 ε（空产生式）→ 什么也不压，原地跳过
- 查不到 → 语法错误

### 关键设计

LL(1) 解析器**只做语法验证，不构建 AST**。验证通过后，把 token 交给递归下降解析器建 AST。两种解析器生成完全一致的输出，可用 `diff` 验证：

```bash
./snlc --mips test.snl      -o out1.asm
./snlc --mips --ll1 test.snl -o out2.asm
diff out1.asm out2.asm  # 无差异
```

---

## 二、编译优化

四类优化全部嵌入在 MIPS 代码生成阶段（`src/mips.cpp`），无需独立 IR 层。

### 1. 常量表达式折叠

`2 + 3 * 4` 编译期算出 `14`，生成 `li $t0, 14`。如果操作数含变量则放弃折叠。除零时也放弃折叠，保留运行时行为。

### 2. 代数恒等式化简

`x + 0` → `x`、`x * 1` → `x`、`x * 0` → `li $t0, 0`、`x / 1` → `x`。还有 `x = x` → 恒真 `1`、`x < x` → 恒假 `0`。

### 3. 删除冗余赋值

生成 `x := expr` 之前先判断赋值是否无意义。`x := x`、`x := x + 0`、`x := x * 1` 等直接跳过，不生成任何代码。

### 4. 常量条件分支消除

`if 1 = 0 then A else B fi` → 条件恒假，只生成 B。`while 0 < 0 do ... endwh` → 条件恒假，整段循环删除。

---

## 三、四类优化的协同

以测试用例为例：

```pascal
x := 2 + 3 * 4;      → 折叠：li $t0, 14
x := x + 0;          → 冗余：全删
if 1 = 0 then
    x := 99
else
    x := x            → 分支消除只走 else + 冗余删除 → 全删
fi;
while 0 < 0 do       → 恒假 → 整段删除
    x := x + 1
endwh;
write(x)             → 输出 14
```

四类优化全部命中，14 行源码最终只生成 8 条汇编指令。
