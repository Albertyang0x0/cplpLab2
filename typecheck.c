#include "typecheck.h"
#include "cst.h"
#include "mycc.h"
#include "parser_state.h"
#include "symtbl.h"
#include "type.h"
#include "typetbl.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>

#define ETYPE_VARIABLE_UNDEF          ("1")
#define ETYPE_STRUCT_UNDEF            ("17")
#define ETYPE_VARIABLE_REDEF          ("3")
#define ETYPE_STRUCT_REDEF            ("16")
#define ETYPE_STRUCT_UNKNOWNFIELD     ("14")
#define ETYPE_ASSIGN_TYPEMISMATCH     ("5")
#define ETYPE_OPERAND_TYPEMISMATCH    ("7")
#define ETYPE_ASSIGN_RVALUE           ("6")
#define ETYPE_INDEX_NOTINTEGER        ("12")
#define ETYPE_INDEX_NOTARRAY          ("10")
#define ETYPE_CALL_NOTCALLABLE        ("11")

#define haserror(pstate, t1, t2)     ((t1).basetype == (pstate)->commontype.errortype || (t2).basetype == (pstate)->commontype.errortype)
#define iserror(pstate, t1)          ((t1).basetype == (pstate)->commontype.errortype)
#define isinteger(pstate, t1)        ((t1).basetype == (pstate)->commontype.inttype && (t1).dim == 0)
#define isfloat(pstate, t1)          ((t1).basetype == (pstate)->commontype.floattype && (t1).dim == 0)
#define isfunction(pstate, t1)       (typetbl_get(&(pstate)->typetbl, (t1).basetype)->kind == TYPE_FUNC && (t1).dim == 0)
#define isstruct(pstate, t1)         (typetbl_get(&(pstate)->typetbl, (t1).basetype)->kind == TYPE_STRUCT && (t1).dim == 0)
#define isarray(pstate, t1)          ((t1).dim != 0)
//创建错误类型，用来存储表达式出错
#define makeerror(pstate)                     (maketype((pstate)->commontype.errortype, 0))


static BaseTypeId type_specifier(ParserState* pstate, Cst* cst);
static void type_stmtcomp(ParserState* pstate, CstStmtComp* comp);
/* evaluate type of expression */
static Type type_expr(ParserState* pstate, Cst* cst);
/* evaluate type of statement */
static void type_stmt(ParserState* pstate, Cst* stmt);
/* handle extern definiton list.
 * including function definiton. extern variable declations. */
static void type_extdeflist(ParserState* pstate, CstList* extdeflist);



void checktype(ParserState* pstate, Cst* program) {
  assert(!program || cst_checkkind(program, CST_LIST));
  type_extdeflist(pstate, (CstList*)program);
}

