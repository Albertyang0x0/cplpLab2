#ifndef _TYPE_H_
#define _TYPE_H_


typedef unsigned BaseTypeId;
/* 在实验要求的条件下，
 只要数组的基类型和维数相同，数组就被认为是相同的
 所以我们只需要记录基本类型和维度。
  （非数组类型被视为具有 0 维）。 */
typedef struct tagType {
  BaseTypeId basetype;
  unsigned dim;
} Type;

#define type_is_same(t1, t2)  ((t1).basetype == (t2).basetype && (t1).dim == (t2).dim)

static inline Type maketype(BaseTypeId base, unsigned dim) {
  Type type = { base, dim };
  return type;
}

#endif
