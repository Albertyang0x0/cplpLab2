//语法树结构，在语法树上进行类型检查
#ifndef _CST_H_
#define _CST_H_

#include "strtbl.h"
#include "utils.h"

#include <stdint.h>
#include <stdlib.h>
//所有结点共有的属性 next链接所有节点
#define CstHeader Cst* next; CstKind kind; unsigned lineno//指针，种类，行号

typedef int32_t Int;
typedef float Float;

typedef enum tagBasicType {
  BTYPE_INT,
  BTYPE_FLOAT
} BasicType;

typedef enum tagCstKind {
  CST_ERROR,
  CST_INT,
  CST_FLOAT,
  CST_TYPE,
  CST_ID,

  // 表达式节点
  CST_BIN,      // 二元运算表达式
  CST_PRE,      // 前缀
  CST_CALL,
  CST_INDEX,    // 数组索引
  CST_DOT,
  CST_DEF,          //符号定义结构，包括specifier和声明列表
  CST_DEC,          //变量声明结构，可能包括初始值
  CST_EXTDEFDEC,    //全局变量声明
  CST_EXTDEFFUNC,   //函数定义结构
  CST_FUNCDEC,      //函数声明结构
  CST_PARAM,

  CST_LIST,//链表节点

  CST_STMTIF,
  CST_STMTRETURN,
  CST_STMTEXPR,
  CST_STMTWHILE,
  CST_STMTCOMP,
  CST_STRUCTDEF,
  CST_STRUCTTYPE,
  CST_DECARR,
} CstKind;

typedef enum tagCstOp {//运算符枚举值
  CSTOP_NE,
  CSTOP_EQ,
  CSTOP_LE,
  CSTOP_LT,
  CSTOP_GE,
  CSTOP_GT,
  CSTOP_ASSIGN,
  CSTOP_ADD,
  CSTOP_SUB,
  CSTOP_MUL,
  CSTOP_DIV,
  CSTOP_AND,
  CSTOP_OR,
  CSTOP_NOT,
  CSTOP_NEG, /* negative, 一元 '-' */
} CstOp;

#define is_terminal(kind)         ((kind) <= CST_WHILE)
#define is_noattrtoken(kind)      ((kind) >= CST_SEMI && (kind) <= CST_WHILE)

#define make_cstkind(macro)       (CST_##macro)

#define cst_checkkind(cst, _kind) ((cst)->kind == (_kind))//检查节点kind

typedef struct tagCst Cst;
struct tagCst {//所有语法树节点都可以用这个结构体来指，cst是一个抽象的基类
  CstHeader;
};
//终结符
typedef struct tagCstInt {
  CstHeader;
  Int val;
} CstInt;

typedef struct tagCstFloat {
  CstHeader;
  Float val;
} CstFloat;

typedef struct tagCstType {//int float关键字
  CstHeader;
  BasicType val;
} CstType;

typedef struct tagCstId {
  CstHeader;
  StringId val;
} CstId;
/* 二元运算表达式 */
typedef struct tagCstBin {
  CstHeader;
  CstOp op;
  Cst* loperand;
  Cst* roperand;
} CstBin;

/* 前缀运算表达式 */
typedef struct tagCstPre {
  CstHeader;
  CstOp op;
  Cst* operand;
} CstPre;

/* 通用的列表。
 * 参数列表，ExtDefList等一切具有列表的结构都使用该结构构造
 */
typedef struct tagCstList {
  CstHeader;
  Cst* cst;
  Cst* nextcst;
} CstList;

/* 函数调用表达式 */
typedef struct tagCstCall {
  CstHeader;
  Cst* func;
  Cst* args;
} CstCall;

/* 结构体成员访问表达式 */
typedef struct tagCstDot {
  CstHeader;
  Cst* loperand;
  StringId name;
} CstDot;

/* 数组索引表达式 */
typedef struct tagCstIndex {
  CstHeader;
  Cst* arr;
  Cst* index;
} CstIndex;

typedef struct tagCstDec {
  CstHeader;
  Cst* vardec;
  Cst* initval;
} CstDec;

typedef struct tagCstExtDefDec {
  CstHeader;
  Cst* specifier;
  Cst* declist;
} CstExtDefDec;

typedef struct tagCstExtDefFunc {
  CstHeader;
  Cst* specifier;
  Cst* funcdec;
  Cst* stmt;
} CstExtDefFunc;

typedef struct tagCstFuncDec {
  CstHeader;
  StringId funcname;
  Cst* paramlist;
} CstFuncDec;

typedef struct tagCstParam {
  CstHeader;
  Cst* specifier;
  Cst* vardec;
} CstParam;

typedef struct tagCstDef {
  CstHeader;
  Cst* specifier;
  Cst* declist;
} CstDef;

