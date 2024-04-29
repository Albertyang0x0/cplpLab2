#include "symtbl.h"
#include <stdlib.h>
#include <string.h>

inline static size_t symtbl_hashing(StrTbl* strtbl, StringId name) {
  char* str = strtbl_getstring(strtbl, name);
  size_t hash = 0;
  while (*str)
    hash = (*str++) + (hash << 6) + (hash << 16) - hash;
  return hash;
}

static void symtbl_rehash(SymTbl* to, SymTbl* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  Symbol** from_array = from->array;
  Symbol** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    Symbol* symbol = from_array[i];
    while (symbol) {
      Symbol* tmp = symbol->next;
      size_t hashval = symbol->hash;
      size_t index = hashval & mask;
      symbol->next = to_array[index];
      to_array[index] = symbol;
      symbol = tmp;
    }
  }
  to->size = from->size;
  free(from->array);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool symtbl_expand(SymTbl* symtbl) {
  SymTbl newtbl;
  if (!symtbl_init(&newtbl, symtbl->capacity << 1, symtbl->strtbl))
    return false;
  symtbl_rehash(&newtbl, symtbl);
  *symtbl = newtbl;
  return true;
}

static void symtbl_bucket_free(Symbol* bucket) {
  while (bucket) {
    Symbol* tmp = bucket->next;
    free(bucket);
    bucket = tmp;
  }
}

inline static size_t pow_of_2_above(size_t num) {
  size_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool symtbl_init(SymTbl* symtbl, size_t capacity, StrTbl* strtbl) {
  if (!symtbl) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  Symbol** array = (Symbol**)malloc(sizeof (Symbol*) * capacity);
  if (!array) return false;
  for (size_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  symtbl->array = array;
  symtbl->capacity = capacity;
  symtbl->size = 0;
  symtbl->strtbl = strtbl;
  return true;
}

void symtbl_destroy(SymTbl* symtbl) {
  if (!symtbl) return;

  Symbol** array = symtbl->array;
  size_t capacity = symtbl->capacity;
  for (size_t i = 0; i < capacity; ++i)
    symtbl_bucket_free(array[i]);
  free(array);
}

SymTbl* symtbl_create(size_t capacity, StrTbl* strtbl) {
  SymTbl* symtbl = (SymTbl*)malloc(sizeof (SymTbl));
  if (!symtbl || !symtbl_init(symtbl, capacity, strtbl)) {
    free(strtbl);
  }
  return symtbl;
}

void symtbl_delete(SymTbl* symtbl) {
  symtbl_destroy(symtbl);
  free(symtbl);
}

Symbol* symtbl_insert(SymTbl* symtbl, StringId name) {//插入
  if (symtbl->size >= symtbl->capacity && !symtbl_expand(symtbl))//检查容量是否够
    return NULL;

  Symbol* newsymbol = (Symbol*)malloc(sizeof (Symbol));
  if (!newsymbol) return NULL;

  size_t hash = symtbl_hashing(symtbl->strtbl, name);//计算名字的哈希值
  size_t index = (symtbl->capacity - 1) & hash;
  newsymbol->name = name;
  newsymbol->hash = hash;
  //将newsymbol链接到桶的头部
  newsymbol->next = symtbl->array[index];
  symtbl->array[index] = newsymbol;
  symtbl->size++;
  return newsymbol;
}

Symbol* symtbl_search(SymTbl* symtbl, StringId name) {//搜索
  size_t hash = symtbl_hashing(symtbl->strtbl, name);   //计算哈希值
  size_t index = (symtbl->capacity - 1) & hash;         //根据哈希值得到该符号应当落入的桶的下标（取模，保证容量是二的幂次）
  Symbol* symbol = symtbl->array[index];
  //遍历这个桶中所有符号
  for (; symbol; symbol = symbol->next) {
    if (hash == symbol->hash &&
        0 == strcmp(strtbl_getstring(symtbl->strtbl, symbol->name),
                    strtbl_getstring(symtbl->strtbl, name))) {//前边用来加速判断
      return symbol;
    }
  }
  return NULL;
}