/* 求列表长度 */
static size_t list_length(ParserState* pstate, CstList* list) {
  (void)pstate;
  size_t len = 0;
  while (list) {
    ++len;
    list = cstlist_next(list);
  }
  return len;
}
//在符号表中插入一个新函数名字
static Symbol* new_function(ParserState* pstate, StringId name, unsigned lineno) {
  Symbol* symbol = symtbl_search(pstate->symtbl, name);
  if (symbol) { /* symbol redefinition */
    mycc_error(ETYPE_VARIABLE_REDEF, lineno, "function redefinition: %s", strtbl_getstring(pstate->strtbl, name));
    return symbol;
  }
  symbol = symtbl_insert(pstate->symtbl, name);
  symbol->attr.kind = SYMBOL_FUNCTION;
  utils_oom_ifnull(symbol);
  return symbol;
}
//在符号表中插入一个新的的结构体名字
static Symbol* new_struct(ParserState* pstate, StringId name, unsigned lineno) {
  Symbol* symbol = symtbl_search(pstate->symtbl, name);
  if (symbol) { /* symbol redefinition */
    mycc_error(ETYPE_STRUCT_REDEF, lineno, "struct redefinition: %s", strtbl_getstring(pstate->strtbl, name));
    return symbol;
  }
  symbol = symtbl_insert(pstate->symtbl, name);
  symbol->attr.kind = SYMBOL_STRUCT;
  utils_oom_ifnull(symbol);
  return symbol;
}
//在符号表中插入一个新的变量名字
static Symbol* new_variable(ParserState* pstate, StringId name, unsigned lineno) {
  Symbol* symbol = symtbl_search(pstate->symtbl, name);
  if (symbol) { /* symbol redefinition */
    mycc_error(ETYPE_VARIABLE_REDEF, lineno, "variable redefinition: %s", strtbl_getstring(pstate->strtbl, name));
    return symbol;
  }
  symbol = symtbl_insert(pstate->symtbl, name);
  symbol->attr.kind = SYMBOL_VARIABLE;
  utils_oom_ifnull(symbol);
  return symbol;
}
//在符号表中插查找名为name的函数或变量，没找到返回NULL
static Symbol* get_variable_or_function(ParserState* pstate, StringId name, unsigned lineno) {
  Symbol* symbol = symtbl_search(pstate->symtbl, name);
  if (!symbol) {
    mycc_error(ETYPE_VARIABLE_UNDEF, lineno, "undefined variable: %s", strtbl_getstring(pstate->strtbl, name));
    return NULL;
  }
  if (symbol->attr.kind != SYMBOL_VARIABLE && symbol->attr.kind != SYMBOL_FUNCTION)
    mycc_error(ETYPE_VARIABLE_UNDEF, lineno, "symbol '%s' is not a variable or function", strtbl_getstring(pstate->strtbl, name));
  return symbol;
}
//对于一个在结构体成员定义列表计算这个列表中有几个成员
static size_t struct_nfield(ParserState* pstate, CstList* deflist) {
  size_t nfield = 0;
  while (deflist) {
    /* deflist->cst is def */
    CstDef* def = cstlist_get(deflist, CstDef*);
    nfield += list_length(pstate, (CstList*)def->declist);
    deflist = cstlist_next(deflist);
  }
  return nfield;
}
//处理在结构体定义中的vardec结构（变量声明）
//具体而言，计算成员的类型并将成员名字和其类型记录在结构体的域描述符中
static Type type_vardec_instruct(ParserState* pstate, Cst* cst, BaseTypeId base, size_t dim, StructFieldDesc* fielddesc) {
  if (cst_checkkind(cst, CST_ID)) {
    CstId* id = (CstId*)cst;
    fielddesc->type = maketype(base, dim);
    fielddesc->name = id->val;
    return fielddesc->type;
  }
  assert(cst_checkkind(cst, CST_DECARR));
  CstDecArr* decarr = (CstDecArr*)cst;
  return type_vardec_instruct(pstate, decarr->vardec, base, dim + 1, fielddesc);
}

//处理非结构体定义中的vardec结构
//具体而言计算变量的类型并将变量和其类型插入符号表
static Type type_vardec(ParserState* pstate, Cst* cst, BaseTypeId base, size_t dim) {
  if (cst_checkkind(cst, CST_ID)) {
    CstId* id = (CstId*)cst;
    Type type = maketype(base, dim);
    Symbol* symbol = new_variable(pstate, id->val, id->lineno);
    symbol->attr.type = type;
    return type;
  }
  assert(cst_checkkind(cst, CST_DECARR));
  CstDecArr* decarr = (CstDecArr*)cst;
  return type_vardec(pstate, decarr->vardec, base, dim + 1);
}

//处理结构体中的声明列表

static size_t type_declist_instruct(ParserState* pstate, BaseTypeId base, CstList* declist, StructFieldDesc* fielddesc) {
  StructFieldDesc* fieldbegin = fielddesc;
  while (declist) {
    CstDec* dec = cstlist_get(declist, CstDec*);
    assert(cst_checkkind(dec, CST_DEC));
    type_vardec_instruct(pstate, dec->vardec, base, 0, fielddesc++);
    declist = cstlist_next(declist);
  }
  return fielddesc - fieldbegin;
}

//处理在函数体中的（即复合语句中的）变量声明列表。如果有初始值计算初始值类型，并判断初始值类型与变量是否匹配
static void type_declist(ParserState* pstate, BaseTypeId base, CstList* declist) {
  while (declist) {
    CstDec* dec = cstlist_get(declist, CstDec*);
    assert(cst_checkkind(dec, CST_DEC));
    Type lval = type_vardec(pstate, dec->vardec, base, 0);
    if (dec->initval) {
      Type rval = type_expr(pstate, dec->initval);
      if (!type_is_same(lval, rval) && !haserror(pstate, lval, rval))
        mycc_error(ETYPE_ASSIGN_TYPEMISMATCH, dec->vardec->lineno, "type mismatching");
    }
    declist = cstlist_next(declist);
  }
}