typedef struct tagCstStmtIf {
  CstHeader;
  Cst* cond;//条件
  Cst* if_stmt;//if后边的代码块
  Cst* else_stmt;
} CstStmtIf;

typedef struct tagCstStmtReturn {
  CstHeader;
  Cst* expr;
} CstStmtReturn;

typedef struct tagCstStmtExpr {
  CstHeader;
  Cst* expr;
} CstStmtExpr;

typedef struct tagCstStmtWhile {
  CstHeader;
  Cst* cond;
  Cst* stmt;
} CstStmtWhile;

/* 复合语句 */
typedef struct tagCstStmtComp {
  CstHeader;
  Cst* deflist;
  Cst* stmtlist;
} CstStmtComp;

/* 结构体定义
 * struct A { int a; } a; */
typedef struct tagCstStructDef {
  CstHeader;
  StringId tagname;
  Cst* deflist;
} CstStructDef;

/* 仅结构体名称
 * struct A a; */
typedef struct tagCstStructType {
  CstHeader;
  StringId tagname;
} CstStructType;

/* 数组的声明结构 */
typedef struct tagCstDecArr {
  CstHeader;
  Cst* vardec;
  size_t nelem;
} CstDecArr;

typedef struct tagCstNoAttrToken {
  CstHeader;
} CstNoAttrToken;

static inline void* safe_alloc(size_t size) {
  void* p = malloc(size);
  utils_oom_ifnull(p);
  return p;
}

#define cstalloc(csttype)         (csttype*)safe_alloc(sizeof (csttype))

static inline CstInt* cst_create_int(unsigned lineno, Int val) {
  CstInt* cst = cstalloc(CstInt);
  cst->kind = make_cstkind(INT);
  cst->lineno = lineno;
  cst->val = val;
  return cst;
}

static inline CstFloat* cst_create_float(unsigned lineno, Float val) {
  CstFloat* cst = cstalloc(CstFloat);
  cst->kind = make_cstkind(FLOAT);
  cst->lineno = lineno;
  cst->val = val;
  return cst;
}

static inline CstType* cst_create_type(unsigned lineno, BasicType val) {
  CstType* cst = cstalloc(CstType);
  cst->kind = make_cstkind(TYPE);
  cst->lineno = lineno;
  cst->val = val;
  return cst;
}

static inline CstId* cst_create_id(unsigned lineno, StringId val) {
  CstId* cst = cstalloc(CstId);
  cst->kind = make_cstkind(ID);
  cst->lineno = lineno;
  cst->val = val;
  return cst;
}

//历史遗留，不用管
static inline CstNoAttrToken* cst_create_noattrtoken(unsigned lineno, CstKind kind) {
  CstNoAttrToken* cst = cstalloc(CstNoAttrToken);
  cst->kind = kind;
  cst->lineno = lineno;
  return cst;
}

static inline CstBin* cst_create_bin(unsigned lineno, CstOp op, Cst* lop, Cst* rop) {
  CstBin* cst = cstalloc(CstBin);
  cst->kind = CST_BIN;
  cst->lineno = lineno;
  cst->op = op;
  cst->loperand = lop;
  cst->roperand = rop;
  return cst;
}

static inline CstPre* cst_create_pre(unsigned lineno, CstOp op, Cst* operand) {
  CstPre* cst = cstalloc(CstPre);
  cst->kind = CST_PRE;
  cst->lineno = lineno;
  cst->op = op;
  cst->operand = operand;
  return cst;
}

static inline CstList* cst_create_list(unsigned lineno, Cst* cst, Cst* list) {
  CstList* newlist = cstalloc(CstList);
  newlist->kind = CST_LIST;
  newlist->lineno = lineno;
  newlist->cst = cst;
  newlist->nextcst = list;
  return newlist;
}

static inline CstCall* cst_create_call(unsigned lineno, Cst* func, Cst* args) {
  CstCall* cst = cstalloc(CstCall);
  cst->kind = CST_CALL;
  cst->lineno = lineno;
  cst->func = func;
  cst->args = args;
  return cst;
}

static inline CstDec* cst_create_dec(unsigned lineno, Cst* vardec, Cst* initval) {
  CstDec* cst = cstalloc(CstDec);
  cst->kind = CST_DEC;
  cst->lineno = lineno;
  cst->vardec = vardec;
  cst->initval = initval;
  return cst;
}

static inline CstExtDefDec* cst_create_extdefdec(unsigned lineno, Cst* specifier, Cst* declist) {
  CstExtDefDec* cst = cstalloc(CstExtDefDec);
  cst->kind = CST_EXTDEFDEC;
  cst->lineno = lineno;
  cst->specifier = specifier;
  cst->declist = declist;
  return cst;
}

