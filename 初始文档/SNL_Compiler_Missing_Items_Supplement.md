# SNL 编译器实现缺失项补充

> 来源：用户上传的扫描版 PDF《编译程序的设计与实现》。  
> 范围：只补充前次提取中缺失或需要进一步确认的 SNL 相关内容。  
> 原则：不重新总结已有内容；看不清处标注“看不清”，未出现处标注“未找到”。  
> 页码：按书内页码标注。

---

## 1. ReturnStm / returnstatement

### 1.1 语法产生式

**页码：书内页 10**

在“语句”产生式中：

```text
61) Stm ::= ConditionalStm
62)       | LoopStm
63)       | InputStm
64)       | OutputStm
65)       | ReturnStm
66)       | ID AssCall
```

在“返回语句”处：

```text
返回语句：
75) ReturnStm ::= RETURN
```

### 1.2 return 是否带括号和表达式

**页码：书内页 10**

从产生式可以精确确定：

```text
ReturnStm ::= RETURN
```

因此 PDF 文法中的 `return`：

```text
不带括号
不带表达式
```

没有出现：

```text
return()
return(expr)
return Exp
```

### 1.3 示例代码

**页码：书内页 12、39、50-51、61**

在可见的 SNL 示例程序中，没有出现 `return` 语句。

**结论：未找到 return 示例代码。**

### 1.4 returnstatement 语义规则

**页码：书内页 171、174-176**

书内页 171 的 `statement` 语句分派流程图中有：

```text
返回语句  -> 调用返回语句处理函数 returnstatement(t)
```

但在书内页 164-176 的后续具体语义分析函数中，只看到：

```text
ifstatement
whilestatement
readstatement
writestatement
callstatement
assignstatement
```

**未找到 returnstatement 的独立流程图或具体判定条件。**

可确认的信息只有：

```text
statement 根据语句类型识别为返回语句时，调用 returnstatement(t)。
```

---

## 2. 词法分析输出样例原样提取

### 2.1 Token 结构表

**页码：书内页 38，表 4.2**

表头：

```text
单词类型 | Token
         | LEX | SEM
```

表内容可读部分如下：

```text
标识符        ID（a String）                  单词名
保留字        如 IF、BEGIN 等                  单词名
无符号整数    INTC（23）                       字符串
单字符分界符  如 EQ（=）、SEMI（;）等           空
双字符分界符  ASSIGN（:=）                     空
注释头符      注释部分不用转换成 TOKEN 形式（{}） 空
字符标识符    该标识符后的字符 CHARC（'b'）     字符（b）
数组下标界限符 UNDERANGE（..）                 空
错误          ERROR                            空
```

表题：

```text
表 4.2 SNL 词法分析得到的 TOKEN 结构
```

### 2.2 表格式词法分析输出样例

**页码：书内页 39，表 4.3**

表头原样：

```text
源程序 | 词法分析后的 TOKEN 序列
       | 行数 | 词法信息 | 语义信息
```

源程序：

```text
program    p
    type   t = integer ;
    var    t   v1;
           char v2;
    begin
        read(v1);
        v1:=v1+10;
        write(v1)
    end.
```

词法分析后的 TOKEN 序列：

```text
行数 | 词法信息   | 语义信息
1    | PROGRAM    | 保留字，无语义信息
1    | ID         | p
2    | TYPE       | 保留字，无语义信息
2    | ID         | t
2    | ASSIGN     | 分界符，无语义信息
2    | INTEGER    | 保留字，无语义信息
2    | SEMI       | 分界符，无语义信息
3    | VAR        | 保留字，无语义信息
3    | ID         | t
3    | ID         | v1
3    | SEMI       | 分界符，无语义信息
4    | CHAR       | 保留字，无语义信息
4    | ID         | v2
4    | SEMI       | 分界符，无语义信息
5    | BEGIN      | 保留字，无语义信息
6    | READ       | 保留字，无语义信息
6    | LMIDPAREN  | 分界符，无语义信息
6    | ID         | v1
6    | RMIDPAREN  | 分界符，无语义信息
6    | SEMI       | 分界符，无语义信息
7    | ID         | v1
7    | ASSIGN     | 分界符，无语义信息
7    | ID         | v1
7    | PLUS       | 分界符，无语义信息
7    | INTC       | 10
7    | SEMI       | 分界符，无语义信息
8    | WRITE      | 保留字，无语义信息
8    | LMIDPAREN  | 分界符，无语义信息
8    | ID         | v1
8    | RMIDPAREN  | 分界符，无语义信息
9    | END        | 保留字，无语义信息
9    | DOT        | 程序结束符，无语义信息
11   | EOF        | 文件结束符，无语义信息
```

