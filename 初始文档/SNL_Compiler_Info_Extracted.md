# 《编译程序的设计与实现》SNL 编译器实现信息提取

> 来源：用户上传的《编译程序的设计与实现(书稿电子版)JPG(1).pdf》扫描版。  
> 说明：该 PDF 为图片扫描版，无可解析文本层；以下内容基于对相关页面的 OCR/人工校读式提取。  
> 原则：只记录 PDF 原文能支撑实现的规则；未讲清楚处标注“未找到”。

---

## 1. SNL 词法定义

### 1.1 字符表

**页码：书内页 6**

SNL 字符表包含：

```text
a b c ... z
A B C ... Z
0 1 2 ... 9
+ - * / < = ( ) [ ] . ; EOF 空白字符
```

原文说明：

```text
程序中英文字母区分大小写；
保留字只能由小写字母组成。
```

### 1.2 单词符号分类

**页码：书内页 6-7**

SNL 编译系统的单词符号分为：

```text
标识符        ID
保留字        保留字是标识符的子集，例如 if, repeat, read, write 等
无符号整数    INTC
单字符分界符  + - * / < = ( ) [ ] . ; EOF 空白字符
双字符分界符  :=
注释头符      {
注释结束符    }
字符起止符    '
数组下标界限符 ..
```

### 1.3 标识符

**页码：书内页 7、31、48**

标识符由字母开头，后跟字母或数字。

词法程序先按标识符识别，再查保留字表；如果在保留字表中，则返回对应保留字，否则返回 `ID`。

PDF 中 LEX 示例给出的规则等价于：

```lex
[a-zA-Z][a-zA-Z0-9]*    -> ID 或 reservedLookup(...)
```

### 1.4 整数常量

**页码：书内页 6、35、41、48**

整数常量为无符号整数，Token 名为 `INTC`。

DFA 中数字进入 `INNUM` 状态，继续读取数字；遇到非数字时结束数字识别。

PDF 中 LEX 示例给出：

```lex
[0-9]+    -> INTC
```

### 1.5 字符常量

**页码：书内页 7、35、41、48-49**

字符常量使用单引号作为起止符。

DFA 中遇到 `'` 进入 `INCHAR` 状态；若正确读入字符并遇到结束单引号，返回 `CHARC`。

PDF 中 LEX 示例给出等价规则：

```lex
'[a-zA-Z0-9]'    -> CHARC
```

**未找到：**PDF 在这些页没有给出完整的“允许字符常量包含哪些特殊字符”的扩展规则，只能确认示例规则支持字母和数字字符。

### 1.6 注释

**页码：书内页 6-7、35、40-41**

注释以 `{` 开始，以 `}` 结束。

DFA 中 `{` 进入 `INCOMMENT` 状态；注释内容不产生 Token；直到读到 `}` 才返回初始状态继续扫描。

错误规则：

```text
若在注释状态中读到文件结束符 EOF，则输出 ERROR。
```

### 1.7 运算符和界符

**页码：书内页 6-7、38-39、47-49**

PDF 中出现的 Token 类别包括：

```text
ASSIGN      :=
EQ          =
LT          <
PLUS        +
MINUS       -
TIMES       *
OVER        /
LPAREN      (
RPAREN      )
LMIDPAREN   [
RMIDPAREN   ]
DOT         .
COLON       :
SEMI        ;
COMMA       ,
UNDERANGE   ..
```

注意：

```text
:   单独出现为 COLON
:=  为 ASSIGN

.   单独出现为 DOT
..  为 UNDERANGE
```

### 1.8 词法 DFA 状态

**页码：书内页 41**

DFA 状态表列出以下状态：

```text
START       开始状态
INASSIGN    赋值状态
INCOMMENT   注释状态
INNUM       数字状态
INID        标识符状态
INCHAR      字符标志状态
INRANGE     数组下标界限状态
DONE        完成状态
```

状态含义：

```text
START 遇到字母        -> INID
START 遇到数字        -> INNUM
START 遇到 ':'        -> INASSIGN
START 遇到 '{'        -> INCOMMENT
START 遇到 '''        -> INCHAR
START 遇到 '.'        -> INRANGE
START 遇到单字符分界符 -> DONE
```

### 1.9 词法错误规则

**页码：书内页 41、48-49**

原文能支撑的错误规则：