static inline CstExtDefFunc* cst_create_extdeffunc(unsigned lineno, Cst* specifier, Cst* funcdec, Cst* stmt) {
  CstExtDefFunc* cst = cstalloc(CstExtDefFunc);
  cst->kind = CST_EXTDEFFUNC;
  cst->lineno = lineno;
  cst->specifier = specifier;
  cst->funcdec = funcdec;
  cst->stmt = stmt;
  return cst;
}

static inline CstFuncDec* cst_create_funcdec(unsigned lineno, StringId funcname, Cst* paramlist) {
  CstFuncDec* cst = cstalloc(CstFuncDec);
  cst->kind = CST_FUNCDEC;
  cst->lineno = lineno;
  cst->funcname = funcname;
  cst->paramlist = paramlist;
  return cst;
}

static inline CstParam* cst_create_param(unsigned lineno, Cst* specifier, Cst* vardec) {
  CstParam* cst = cstalloc(CstParam);
  cst->kind = CST_PARAM;
  cst->lineno = lineno;
  cst->specifier = specifier;
  cst->vardec = vardec;
  return cst;
}

static inline CstDef* cst_create_def(unsigned lineno, Cst* specifier, Cst* declist) {
  CstDef* cst = cstalloc(CstDef);
  cst->kind = CST_DEF;
  cst->lineno = lineno;
  cst->specifier = specifier;
  cst->declist = declist;
  return cst;
}

static inline CstStmtIf* cst_create_stmtif(unsigned lineno, Cst* cond, Cst* if_stmt, Cst* else_stmt) {
  CstStmtIf* cst = cstalloc(CstStmtIf);
  cst->kind = CST_STMTIF;
  cst->lineno = lineno;
  cst->cond = cond;
  cst->if_stmt = if_stmt;
  cst->else_stmt = else_stmt;
  return cst;
}

static inline CstStmtReturn* cst_create_stmtreturn(unsigned lineno, Cst* expr) {
  CstStmtReturn* cst = cstalloc(CstStmtReturn);
  cst->kind = CST_STMTRETURN;
  cst->lineno = lineno;
  cst->expr = expr;
  return cst;
}

static inline CstStmtExpr* cst_create_stmtexpr(unsigned lineno, Cst* expr) {
  CstStmtExpr* cst = cstalloc(CstStmtExpr);
  cst->kind = CST_STMTEXPR;
  cst->lineno = lineno;
  cst->expr = expr;
  return cst;
}

static inline CstStmtWhile* cst_create_stmtwhile(unsigned lineno, Cst* cond, Cst* stmt) {
  CstStmtWhile* cst = cstalloc(CstStmtWhile);
  cst->kind = CST_STMTWHILE;
  cst->lineno = lineno;
  cst->cond = cond;
  cst->stmt = stmt;
  return cst;
}

static inline CstStmtComp* cst_create_stmtcomp(unsigned lineno, Cst* deflist, Cst* stmtlist) {
  CstStmtComp* cst = cstalloc(CstStmtComp);
  cst->kind = CST_STMTCOMP;
  cst->lineno = lineno;
  cst->deflist = deflist;
  cst->stmtlist = stmtlist;
  return cst;
}

static inline CstStructDef* cst_create_structdef(unsigned lineno, StringId tagname, Cst* deflist) {
  CstStructDef* cst = cstalloc(CstStructDef);
  cst->kind = CST_STRUCTDEF;
  cst->lineno = lineno;
  cst->tagname = tagname;
  cst->deflist = deflist;
  return cst;
}

static inline CstStructType* cst_create_structtype(unsigned lineno, StringId tagname) {
  CstStructType* cst = cstalloc(CstStructType);
  cst->kind = CST_STRUCTTYPE;
  cst->lineno = lineno;
  cst->tagname = tagname;
  return cst;
}

static inline CstDecArr* cst_create_decarr(unsigned lineno, Cst* vardec, size_t nelem) {
  CstDecArr* cst = cstalloc(CstDecArr);
  cst->kind = CST_DECARR;
  cst->lineno = lineno;
  cst->vardec = vardec;
  cst->nelem = nelem;
  return cst;
}

static inline CstDot* cst_create_dot(unsigned lineno, Cst* operand, StringId name) {
  CstDot* cst = cstalloc(CstDot);
  cst->kind = CST_DOT;
  cst->lineno = lineno;
  cst->loperand = operand;
  cst->name = name;
  return cst;
}

static inline CstIndex* cst_create_index(unsigned lineno, Cst* arr, Cst* index) {
  CstIndex* cst = cstalloc(CstIndex);
  cst->kind = CST_INDEX;
  cst->lineno = lineno;
  cst->arr = arr;
  cst->index = index;
  return cst;
}


/* some helpful functions and macros */
static inline CstList* cstlist_next(CstList* list) {
  return (CstList*)list->nextcst;
}

#define cstlist_get(list, type)   ((type)((list)->cst))

#endif
