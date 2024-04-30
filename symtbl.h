//符号表，用哈希表实现
#ifndef _SYMTBL_H_
#define _SYMTBL_H_

#include "strtbl.h"
#include "type.h"
#include "typetbl.h"
#include <stdlib.h>

typedef struct tagSymbol Symbol;

// 标识符号的种类
typedef enum tagSymBolKind {
  SYMBOL_VARIABLE,  //这个符号是一个变量
  SYMBOL_FUNCTION,  //函数
  SYMBOL_STRUCT,    //结构体名字
} SymbolKind;

typedef struct tagSymbolAttr {      //符号属性
  SymbolKind kind;
  Type type;    //类型
} SymbolAttr;

struct tagSymbol {//符号信息
  size_t hash;
  StringId name;    //符号名称
  SymbolAttr attr;  //符号属性
  Symbol* next;
};

typedef struct tagSymTbl SymTbl;
struct tagSymTbl {
  Symbol** array;
  size_t capacity;
  size_t size;
  StrTbl* strtbl;
};


bool symtbl_init(SymTbl* symtbl, size_t capacity, StrTbl* strtbl);
void symtbl_destroy(SymTbl* symtbl);
SymTbl* symtbl_create(size_t capacity, StrTbl* strtbl);
void symtbl_delete(SymTbl* symtbl);


Symbol* symtbl_insert(SymTbl* symtbl, StringId name);//插入符号
Symbol* symtbl_search(SymTbl* symtbl, StringId name);//查询符号

#endif