```text
1. 在 INASSIGN 状态，如果 ':' 后不是 '='，返回 ERROR。
2. 在 INCOMMENT 状态，如果读到 EOF，返回 ERROR。
3. 在 INCHAR 状态，如果字符常量格式不匹配，返回 ERROR。
4. 在 START 状态，如果字符不匹配任何合法词法规则，返回 ERROR。
```

---

## 2. Token 内部表示

### 2.1 Token 结构

**页码：书内页 32**

PDF 给出的 Token 结构如下：

```text
Token
  Lineshow : int
  Lex      : LexType
  Sem      : Char*
```

含义：

```text
Lineshow : 记录该单词在源程序中的行数
Lex      : 记录该单词的词法信息，其中 LexType 为单词的类型枚举
Sem      : 记录该单词的语义信息
```

PDF 说明：

```text
Token 的结构可由两部分构成，即词法信息和语义信息。
词法信息记录单词种类；
语义信息记录该单词的具体信息。
若是标识符，则记录标识符名字 id；
若是常数，则记录常数值 val。
```

### 2.2 Token 类别名

**页码：书内页 47-49**

PDF 的 LEX 示例中定义的 Token 类别包括：

```text
ENDFILE, ERROR,

PROGRAM, PROCEDURE, TYPE, VAR,
IF, THEN, ELSE, FI,
WHILE, DO, ENDWH,
BEGIN1, END1,
READ, WRITE,
ARRAY, OF, RECORD, RETURN,
INTEGER, CHAR1,

ID, INTC, CHARC,

ASSIGN, EQ, LT,
PLUS, MINUS, TIMES, OVER,
LPAREN, RPAREN,
DOT, COLON, SEMI, COMMA,
LMIDPAREN, RMIDPAREN,
UNDERANGE
```

注意：

```text
PDF 示例为了避免和 C/C++ 关键字或已有名字冲突，
把 begin、end、char 写成了 BEGIN1、END1、CHAR1。
```

### 2.3 Token 类别码

**页码：书内页 47-49**

**未找到：**PDF 未给出明确的整数类别码表。

PDF 中给的是 `typedef enum` 形式的 Token 类别枚举，而不是显式数值编码。因此可确认“类别码”由枚举值承担，但 PDF 未给出每个 Token 的固定整数码。

### 2.4 Token 输出样例

**页码：书内页 39、50-51**

PDF 给出的词法分析输出样例包含三列：

```text
行数
词法信息
语义信息
```

示例中的输出形式包括：

```text
1  PROGRAM       保留字，无语义信息
1  ID            p
2  TYPE          保留字，无语义信息
2  ID            t
2  EQ            分界符，无语义信息
2  INTEGER       保留字，无语义信息
2  SEMI          分界符，无语义信息
3  VAR           保留字，无语义信息
3  ID            v1
3  COMMA         分界符，无语义信息
3  ID            v2
3  COLON         分界符，无语义信息
3  CHAR          保留字，无语义信息
...
EOF
```

LEX 示例输出中还出现：

```text
reserved word: program
ID, name = p
reserved word: type
ID, name = t
reserved word: integer
...
NUM, val = 10
ERROR: '
CHAR, char = a
```

---

## 3. SNL 语法树节点

### 3.1 语法树总体结构

**页码：书内页 58**

PDF 给出的语法树根节点为：

```text
根节点 ProK
```

根节点下包含：

```text
程序头 PheadK
类型声明 TypeK
变量声明 VarK
过程声明 ProcK
程序体 StmLK
```

过程声明节点下包含：

```text
形参
所有声明
过程体
```

程序体和过程体下为语句序列。

### 3.2 节点种类

**页码：书内页 58-59**

`NodeKind` 分为两类：标志节点和具体节点。

标志节点：

```text
ProK       根标志节点
PheadK     程序头标志节点
TypeK      类型声明标志节点
VarK       变量声明标志节点
ProcDecK   函数/过程声明标志节点
StmLK      语句序列标志节点
```

具体节点：

```text
DecK       声明节点
StmtK      语句节点
ExpK       表达式节点
```

### 3.3 DecK 声明节点子类

**页码：书内页 59**

声明节点根据类型部分的信息分为：

```text
ArrayK     组类型
CharK      字符类型
IntegerK   整数类型
RecordK    记录类型
IdK        以类型标识符作为类型
```

