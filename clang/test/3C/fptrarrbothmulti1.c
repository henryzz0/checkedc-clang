// RUN: rm -rf %t*
// RUN: 3c -base-dir=%S -addcr -alltypes -output-dir=%t.checkedALL %s %S/fptrarrbothmulti2.c --
// RUN: 3c -base-dir=%S -addcr -output-dir=%t.checkedNOALL %s %S/fptrarrbothmulti2.c --
// RUN: %clang -working-directory=%t.checkedNOALL -c fptrarrbothmulti1.c fptrarrbothmulti2.c
// RUN: FileCheck -match-full-lines -check-prefixes="CHECK_NOALL","CHECK" --input-file %t.checkedNOALL/fptrarrbothmulti1.c %s
// RUN: FileCheck -match-full-lines -check-prefixes="CHECK_ALL","CHECK" --input-file %t.checkedALL/fptrarrbothmulti1.c %s
// RUN: 3c -base-dir=%S -alltypes -output-dir=%t.checked %S/fptrarrbothmulti2.c %s --
// RUN: 3c -base-dir=%t.checked -alltypes -output-dir=%t.convert_again %t.checked/fptrarrbothmulti1.c %t.checked/fptrarrbothmulti2.c --
// RUN: test ! -f %t.convert_again/fptrarrbothmulti1.c
// RUN: test ! -f %t.convert_again/fptrarrbothmulti2.c

/******************************************************************************/

/*This file tests three functions: two callers bar and foo, and a callee sus*/
/*In particular, this file tests: using a function pointer and an array in
  tandem to do computations*/
/*For robustness, this test is identical to
fptrarrprotoboth.c and fptrarrboth.c except in that
the callee and callers are split amongst two files to see how
the tool performs conversions*/
/*In this test, foo will treat its return value safely, but sus and bar will
  not, through invalid pointer arithmetic, an unsafe cast, etc.*/

/******************************************************************************/

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

struct general {
  int data;
  struct general *next;
  //CHECK: _Ptr<struct general> next;
};

struct warr {
  int data1[5];
  //CHECK_NOALL: int data1[5];
  //CHECK_ALL: int data1 _Checked[5];
  char *name;
  //CHECK: _Ptr<char> name;
};

struct fptrarr {
  int *values;
  //CHECK: _Ptr<int> values;
  char *name;
  //CHECK: _Ptr<char> name;
  int (*mapper)(int);
  //CHECK: _Ptr<int (int )> mapper;
};

struct fptr {
  int *value;
  //CHECK: _Ptr<int> value;
  int (*func)(int);
  //CHECK: _Ptr<int (int )> func;
};

struct arrfptr {
  int args[5];
  //CHECK_NOALL: int args[5];
  //CHECK_ALL: int args _Checked[5];
  int (*funcs[5])(int);
  //CHECK_NOALL: int (*funcs[5])(int);
  //CHECK_ALL: _Ptr<int (int )> funcs _Checked[5];
};

static int add1(int x) {
  //CHECK: static int add1(int x) _Checked {
  return x + 1;
}

static int sub1(int x) {
  //CHECK: static int sub1(int x) _Checked {
  return x - 1;
}

static int fact(int n) {
  //CHECK: static int fact(int n) _Checked {
  if (n == 0) {
    return 1;
  }
  return n * fact(n - 1);
}

static int fib(int n) {
  //CHECK: static int fib(int n) _Checked {
  if (n == 0) {
    return 0;
  }
  if (n == 1) {
    return 1;
  }
  return fib(n - 1) + fib(n - 2);
}

static int zerohuh(int n) {
  //CHECK: static int zerohuh(int n) _Checked {
  return !n;
}

static int *mul2(int *x) {
  //CHECK: static _Ptr<int> mul2(_Ptr<int> x) _Checked {
  *x *= 2;
  return x;
}

int **sus(int *, int *);
//CHECK_NOALL: int **sus(int *x : itype(_Ptr<int>), int *y : itype(_Ptr<int>)) : itype(_Ptr<int *>);
//CHECK_ALL: _Array_ptr<_Ptr<int>> sus(int *x : itype(_Ptr<int>), _Array_ptr<int> y : count(5));

int **foo() {
  //CHECK_NOALL: _Ptr<int *> foo(void) {
  //CHECK_ALL: _Array_ptr<_Ptr<int>> foo(void) {

  int *x = malloc(sizeof(int));
  //CHECK: _Ptr<int> x = malloc<int>(sizeof(int));
  int *y = calloc(5, sizeof(int));
  //CHECK_NOALL: int *y = calloc<int>(5, sizeof(int));
  //CHECK_ALL: _Array_ptr<int> y : count(5) = calloc<int>(5, sizeof(int));
  int i;
  for (i = 0; i < 5; i++) {
    //CHECK_NOALL: for (i = 0; i < 5; i++) {
    //CHECK_ALL: for (i = 0; i < 5; i++) _Checked {
    y[i] = i + 1;
  }
  int **z = sus(x, y);
  //CHECK_NOALL: _Ptr<int *> z = sus(x, y);
  //CHECK_ALL: _Array_ptr<_Ptr<int>> z = sus(x, y);

  return z;
}

int **bar() {
  //CHECK_NOALL: int **bar(void) : itype(_Ptr<int *>) {
  //CHECK_ALL: _Array_ptr<_Ptr<int>> bar(void) {

  int *x = malloc(sizeof(int));
  //CHECK: _Ptr<int> x = malloc<int>(sizeof(int));
  int *y = calloc(5, sizeof(int));
  //CHECK_NOALL: int *y = calloc<int>(5, sizeof(int));
  //CHECK_ALL: _Array_ptr<int> y : count(5) = calloc<int>(5, sizeof(int));
  int i;
  for (i = 0; i < 5; i++) {
    //CHECK_NOALL: for (i = 0; i < 5; i++) {
    //CHECK_ALL: for (i = 0; i < 5; i++) _Checked {
    y[i] = i + 1;
  }
  int **z = sus(x, y);
  //CHECK_NOALL: int **z = sus(x, y);
  //CHECK_ALL: _Array_ptr<_Ptr<int>> z = sus(x, y);

  z += 2;
  return z;
}