表题：

```text
表 4.3 SNL 语言程序的词法分析结果例
```

### 2.3 FLEX 运行输出样例

**页码：书内页 50-51，表 4.6**

源程序：

```text
program p
type t = integer;
var  t  v1;
      char  v2;
begin
    read(v1);
    v1:=v1*10;
    v1 = "d";
    v2:='a';
    write(v1)
end.
```

运行 `yylex.c` 后生成的词法分析结果：

```text
reserved word: program
ERROR:
ID, name= p
reserved word: type
ID, name= t
=
reserved word: integer
;
reserved word: var
ID, name= t
ID, name= v1
;
reserved word: char
ID, name= v2
;
reserved word: begin
reserved word: read
(
ID, name= v1
)
;
ID, name= v1
:=
ID, name= v1
*
NUM, val= 10
;
ID, name= v1
=
ERROR: "
ID, name= d
ERROR: "
;
ID, name= v2
:=
CHAR, char=a
;
reserved word: write
(
ID, name= v1
)
reserved word: end
.
EOF
```

表题：

```text
表 4.6 运行 yylex.c 后生成的词法分析结果
```

---

## 3. 语法树输出样例原样提取

**页码：书内页 61**

表头：

```text
源程序 | 词法分析后的 TOKEN 序列 | 语法分析后的语法树
```

右侧“语法分析后的语法树”可读内容如下。缩进按原图尽量保留：

```text
ProK
    PheadK  p
    TypeK
        DecK  IntegerK  t1
    VarK
        DecK  IntegerK  v1  v2
    ProcDecK  q
        DecK  value      param:
IntegerK  i
        VarK
            DecK  IntegerK  a
        StmLK
            StmtK  Assign
                ExpK  a   IdV
                ExpK  i   IdV
            StmtK  Write
                ExpK  a   IdV
    StmLK
        StmtK  Read  v1
        StmtK  If
            ExpK  Op  <
                ExpK  v1  IdV
                ExpK  Const  10
            StmtK  Assign
                ExpK  v1  IdV
                ExpK  Op  +
                    ExpK  v1  IdV
                    ExpK  Const  10
            StmtK  Assign
                ExpK  v1  IdV
                ExpK  Op  -
                    ExpK  v1  IdV
                    ExpK  Const  10
        StmtK  Call
            ExpK  q   IdV
            ExpK  v1  IdV
```

说明：

```text
1. 声明节点显示形式示例：
   DecK IntegerK t1
   DecK IntegerK v1 v2
   DecK value param: IntegerK i

2. 语句节点显示形式示例：
   StmtK Assign
   StmtK Write
   StmtK Read v1
   StmtK If
   StmtK Call

3. 表达式节点显示形式示例：
   ExpK a IdV
   ExpK i IdV
   ExpK Op <
   ExpK Const 10
   ExpK Op +
   ExpK Op -
```

**看不清：**`DecK value param:` 与 `IntegerK i` 在图中横向排版靠近，OCR/肉眼可辨其内容，但精确空格列宽无法保证。

---

## 4. P164-P176 语义分析流程图文字补充

以下只列用户点名的函数，以及相关能读清的判断框/处理框。看不清的地方明确标注。

---

### 4.1 TypeProcess

**页码：书内页 164-165，图 6.5**

函数声明：

```text
TypeIR * TypeProcess(TreeNode *t, DecKind deckind)
```

算法说明：

```text
类型分析处理。处理语法树的当前节点类型，构造当前类型的内部表示，
并将其地址返回给 Ptr 类型内部表示的地址。
```

流程图中可辨识文字：

```text
入口
deckind 是 id?
deckind 是 array?
deckind 是 record?
deckind 是整数?
deckind 是字符?
函数返回 Ptr
```

对应处理框可辨识部分：

```text
调用自定义类型内部结构分析函数 nameType
调用数组类型内部表示处理函数 arrayType
调用记录类型的内部表示处理函数 recordType
直接返回整数指针
直接返回字符指针
```

**看不清：**图 6.5 中部分判断框和箭头旁 Yes/No 对应关系较模糊，但上述节点文字可辨。

---

### 4.2 VarDecPart / VarDecList

PDF 在书内页 167-168 的标题实际写为：