### 3.4 StmtK 语句节点子类

**页码：书内页 59**

语句节点由 `StmtKind` 定义，分为：

```text
IfK        判断语句
WhileK     循环语句
AssignK    赋值语句
ReadK      读语句
WriteK     写语句
CallK      函数/过程调用语句
ReturnK    返回语句
```

### 3.5 ExpK 表达式节点子类

**页码：书内页 59**

表达式节点由 `ExpKind` 定义，分为：

```text
OpK        操作符类型
ConstK     常整数类型
IdK        标识符类型
```

### 3.6 TreeNode 数据结构

**页码：书内页 59-60**

PDF 给出的语法树节点结构字段如下：

```text
child[3]
sibling
lineno
nodekind
kind
idnum
name
table
attr
type
```

字段含义：

```text
child[i]    指向子语法树节点指针
sibling     指向兄弟语法树节点指针
lineno      源程序行号，整数类型
nodekind    语法树节点类型，取 ProK、PheadK、TypeK、VarK、ProcDecK、StmLK、DecK、StmtK、ExpK
kind        共用体结构，记录具体类型
idnum       记录一个节点中标志符的个数
name        字符串数组，数组成员是节点中的标志符名字
table       指针数组，数组成员是节点中各标志符在符号表中的入口
type_name   记录类型名；当节点为声明类型且类型由类型标志符表示时有效
attr        记录语法树节点其他属性，为结构体类型
```

### 3.7 attr 属性字段

**页码：书内页 60**

数组属性：

```text
ArrayAttr.low        记录数组下界
ArrayAttr.up         记录数组上界
ArrayAttr.childType  记录数组成员类型
```

过程属性：

```text
procAttr.paramt
```

含义：记录过程参数类型，取枚举值：

```text
valparamtype
varparamtype
```

表达式属性：

```text
ExpAttr.op
ExpAttr.val
ExpAttr.varkind
ExpAttr.type
```

含义：

```text
ExpAttr.op:
  记录运算符单词。
  关系运算表达式对应节点取 LT、EQ。
  加法运算简单表达式对应节点取 PLUS、MINUS。
  乘法运算项对应节点取 TIMES、OVER。

ExpAttr.val:
  记录语法树节点的数值。
  当节点为“数字因子”时有效，为整数类型。

ExpAttr.varkind:
  记录变量类别，取值：
    IdV
    ArrayMembV
    FieldMembV

ExpAttr.type:
  记录语法树节点的检查类型，取值：
    Void
    Integer
    Boolean
```

**未找到：**PDF 此处没有列出 `Char` 作为 `ExpType` 的取值，尽管语言有 `char` 类型。

### 3.8 语法树输出格式

**页码：书内页 61**

PDF 给出的是表格样例：左侧为源程序，中间为词法分析后的 Token 序列，右侧为语法分析后的语法树。语法树以缩进层次形式显示，例如：

```text
ProK
  PheadK p
  TypeK
    DecK IntegerK t1
  VarK
    DecK IntegerK v1 v2
  ProcDecK q
    DecK value param:
      IntegerK i
    StmLK
      StmtK Assign
        ExpK ...
```

**未找到：**PDF 没有给出唯一强制的机器可解析输出格式，只给出层次树样例。

---

## 4. 符号表设计

### 4.1 语义分析需要构造的信息表

**页码：书内页 154**

SNL 语义分析主要完成：

```text
1. 构造符号表和信息表：
   (1) 构造标识符的符号表；
   (2) 构造类型信息表，包括数组信息表和记录信息表；
   (3) 构造过程信息表；
   (4) 构造形参信息表。
```

### 4.2 符号表内容

**页码：书内页 155-156**

PDF 说明符号表用于记录标识符属性。表项包括名字与信息，其中名字字段示例为：

```text
Name1
Name2
...
Namen
```

对应信息字段示例为：

```text
Name1 info
Name2 info
...
Namen info
```

### 4.3 类型内部表示

**页码：书内页 156-158**

PDF 给出类型表示结构：

```c
typedef struct typeIR {
    int size;
    kind;
    union {
        struct {
            AccessKind access;
            int level;
            int off;
        } VarAttr;

        struct {
            int level;
            ParamTable *param;
            int code;
            int size;
        } ProcAttr;
    } More;
} typeIR;
```

