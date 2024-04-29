#ifndef _PARSER_STATE_H_
#define _PARSER_STATE_H_

#include "cst.h"
#include "strtbl.h"
#include "symtbl.h"
#include "typetbl.h"

typedef struct tagParserState {//编译器的全局状态
  StrTbl* strtbl;
  SymTbl* symtbl;
  Cst* allcsts;         /*节点间链接的指针*/
  Cst* program;         /* 语法树根节点 */
  unsigned nerror;
  StringId dummyname;   //结构体定义的时候未给出名用dummyname表示/
  TypeTbl typetbl;
  struct {
    Cst* error;
  } noattrtokennode;
  struct {
    BaseTypeId inttype;
    BaseTypeId floattype;
    BaseTypeId errortype;
  } commontype;
} ParserState;

#define pstate_appendcst(pstate, cst)   pstate_appendcst_raw((pstate), (Cst*)(cst))

static inline Cst* pstate_appendcst_raw(ParserState* pstate, Cst* cst) {
  cst->next = pstate->allcsts;
  pstate->allcsts = cst;
  return cst;
}

void pstate_init(ParserState* pstate);
void pstate_destroy(ParserState* pstate);

#endif