```text
变量声明部分分析处理函数
函数声明：void VarDecList(TreeNode *t)
```

**页码：书内页 167，文字说明**

```text
算法说明：当遇到变量标识符 id 时，把 id 登记到符号表中，检查重复性定义；
遇到类型时，构造其内部表示。
```

**页码：书内页 168，图 6.10**

流程图可辨识文字：

```text
入口
t != NULL，循环处理变量
Kind 为变量类型
判断 t->idnum，循环处理同一节点的 id，调用类型处理函数
该变量是直接变量
记录层数、变量类型等信息，偏移加变量类型的 size
记录层数、变量类型等信息，偏移加 1
登记该变量的属性及名字，并判断是否被重复定义
取下一变量声明
结束
```

其中判断分支：

```text
该变量是直接变量
Yes -> 记录层数、变量类型等信息，偏移加变量类型的 size
No  -> 记录层数、变量类型等信息，偏移加 1
```

**看不清：**图中个别外层循环线和 No 分支返回位置较模糊，但判断条件和处理框文字基本可辨。

---

### 4.3 HeadProcess

**页码：书内页 168-169，图 6.12**

函数声明：

```text
SymbTable * HeadProcess(TreeNode *t)
```

算法说明：

```text
在当前层符号表中填写函数标识符的属性，在新层符号表中填写形参标识符的属性。
其中过程的大小和代码需以后回填。
```

流程图可辨识文字：

```text
入口
填写过程标识符的属性，登记过程的符号表项，调用形参处理函数
函数返回 entry
```

**看不清：**无明显复杂判断框；流程图内容基本可辨。

---

### 4.4 ParaDecList

**页码：书内页 169-170，图 6.13**

函数声明：

```text
ParamTable * ParaDecList(TreeNode *t)
```

算法说明：

```text
在新的符号表中登记所有形参的表项，构造形参表项的地址表，
并使 param 指向此地址表。
```

流程图可辨识文字：

```text
入口
当前语法树节点的第一个儿子不为 NULL
p 指向 t 的第一个子节点
进入新的局部化区，调用函数 VarDecPart(p)，处理变量声明部分
构造形参符号表；并使其连接至符号表的 param 项
函数返回形参符号表链的头 head
```

**看不清：**图中 `VarDecPart(p)` 的函数名可辨；但是否是 `VarDecPart` 还是书中其他同义函数名，图像上略模糊。结合文字说明应为处理变量声明部分。

---

### 4.5 statement

**页码：书内页 170-171，图 6.15**

函数声明：

```text
void statement(TreeNode *t)
```

算法说明：

```text
根据语法树节点中的 kind 项判断应该转向处理哪个语句类型函数。
```

流程图可辨识文字：

```text
入口

if语句
  Yes -> 调用条件语句处理函数 ifstatement(t)

while语句
  Yes -> 调用循环语句处理函数 whilestatement(t)

赋值语句
  Yes -> 调用赋值语句处理函数 assignstatement(t)

读语句
  Yes -> 调用输入语句处理函数 readstatement(t)

写语句
  Yes -> 调用输出语句处理函数 writestatement(t)

调用语句
  Yes -> 调用调用语句处理函数 callstatement(t)

返回语句
  Yes -> 调用返回语句处理函数 returnstatement(t)

出口
```

---

### 4.6 Assignment / assignstatement

**页码：书内页 174，图 6.21**

函数声明：

```text
void assignstatement(TreeNode *t)
```

算法说明：

```text
赋值语句的语义分析的重点是检查赋值号两端分量的类型相容性。
```

流程图可辨识文字：

```text
入口
Child1, child2 分别为 t 的第一、二个儿子
child1 的第一个子节点等于空
```

左侧分支可辨识：

```text
Varkind == ArrayMembV
Yes -> Eptr = arrayVar(child1)
No  -> Eptr = recordVar(child1)
```

右侧分支可辨识：

```text
在符号表线查找标识符
找到
  No -> 报错
Id 是变量
  No -> 报错
Eptr 赋值为该变量的类型指针
检查赋值号两端是否等价
出口
```

**看不清：**图中“在符号表线查找标识符”的“线”可能为 OCR/图像误读，原意可能是“在符号表链查找标识符”，但此处不推测，按可见文字记录。

---

### 4.7 callstatement

**页码：书内页 174，图 6.22**

函数声明：

```text
void callstatement(TreeNode *t)
```

算法说明：

```text
函数调用语句的语义分析首先检查符号表求出其属性中的 Param 部分
（形参符号表项地址表），并用它检查形参和实参之间的对应关系是否正确。
```