类型种类包括：

```text
intTy
charTy
arrayTy
recordTy
boolTy
```

数组类型结构：

```c
struct {
    int size;
    int indexTy;
    int elemTy;
} arrayIR;
```

含义：

```text
size     内存大小
indexTy  下标类型
elemTy   成员类型
```

记录类型结构：

```c
struct {
    int size;
    int kind;
    int body;
} recordIR;
```

含义：

```text
size  记录大小
kind  recordTy
body  域成员链表地址
```

### 4.4 符号表项结构

**页码：书内页 159**

PDF 给出符号表项结构：

```c
struct symbtable {
    char idname[10];
    AttributeIR attrIR;
    struct symbtable *next;
}
```

字段含义：

```text
idname   记录域中的标识符
attrIR   指向该项目的内部表示
offset   表示当前标识符在记录中的偏移
next     指向下一个域
```

### 4.5 符号表组织

**页码：书内页 159-160**

PDF 说明 SNL 程序由分程序嵌套结构组成，每个分程序对应一个符号表。符号表通过 `scope` 组织。

原文支撑的作用域规则：

```text
1. 每进入一层分程序，生成该层的符号表。
2. 在该层符号表中记录本层声明的标识符。
3. 查找标识符时，从当前作用域开始，逐层向外查找。
4. 分程序结束时，删除该层符号表。
```

图中显示：

```text
SymbTable
  -> 当前层符号表
  -> 外层符号表
  -> ...
```

### 4.6 符号表操作

**页码：书内页 161-162**

PDF 给出的符号表操作包括：

```text
CreateTable(level)
DestroyTable()
FindEntry(char *id, int flag, SymbTable **Entry)
SearchTable(char *id, currentLevel, SymbTable **Entry)
FindField(char *id, FieldChain *head, FieldChain **Entry)
PrintSymbTable()
```

含义：

```text
CreateTable:
  创建 level 层符号表。

DestroyTable:
  删除一个符号表，Level = Level - 1。

FindEntry:
  在当前作用域中查找 id。

SearchTable:
  从当前层开始，沿 scope 链查找 id。

FindField:
  在记录域链表中查找字段 id。

PrintSymbTable:
  输出符号表内容。
```

插入算法：PDF 中列出函数 `Enter`，说明为“登记标识符和属性”。

**未找到：**更完整的插入伪代码细节。

---

## 5. 语义检查规则

### 5.1 PDF 明确列出的语义错误类别

**页码：书内页 154**

PDF 列出 SNL 语义错误通常包括：

```text
1. 标识符的重复定义；
2. 无声明的标识符；
3. 标识符为非期望的标识符类别：
   类型标识符、变量标识符、过程名标识符；
4. 数组类型下标越界错误；
5. 数组成员变量和域变量的引用不合法；
6. 赋值语句的左右两边类型不相容；
7. 赋值语句左端不是变量标识符；
8. 过程调用中，形实参类型不匹配；
9. 过程调用中，形实参个数不相同；
10. 过程调用语句中，标识符不是过程标识符；
11. if 和 while 语句的条件部分不是 bool 类型；
12. 表达式中运算符的分量类型不相容。
```

### 5.2 类型声明分析

**页码：书内页 164-166**

PDF 给出的语义分析流程包括：

```text
TypeProcess(TreeNode *t, IdKind deckind)
```

功能：处理语法树中的类型声明部分，建立类型内部表示，并检查类型声明。

明确可提取规则：

```text
1. 类型名进入符号表。
2. 数组类型需要记录上下界与成员类型。
3. 记录类型需要建立域成员链表。
4. 类型标识符引用需要查符号表。
```

**未找到：**具体判定条件在流程图中给出，但 OCR 可读性不足；未能完整还原每个判断框文字。

### 5.3 变量声明分析

**页码：书内页 167-168**

PDF 给出：

```text
VarDecPart(TreeNode *t)
```

功能：分析变量声明部分，建立变量类型及其信息，检查变量定义。

可提取规则：

```text
1. 变量声明中每个变量名要进入符号表。
2. 若当前作用域已存在同名标识符，则属于重复定义错误。
3. 变量类型若引用类型标识符，需要查类型符号。
4. 变量类型内部表示用于后续类型检查。
```

### 5.4 过程声明分析