//处理全局变量声明列表
static void type_extdeclist(ParserState* pstate, BaseTypeId base, CstList* declist) {
  while (declist) {
    assert(cst_checkkind(declist->cst, CST_ID) || cst_checkkind(declist->cst, CST_DECARR));
    type_vardec(pstate, declist->cst, base, 0);
    declist = cstlist_next(declist);
  }
}

//处理函数声明结构
static void type_extfuncdec(ParserState* pstate, Type rettype, CstFuncDec* funcdec) {
  /* create function */
  Symbol* symbol = new_function(pstate, funcdec->funcname, funcdec->lineno);
  BaseType* functype = typetbl_newfunction(&pstate->typetbl);
  utils_oom_ifnull(functype);
  functype->funcion_rettype = rettype;
  symbol->attr.type = maketype(functype->id, 0);
  /* handle parameters */
  CstList* paramlist = (CstList*)funcdec->paramlist;
  while (paramlist) {
    CstParam* param = cstlist_get(paramlist, CstParam*);
    assert(cst_checkkind(param, CST_PARAM));
    BaseTypeId base = type_specifier(pstate, param->specifier);
    type_vardec(pstate, param->vardec, base, 0);
    paramlist = cstlist_next(paramlist);
  }
}

//处理全局定义结构列表（包括函数定义，全局变量定义）
static void type_extdeflist(ParserState* pstate, CstList* extdeflist) {
  while (extdeflist) {
    Cst* extdef = cstlist_get(extdeflist, Cst*);
    if (cst_checkkind(extdef, CST_EXTDEFFUNC)) {
      CstExtDefFunc* extdeffunc = (CstExtDefFunc*)extdef;
      BaseTypeId base = type_specifier(pstate, extdeffunc->specifier);
      type_extfuncdec(pstate, maketype(base, 0), (CstFuncDec*)extdeffunc->funcdec);
      /* handle function body (compound statement) */
      assert(cst_checkkind(extdeffunc->stmt, CST_STMTCOMP));
      type_stmtcomp(pstate, (CstStmtComp*)extdeffunc->stmt);
    } else {
      assert(cst_checkkind(extdef, CST_EXTDEFDEC));
      CstExtDefDec* extdefdec = (CstExtDefDec*)extdef;
      BaseTypeId base = type_specifier(pstate, extdefdec->specifier);
      type_extdeclist(pstate, base, (CstList*)extdefdec->declist);
    }
    extdeflist = cstlist_next(extdeflist);
  }
}

//处理在结构体中的定义列表，将这些定义的变量名和它们的类型记录在fileddesc中
static void type_deflist_instruct(ParserState* pstate, CstList* deflist, StructFieldDesc* fielddesc) {
  while (deflist) {
    CstDef* def = cstlist_get(deflist, CstDef*);
    assert(cst_checkkind(def, CST_DEF));
    BaseTypeId base = type_specifier(pstate, def->specifier); 
    fielddesc += type_declist_instruct(pstate, base, (CstList*)def->declist, fielddesc);
    deflist = cstlist_next(deflist);
  }
}

