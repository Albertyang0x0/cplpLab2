#ifndef _TYPETBL_H_
#define _TYPETBL_H_

#include "array.h"
#include "cst.h"
#include "strtbl.h"
#include "type.h"

//类型表只存基类型，其他所有类型（数组）都可以由基类型加数组维数表示
typedef enum tagTypeKind {
  TYPE_FUNC,
  TYPE_STRUCT,  //结构体类型
  TYPE_BASIC,   //基本数据类型(int, float)
  TYPE_ERROR,   //用来表示类型错误的类型
} TypeKind;


//结构体成员描述符
typedef struct tagStructFieldDesc {
  StringId name;
  Type type;
} StructFieldDesc;

typedef struct tagStructType {
  size_t nfield;
  StructFieldDesc fields[];
} StructType;

//基类型结构体
typedef struct tagBaseType {
  BaseTypeId id;//表示基类型在数组的下标
  TypeKind kind;//三种，上边定义了
  union {
    BasicType basic;//cst.h里定义
    /* 在本实验的要求下，不需要检查函数参数是否匹配，因此只需记录其返回类型 */
    Type funcion_rettype;
    StructType* structtype;
  };
} BaseType;

 /* 类型表本体 */
typedef struct tagTypeTbl {
  Array types;   //动态扩容的数组
} TypeTbl;


bool typetbl_init(TypeTbl* typetbl);
void typetbl_destroy(TypeTbl* typetbl);
//在类型表中新建结构体型的基类型，nfield是结构体的域（成员）的个数
BaseType* typetbl_newstruct(TypeTbl* typetbl, size_t nfield);
BaseType* typetbl_newerror(TypeTbl* typetbl);
BaseType* typetbl_newfunction(TypeTbl* typetbl);
BaseType* typetbl_newbasic(TypeTbl* typetbl, BasicType basictype);

static inline BaseType* typetbl_get(TypeTbl* typetbl, BaseTypeId id) {
  assert(id < array_size(&typetbl->types));
  return (BaseType*)array_access(&typetbl->types, id);
}

#endif