**页码：书内页 168-170**

PDF 给出：

```text
HeadProcess(TreeNode *t)
ParaDecList(TreeNode *t)
Body(TreeNode *t)
```

可提取规则：

```text
1. 过程名要登记到符号表。
2. 形参表要建立参数信息。
3. 过程体有自己的作用域。
4. 过程声明部分处理完成后进入过程体语义分析。
```

### 5.5 语句分析

**页码：书内页 170-176**

PDF 给出语句分析函数：

```text
statement(TreeNode *t)
Ifstatement(TreeNode *t)
Whilestatement(TreeNode *t)
Assignment(TreeNode *t)
readstatement(TreeNode *t)
writestatement(TreeNode *t)
callstatement(TreeNode *t)
returnstatement(TreeNode *t)
```

明确规则：

```text
1. if 语句条件必须是 bool 类型。
2. while 语句条件必须是 bool 类型。
3. 赋值语句左端必须是变量。
4. 赋值语句左右类型必须相容。
5. read 语句参数必须是变量。
6. write 语句参数为表达式。
7. call 语句中被调用标识符必须为过程标识符。
8. call 语句中实参与形参个数必须一致。
9. call 语句中实参与形参类型必须匹配。
```

---

## 6. 过程调用与参数传递

### 6.1 形参类别

**页码：书内页 60**

过程参数类型由：

```text
valparamtype
varparamtype
```

表示。

含义：

```text
valparamtype 表示值参
varparamtype 表示变参
```

### 6.2 过程调用语义检查

**页码：书内页 154、170-176**

PDF 支撑的规则：

```text
1. 被调用标识符必须是过程标识符。
2. 形参和实参个数必须相同。
3. 形参和实参类型必须匹配。
```

### 6.3 value 参数传递实现

**页码：书内页 60、184-186**

PDF 明确区分了值参与变参，但在语义章节中只给出参数类型枚举和检查规则。

在中间代码章节，过程调用相关中间代码包括：

```text
VARACT
VALACT
CALL
```

其中 `VALACT` 表示传值实参相关处理。

**未找到：**PDF 没有在可读文本中给出 value 参数的完整运行时拷贝算法。

### 6.4 var 参数传递实现

**页码：书内页 60、184-186**

PDF 中间代码包含：

```text
VARACT
```

用于变参实参处理。语法树属性中 `varparamtype` 标记过程参数是变参。

**未找到：**PDF 没有在可读文本中给出 var 参数“传地址”的完整伪代码描述；只可确认 PDF 通过 `varparamtype` 和 `VARACT` 区分变参。

### 6.5 嵌套过程

**页码：书内页 154-160、228-231**

PDF 明确说明 SNL 支持嵌套分程序结构，符号表按层次组织，每个分程序对应一个符号表；查找时沿作用域链向外查找。

目标代码章节中运行时环境使用过程活动记录和 display 表支持非局部变量访问。

### 6.6 递归调用

**页码：书内页 6、229-231**

PDF 在 SNL 特点中说明“过程允许嵌套定义，允许递归调用”。运行时管理章节说明过程活动记录用于过程执行时保存动态信息。

**未找到：**PDF 没有给出递归调用的独立算法，只能从“活动记录 + 过程调用”机制支撑递归实现。

---

## 7. 目标代码生成

注意：PDF 第 9 章使用的是 **虚拟目标机 TM**，不是 MARS/MIPS。MIPS 指令、MARS syscall、32 位 MIPS 栈帧等内容在 PDF 中 **未找到**。

### 7.1 TM 机器寄存器

**页码：书内页 226**

PDF 给出 TM 寄存器：

```text
reg[0]  常数 0
reg[1]  ac 累加器
reg[2]  ac1 第二累加器
reg[3]  ax
reg[4]  sp 指向 display 表的栈顶 Noff
reg[5]  top 指向 AR 区栈顶
reg[6]  mp 指向 AR 区栈底
reg[7]  pc 指令地址寄存器
```

### 7.2 TM 指令格式

**页码：书内页 226**

指令格式：

```text
instruction = opcode, r, s, t
```

其中：

```text
opcode  操作码
r       参数1
s       参数2
t       参数3
```

### 7.3 TM 指令集

**页码：书内页 227**

PDF 列出四类指令：

