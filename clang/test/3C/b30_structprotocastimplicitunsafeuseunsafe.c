// RUN: rm -rf %t*
// RUN: 3c -base-dir=%S -alltypes -addcr %s -- | FileCheck -match-full-lines -check-prefixes="CHECK_ALL","CHECK" %s
// RUN: 3c -base-dir=%S -addcr %s -- | FileCheck -match-full-lines -check-prefixes="CHECK_NOALL","CHECK" %s
// RUN: 3c -base-dir=%S -addcr %s -- | %clang -c -fcheckedc-extension -x c -o /dev/null -
// RUN: 3c -base-dir=%S -alltypes -output-dir=%t.checked %s --
// RUN: 3c -base-dir=%t.checked -alltypes %t.checked/b30_structprotocastimplicitunsafeuseunsafe.c -- | diff %t.checked/b30_structprotocastimplicitunsafeuseunsafe.c -
#include <stddef.h>
extern _Itype_for_any(T) void *calloc(size_t nmemb, size_t size)
    : itype(_Array_ptr<T>) byte_count(nmemb * size);
extern _Itype_for_any(T) void free(void *pointer
                                   : itype(_Array_ptr<T>) byte_count(0));
extern _Itype_for_any(T) void *malloc(size_t size)
    : itype(_Array_ptr<T>) byte_count(size);
extern _Itype_for_any(T) void *realloc(void *pointer
                                       : itype(_Array_ptr<T>) byte_count(1),
                                         size_t size)
    : itype(_Array_ptr<T>) byte_count(size);
extern int printf(const char *restrict format
                  : itype(restrict _Nt_array_ptr<const char>), ...);
extern _Unchecked char *strcpy(char *restrict dest, const char *restrict src
                               : itype(restrict _Nt_array_ptr<const char>));

struct np {
  int x;
  int y;
};

struct p {
  int *x;
  //CHECK: _Ptr<int> x;
  char *y;
  //CHECK: _Ptr<char> y;
};

struct r {
  int data;
  struct r *next;
  //CHECK: struct r *next;
};

struct r *sus(struct r *, struct r *);
//CHECK: _Ptr<struct r> sus(_Ptr<struct r> x, _Ptr<struct r> y);

struct r *foo() {
  //CHECK: _Ptr<struct r> foo(void) {
  struct r *x;
  //CHECK: struct r *x;
  struct r *y;
  //CHECK: struct r *y;
  x->data = 2;
  y->data = 1;
  x->next = &y;
  y->next = &x;
  struct r *z = (struct r *)sus(x, y);
  //CHECK: _Ptr<struct r> z = (_Ptr<struct r>)sus(_Assume_bounds_cast<_Ptr<struct r>>(x), _Assume_bounds_cast<_Ptr<struct r>>(y));
  return z;
}

struct np *bar() {
  //CHECK_NOALL: struct np *bar(void) : itype(_Ptr<struct np>) {
  //CHECK_ALL: struct np *bar(void) : itype(_Array_ptr<struct np>) {
  struct r *x;
  //CHECK: struct r *x;
  struct r *y;
  //CHECK: struct r *y;
  x->data = 2;
  y->data = 1;
  x->next = &y;
  y->next = &x;
  struct np *z = sus(x, y);
  //CHECK: struct np *z = ((struct r *)sus(_Assume_bounds_cast<_Ptr<struct r>>(x), _Assume_bounds_cast<_Ptr<struct r>>(y)));
  z += 2;
  return z;
}

struct r *sus(struct r *x, struct r *y) {
  //CHECK: _Ptr<struct r> sus(_Ptr<struct r> x, _Ptr<struct r> y) {
  x->next += 1;
  struct r *z = malloc(sizeof(struct r));
  //CHECK: _Ptr<struct r> z = malloc<struct r>(sizeof(struct r));
  z->data = 1;
  z->next = 0;
  return z;
}
