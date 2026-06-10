# SNL PPT 语法产生式摘要

本文件记录第二轮递归下降语法分析采用的 PPT 文法口径。PPT 是本项目语法实现的主要依据；与 PDF 提取资料冲突时，以 PPT 为准。

```text
Program        ::= ProgramHead DeclarePart ProgramBody .
ProgramHead    ::= PROGRAM ProgramName
ProgramName    ::= ID
DeclarePart    ::= TypeDec VarDec ProcDec

TypeDec        ::= ε | TypeDeclaration
TypeDeclaration::= TYPE TypeDecList
TypeDecList    ::= TypeId = TypeName ; TypeDecMore
TypeDecMore    ::= ε | TypeDecList
TypeId         ::= ID
TypeName       ::= BaseType | StructureType | ID
BaseType       ::= INTEGER | CHAR
StructureType  ::= ArrayType | RecType
ArrayType      ::= ARRAY [ Low .. Top ] OF BaseType
Low            ::= INTC
Top            ::= INTC
RecType        ::= RECORD FieldDecList END
FieldDecList   ::= BaseType IdList ; FieldDecMore
                 | ArrayType IdList ; FieldDecMore
FieldDecMore   ::= ε | FieldDecList
IdList         ::= ID IdMore
IdMore         ::= ε | , IdList

VarDec         ::= ε | VarDeclaration
VarDeclaration ::= VAR VarDecList
VarDecList     ::= TypeName VarIdList ; VarDecMore
VarDecMore     ::= ε | VarDecList
VarIdList      ::= ID VarIdMore
VarIdMore      ::= ε | , VarIdList

ProcDec        ::= ε | ProcDeclaration
ProcDeclaration::= PROCEDURE ProcName ( ParamList ) ;
                   ProcDecPart ProcBody ProcDecMore
ProcDecMore    ::= ε | ProcDeclaration
ProcName       ::= ID
ParamList      ::= ε | ParamDecList
ParamDecList   ::= Param ParamMore
ParamMore      ::= ε | ; ParamDecList
Param          ::= TypeName FormList | VAR TypeName FormList
FormList       ::= ID FidMore
FidMore        ::= ε | , FormList
ProcDecPart    ::= DeclarePart
ProcBody       ::= ProgramBody

ProgramBody    ::= BEGIN StmList END
StmList        ::= Stm StmMore
StmMore        ::= ε | ; StmList
Stm            ::= ConditionalStm | LoopStm | InputStm | OutputStm
                 | ReturnStm | ID AssCall
AssCall        ::= AssignmentRest | CallStmRest
AssignmentRest ::= VariMore := Exp
ConditionalStm ::= IF RelExp THEN StmList ELSE StmList FI
LoopStm        ::= WHILE RelExp DO StmList ENDWH
InputStm       ::= READ ( Invar )
Invar          ::= ID
OutputStm      ::= WRITE ( Exp )
ReturnStm      ::= RETURN ( Exp )
CallStmRest    ::= ( ActParamList )
ActParamList   ::= ε | Exp ActParamMore
ActParamMore   ::= ε | , ActParamList

RelExp         ::= Exp OtherRelE
OtherRelE      ::= CmpOp Exp
Exp            ::= Term OtherTerm
OtherTerm      ::= ε | AddOp Exp
Term           ::= Factor OtherFactor
OtherFactor    ::= ε | MultOp Term
Factor         ::= ( Exp ) | INTC | Variable
Variable       ::= ID VariMore
VariMore       ::= ε | [ Exp ] | . FieldVar
FieldVar       ::= ID FieldVarMore
FieldVarMore   ::= ε | [ Exp ]
CmpOp          ::= < | =
AddOp          ::= + | -
MultOp         ::= * | /
```

实现补充：

- Parser 额外接受 `CHARC` 作为表达式因子，以支持语言中的字符常量和第一阶段词法样例。
- 本轮只做语法分析，不执行声明存在性、类型兼容性或参数匹配检查。