```text
寄存器-寄存器指令:
  HALT, IN, OUT, ADD, SUB, MUL, DIV

寄存器-内存指令:
  LD, ST

寄存器-立即数指令:
  LDA, LDC, JLT, JLE, JGT, JGE, JEQ, JNE

无条件跳转:
  JMP
```

### 7.4 运行时空间管理

**页码：书内页 228-231**

PDF 将运行时内存划分为：

```text
代码区
静态空间
栈空间
```

其中栈空间用于：

```text
1. 过程活动记录
2. display 表
```

### 7.5 过程活动记录

**页码：书内页 229-230**

PDF 说明过程活动记录用于保存一次过程调用所需信息。活动记录包含：

```text
display 表相关信息
返回地址
动态链 / 控制链相关信息
局部变量空间
临时变量空间
参数相关空间
```

**说明：**OCR 对活动记录图中字段无法完整精确还原，但能确认该章使用活动记录管理过程调用。

### 7.6 display 表

**页码：书内页 229-231**

PDF 使用 display 表处理分程序嵌套结构中的非局部变量访问。display 表保存不同层次活动记录的入口地址，用于查找外层变量地址。

可提取规则：

```text
1. display 表保存各层活动记录地址。
2. 非局部变量访问通过 display 表定位对应层。
3. 调用过程时需要维护 display 表。
4. 过程返回时需要恢复 display 表。
```

### 7.7 变量地址

**页码：书内页 232-234、246-251**

目标代码生成章节中通过符号表项中的层次和偏移计算变量地址。

可提取规则：

```text
1. 标识符有层号 level 和偏移 offset。
2. 局部变量用当前活动记录基址加偏移访问。
3. 非局部变量通过 display 表找到对应层活动记录，再加偏移访问。
```

### 7.8 数组地址

**页码：书内页 234-235、239-242**

PDF 目标代码章节给出数组变量处理。可提取规则：

```text
1. 数组变量需要计算成员地址。
2. 下标表达式用于计算相对偏移。
3. 数组下标需要与数组下界进行偏移计算。
4. 访问数组成员时生成取成员地址或取成员值的目标代码。
```

**未找到：**PDF 可读文本中没有完整清晰的数组地址公式；但流程图和表格显示数组访问目标代码生成包含下标、偏移、成员地址计算。

### 7.9 记录地址

**页码：书内页 236、239-242**

PDF 目标代码章节给出域变量处理。可提取规则：

```text
1. 记录域通过域名查找域表。
2. 域变量地址由记录变量基址加域偏移得到。
3. 记录域访问生成对应的地址计算代码。
```

**未找到：**OCR 可读文本中未能完整还原记录字段地址计算公式。

### 7.10 过程调用目标代码

**页码：书内页 241、249-251**

目标代码章节中过程调用相关代码生成包括：

```text
CALL
PENTRY
ENDPROC
```

可提取规则：

```text
1. CALL 用于过程调用。
2. PENTRY 用于过程入口。
3. ENDPROC 用于过程出口。
4. 调用时需要处理活动记录、返回地址、display 表。
5. 返回时恢复调用者运行环境。
```

### 7.11 read / write

**页码：书内页 234、248**

TM 目标代码表中出现：

```text
READC / READ
WRITEC / WRITE
```

语义和中间代码章节中也有：

```text
READ
WRITE
```

可提取规则：

```text
read 语句生成输入相关目标代码。
write 语句生成输出相关目标代码。
```

**未找到：**PDF 中没有 MIPS syscall 形式的 `read/write` 实现，因为目标机是 TM，不是 MARS/MIPS。

---

## 8. PDF 未讲清楚或未找到的项目

```text
1. Token 的固定整数类别码：未找到。
2. 机器可解析的 AST 输出格式：未找到；只找到层次树样例。
3. value 参数的完整运行时拷贝算法：未找到。
4. var 参数传地址的完整伪代码：未找到；只找到 varparamtype 和 VARACT。
5. MARS 32 位 MIPS 目标代码规则：未找到；PDF 使用 TM 虚拟机。
6. MIPS syscall read/write：未找到。
7. MIPS 栈帧布局：未找到。
8. 记录字段地址计算的完整公式：未完整找到；只找到域表/偏移/目标代码生成流程。
9. 数组地址计算的完整公式：未完整找到；只找到下标、偏移和目标代码生成流程。
```