流程图可辨识文字：

```text
入口
在符号表中查找此标识符
找到
  No -> 报错
Id 不是过程名
  Yes -> 报错
处理形实参结合
结束
```

可确认判断条件：

```text
1. 查找调用标识符是否存在。
2. 若未找到，报错。
3. 若 Id 不是过程名，报错。
4. 然后处理形参和实参结合。
```

---

### 4.8 returnstatement

**页码：书内页 171、174-176**

书内页 171 的 `statement` 流程图中有：

```text
返回语句 -> 调用返回语句处理函数 returnstatement(t)
```

但书内页 164-176 范围内未出现 `returnstatement` 的独立函数说明和算法框图。

**结论：未找到 returnstatement 的判断条件。**

---

## 5. 其他相关流程图中可读的判定条件

这些不是逐项点名的主函数，但与 P164-P176 语义规则直接相关。

### 5.1 nameType

**页码：书内页 165，图 6.6**

函数声明：

```text
TypeIR * nameType(TreeNode *t)
```

算法说明：

```text
符号表中寻找已定义类型名，调用寻找表项地址函数 FindEntry，
返回找到的表项地址指针 entry。
如果 present 为 0，则发生无声明错误。
如果符号表中的该标识符属性信息不是类型，则非类型标识符错。
```

流程图可辨识判断：

```text
在符号表中查找该类型名，返回 present
present = 0 -> 报错
不是类型标识符 -> 报错
Ptr = entry->attrIR.idtype
函数返回 Ptr
```

### 5.2 arrayVar

**页码：书内页 172-173，图 6.19**

函数声明：

```text
TypeIR * arrayVar(TreeNode *t)
```

算法说明：

```text
检查 var := var0[E] 中 var0 是不是数组类型变量，
E 是不是和数组的下标变量类型匹配。
```

流程图可辨识判断：

```text
在符号表中查找此标识符
没找到 -> 报错
Var0 不是变量 -> 报错
Var0 不是数组变量 -> 报错
E 与下标类型不相符 -> 报错
函数返回指向数组元素类型内部表示的指针
```

### 5.3 recordVar

**页码：书内页 173，图 6.20**

函数声明：

```text
TypeIR * recordVar(TreeNode *t)
```

算法说明：

```text
检查 var := var0.id 中的 var0 是不是记录类型变量，
id 是不是该记录类型中的域成员。
```

流程图可辨识判断：

```text
在符号表中查找此标识符
没有找到 -> 报错
Var0 不是变量 -> 报错
Var0 不是记录变量 -> 报错
检查 id 不是合法域名 -> 报错
返回指向记录元素类型内部表示的指针
```

### 5.4 ifstatement

**页码：书内页 175，图 6.23**

函数声明：

```text
void ifstatement(TreeNode *t)
```

算法说明：

```text
检查条件表达式是否为 bool 类型，处理 then 语句序列部分和 else 语句序列部分。
```

流程图可辨识判断：

```text
条件表达式是 bool 型
No -> 报错
Yes -> 循环处理 then 语句部分
       循环处理 else 语句部分
```

### 5.5 whilestatement

**页码：书内页 175，图 6.24**

函数声明：

```text
void whilestatement(TreeNode *t)
```

算法说明：

```text
检查条件表达式是否为 bool 类型，处理语句序列部分。
```

流程图可辨识判断：

```text
条件表达式是 bool 型
No -> 报错
Yes -> 循环处理语句序列部分
```

### 5.6 readstatement

**页码：书内页 175-176，图 6.25**

函数声明：

```text
void readstatement(TreeNode *t)
```

算法说明：

```text
检查要读入的变量有无声明和是否为变量。
```

流程图可辨识判断：

```text
在符号表中查找该标识符
找到
  No -> 报错
Id 是变量标识符
  No -> 报错
结束
```

### 5.7 writestatement

**页码：书内页 176，图 6.26**

函数声明：

```text
void writestatement(TreeNode *t)
```

算法说明：

```text
分析写语句中的表达式是否合法。
```

流程图可辨识文字：

```text
入口
处理表达式部分
条件表达式不是 bool
  No -> 报错
结束
```

**看不清 / 疑点：**图 6.26 的判断框写作“条件表达式不是 bool”，这与“write 表达式是否合法”的文字说明不完全一致；图中 Yes/No 分支也较模糊。这里只按图中可见文字记录，不推测其真实意图。