//处理函数体中的定义列表，并将变量名和类型插入符号表
static void type_deflist(ParserState* pstate, CstList* deflist) {
  while (deflist) {
    CstDef* def = cstlist_get(deflist, CstDef*);
    assert(cst_checkkind(def, CST_DEF));
    BaseTypeId base = type_specifier(pstate, def->specifier); 
    type_declist(pstate, base, (CstList*)def->declist);
    deflist = cstlist_next(deflist);
  }
}
//对于speificer中的结构体定义由该函数处理 struct A{int a;}  a;
//这个函数在类型表中插入一个新的结构体类型
//如果指定了结构体的标签名，那么在符号表中插入结构体标签名和其类型
//返回这个结构体基类型ID
static BaseTypeId type_structdef(ParserState* pstate, CstStructDef* structdef) {
  CstList* deflist = (CstList*)structdef->deflist;
  size_t nfield = struct_nfield(pstate, deflist);
  BaseType* structtype = typetbl_newstruct(&pstate->typetbl, nfield);
  utils_oom_ifnull(structtype);
  type_deflist_instruct(pstate, deflist, structtype->structtype->fields);
  if (structdef->tagname != pstate->dummyname) {
    Symbol* symbol = new_struct(pstate, structdef->tagname, structdef->lineno);
    symbol->attr.type = maketype(structtype->id, 0);
  }
  return structtype->id;
}
//对于speificer中的结构体引用由该函数处理 struct A a;
//这个函数在符号表中查找指定标签名的结构体，并返回基类型ID
static BaseTypeId type_structtype(ParserState* pstate, CstStructType* structtype) {
  Symbol* symbol = symtbl_search(pstate->symtbl, structtype->tagname);
  if (!symbol || symbol->attr.kind != SYMBOL_STRUCT) {
    mycc_error(ETYPE_STRUCT_UNDEF, structtype->lineno, "undefined struct name: %s", strtbl_getstring(pstate->strtbl, structtype->tagname));
    return pstate->commontype.errortype;
  }
  assert(symbol->attr.type.dim == 0); /* structure is not a array */
  return symbol->attr.type.basetype;
}

//处理specificer，返回这个specificer所表示的基类型ID
static BaseTypeId type_specifier(ParserState* pstate, Cst* cst) {
  if (cst_checkkind(cst, CST_TYPE)) {
    CstType* csttype = (CstType*)cst;
    assert(csttype->val == BTYPE_INT || csttype->val == BTYPE_FLOAT);
    return csttype->val == BTYPE_INT ? pstate->commontype.inttype
                                     : pstate->commontype.floattype;
  } else if (cst_checkkind(cst, CST_STRUCTDEF)) {
    return type_structdef(pstate, (CstStructDef*)cst);
  } else {
    assert(cst_checkkind(cst, CST_STRUCTTYPE));
    return type_structtype(pstate, (CstStructType*)cst);
  }
}

static bool is_lvalue(ParserState* pstate, Cst* expr) {//判断左值
  (void)pstate;
  return cst_checkkind(expr, CST_ID) || cst_checkkind(expr, CST_INDEX) || cst_checkkind(expr, CST_DOT);//标识符，数组下标，结构体成员访问
}

static Type type_exprbin(ParserState* pstate, CstBin* binary) {//计算二元表达式类型
  Type ltype = type_expr(pstate, binary->loperand);
  Type rtype = type_expr(pstate, binary->roperand);//计算左右子表达式类型
  if (haserror(pstate, ltype, rtype))//左右至少有一个是error
    return makeerror(pstate);
    //比较运算符
  switch (binary->op) {
    case CSTOP_LE:
    case CSTOP_LT:
    case CSTOP_GE:
    case CSTOP_GT:
    case CSTOP_NE:
    case CSTOP_EQ: {
      if (isinteger(pstate, ltype) && isinteger(pstate, rtype)) {
        return ltype;
      } else if (isfloat(pstate, ltype) && isfloat(pstate, rtype)) {
        return maketype(pstate->commontype.inttype, 0);
      }
      mycc_error(ETYPE_OPERAND_TYPEMISMATCH, binary->lineno, "the types of operands in the comparison operation do not match");
      return makeerror(pstate);
    }
    //计算运算符
    case CSTOP_ADD:
    case CSTOP_SUB:
    case CSTOP_MUL:
    case CSTOP_DIV: {
      if (isinteger(pstate, ltype) && isinteger(pstate, rtype)) {
        return ltype;
      } else if (isfloat(pstate, ltype) && isfloat(pstate, rtype)) {
        return ltype;
      }
      mycc_error(ETYPE_OPERAND_TYPEMISMATCH, binary->lineno, "the types of operands in the arithmetic operation do not match");
      return makeerror(pstate);
    }
    //逻辑运算符
    case CSTOP_OR:
    case CSTOP_AND: {
      if (isinteger(pstate, ltype) && isinteger(pstate, rtype))
        return ltype;
      mycc_error(ETYPE_OPERAND_TYPEMISMATCH, binary->lineno, "logic operator can only be applied to integer");
      return makeerror(pstate);
    }
    //赋值运算符
    case CSTOP_ASSIGN: {
      if (!is_lvalue(pstate, binary->loperand)) { //先判断是否是左值（是否可以被赋值的结构）291行
        mycc_error(ETYPE_ASSIGN_RVALUE, binary->lineno, "lvalue expected in left hand side of assignment");
        return makeerror(pstate);
      } else if (!type_is_same(ltype, rtype)) {
        mycc_error(ETYPE_ASSIGN_TYPEMISMATCH, binary->lineno, "type mismatching in assignment");
        return makeerror(pstate);
      }
      return ltype;
    }
    default: {
      assert(false);
      return makeerror(pstate);
    }
  }
}

static Type type_exprpre(ParserState* pstate, CstPre* prefix) {//计算前缀运算符类型
  Type type = type_expr(pstate, prefix->operand);
  if (iserror(pstate, type))
    return type;
  switch (prefix->op) {
    case CSTOP_NEG: {
      if (isinteger(pstate, type) || isfloat(pstate, type))
        return type;
      mycc_error(ETYPE_OPERAND_TYPEMISMATCH, prefix->lineno, "unary '-' can only be applied to integer and floating number");
      return makeerror(pstate);
    }
    case CSTOP_NOT: {//逻辑非
      if (isinteger(pstate, type)) 
        return type;
      mycc_error(ETYPE_OPERAND_TYPEMISMATCH, prefix->lineno, "logic operator can only be applied to integer");
      return makeerror(pstate);
    }
    default: {
      assert(false);
      return makeerror(pstate);
    }
  }
}

static Type type_exprcall(ParserState* pstate, CstCall* call) {//函数调用表达式
  Type functype = type_expr(pstate, call->func);//函数部分求类型
  if (iserror(pstate, functype))
    return functype;
  Type resulttype;
  if (!isfunction(pstate, functype)) {//不是函数类型
    mycc_error(ETYPE_CALL_NOTCALLABLE, call->func->lineno, "expected a function type");
    resulttype = makeerror(pstate);
  } else {
    resulttype = typetbl_get(&pstate->typetbl, functype.basetype)->funcion_rettype;//从类型表中取出basetype，返回它的function return type
  }
  CstList* arglist = (CstList*)call->args;//实参部分，对每一个参数求值（实验不要求检查参数个数是否匹配，所以求完后什么都不做）
  while (arglist) {
    Cst* arg = cstlist_get(arglist, Cst*);
    type_expr(pstate, arg);
    arglist = cstlist_next(arglist);
  }
  return resulttype;
}

/* find struct field by field name.
 * return NULL if no such field. */
static StructFieldDesc* get_structfield(ParserState* pstate, StructType* structtype, StringId fieldname) {
  char* name = strtbl_getstring(pstate->strtbl, fieldname);
  for (size_t i = 0; i < structtype->nfield; ++i) {
    if (0 == strcmp(name, strtbl_getstring(pstate->strtbl, structtype->fields[i].name)))
      return &structtype->fields[i];
  }
  return NULL;
}

static Type type_exprdot(ParserState* pstate, CstDot* dot) {//结构体成员访问表达式 struct A ；A.a;
  Type type = type_expr(pstate, dot->loperand);//结构体部分的类型
  if (iserror(pstate, type))
    return type;
  if (!isstruct(pstate, type)) {
    mycc_error(ETYPE_CALL_NOTCALLABLE, dot->loperand->lineno, "expected a structure");
    return makeerror(pstate);
  }
  StructType* structtype = typetbl_get(&pstate->typetbl, type.basetype)->structtype;//从类型表中获取structtype
  StructFieldDesc* fielddesc = get_structfield(pstate, structtype, dot->name);//根据成员名字找成员，返回成员的描述符
  if (!fielddesc) {
    mycc_error(ETYPE_STRUCT_UNKNOWNFIELD, dot->lineno, "no field named '%s' in this structure", strtbl_getstring(pstate->strtbl, dot->name));
    return makeerror(pstate);
  }
  return fielddesc->type;
}

static Type type_exprindex(ParserState* pstate, CstIndex* indexexpr) {//数组下标表达式
  Type arrtype = type_expr(pstate, indexexpr->arr);//数组部分的类型
  if (iserror(pstate, arrtype))
    return arrtype;
  if (!isarray(pstate, arrtype)) {
    mycc_error(ETYPE_INDEX_NOTARRAY, indexexpr->arr->lineno, "expected an array");
    return makeerror(pstate);
  }
  Type indextype = type_expr(pstate, indexexpr->index);//下标类型
  if (iserror(pstate, indextype))
    return indextype;
  if (!isinteger(pstate, indextype)) {//不是整数就报错
    mycc_error(ETYPE_INDEX_NOTINTEGER, indexexpr->index->lineno, "expected an integer");
    return makeerror(pstate);
  }
  --arrtype.dim;//返回数组结果类型就是它维度减一
  return arrtype;
}

static Type type_expr(ParserState* pstate, Cst* cst) {//输入表达式语法树，返回表达式类型
  switch (cst->kind) {
    case CST_BIN: {
      return type_exprbin(pstate, (CstBin*)cst);
    }
    case CST_PRE: {
      return type_exprpre(pstate, (CstPre*)cst);
    }
    case CST_CALL: {
      return type_exprcall(pstate, (CstCall*)cst);
    }
    case CST_DOT: {
      return type_exprdot(pstate, (CstDot*)cst);
    }
    case CST_INDEX: {
      return type_exprindex(pstate, (CstIndex*)cst);
    }
    case CST_ID: {
      CstId* id = (CstId*)cst;
      Symbol* symbol = get_variable_or_function(pstate, id->val, id->lineno);//在符号表中查找以这个符号命名的变量或者函数
      if (!symbol) return makeerror(pstate);//没找到
      return symbol->attr.type;
    }
    case CST_FLOAT: {
      return maketype(pstate->commontype.floattype, 0);
    }
    case CST_INT: {
      return maketype(pstate->commontype.inttype, 0);
    }
    default: {
      assert(false);
      return makeerror(pstate);
    }
  }
}

static void type_stmtcomp(ParserState* pstate, CstStmtComp* comp) {//复合语句
  type_deflist(pstate, (CstList*)comp->deflist);//定义列表的类型检查
  CstList* stmtlist = (CstList*)comp->stmtlist;
  while (stmtlist) {
    type_stmt(pstate, cstlist_get(stmtlist, Cst*));//可以对任何语句做类型检查(递归调用)
    stmtlist = cstlist_next(stmtlist);
  }
}

static void type_stmtif(ParserState* pstate, CstStmtIf* stmtif) {//if语句
  Type condtype = type_expr(pstate, stmtif->cond);//条件部分求值
  if (!iserror(pstate, condtype) && !isinteger(pstate, condtype))
    mycc_error(ETYPE_OPERAND_TYPEMISMATCH, stmtif->cond->lineno, "condition should be evaluated to be an integer");
  type_stmt(pstate, stmtif->if_stmt);//if后边部分类型检查
  if (stmtif->else_stmt)//else部分
    type_stmt(pstate, stmtif->else_stmt);
}

static void type_stmtwhile(ParserState* pstate, CstStmtWhile* stmtwhile) {//while语句
  Type condtype = type_expr(pstate, stmtwhile->cond);//条件部分求值
  if (!iserror(pstate, condtype) && !isinteger(pstate, condtype))//不是错误不是整数就报错
    mycc_error(ETYPE_OPERAND_TYPEMISMATCH, stmtwhile->cond->lineno, "condition should be evaluated to be an integer");
  type_stmt(pstate, stmtwhile->stmt);//复合语句
}

static void type_stmtexpr(ParserState* pstate, CstStmtExpr* stmtexpr) {//表达式
  type_expr(pstate, stmtexpr->expr);
}

static void type_stmtreturn(ParserState* pstate, CstStmtReturn* stmtreturn) {//return
  type_expr(pstate, stmtreturn->expr);
}

static void type_stmt(ParserState* pstate, Cst* stmt) {//语句类型检查
  switch (stmt->kind) {
    case CST_STMTWHILE: {
      type_stmtwhile(pstate, (CstStmtWhile*)stmt);
      break;
    }
    case CST_STMTIF: {
      type_stmtif(pstate, (CstStmtIf*)stmt);
      break;
    }
    case CST_STMTEXPR: {
      type_stmtexpr(pstate, (CstStmtExpr*)stmt);
      break;
    }
    case CST_STMTRETURN: {
      type_stmtreturn(pstate, (CstStmtReturn*)stmt);
      break;
    }
    case CST_STMTCOMP: {
      type_stmtcomp(pstate, (CstStmtComp*)stmt);
      break;
    }
    default: {
      assert(false);
      break;
    }
  }
}
