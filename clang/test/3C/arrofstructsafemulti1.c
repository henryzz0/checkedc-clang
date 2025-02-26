// RUN: rm -rf %t*
// RUN: 3c -base-dir=%S -addcr -alltypes -output-dir=%t.checkedALL %s %S/arrofstructsafemulti2.c --
// RUN: 3c -base-dir=%S -addcr -output-dir=%t.checkedNOALL %s %S/arrofstructsafemulti2.c --
// RUN: %clang -working-directory=%t.checkedNOALL -c arrofstructsafemulti1.c arrofstructsafemulti2.c
// RUN: FileCheck -match-full-lines -check-prefixes="CHECK_NOALL","CHECK" --input-file %t.checkedNOALL/arrofstructsafemulti1.c %s
// RUN: FileCheck -match-full-lines -check-prefixes="CHECK_ALL","CHECK" --input-file %t.checkedALL/arrofstructsafemulti1.c %s
// RUN: 3c -base-dir=%S -alltypes -output-dir=%t.checked %S/arrofstructsafemulti2.c %s --
// RUN: 3c -base-dir=%t.checked -alltypes -output-dir=%t.convert_again %t.checked/arrofstructsafemulti1.c %t.checked/arrofstructsafemulti2.c --
// RUN: test ! -f %t.convert_again/arrofstructsafemulti1.c
// RUN: test ! -f %t.convert_again/arrofstructsafemulti2.c

/******************************************************************************/

/*This file tests three functions: two callers bar and foo, and a callee sus*/
/*In particular, this file tests: how the tool behaves when there is an array
  of structs*/
/*For robustness, this test is identical to
arrofstructprotosafe.c and arrofstructsafe.c except in that
the callee and callers are split amongst two files to see how
the tool performs conversions*/
/*In this test, foo, bar, and sus will all treat their return values safely*/

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

struct general **sus(struct general *, struct general *);
//CHECK_NOALL: struct general **sus(struct general *x : itype(_Ptr<struct general>), struct general *y : itype(_Ptr<struct general>)) : itype(_Ptr<struct general *>);
//CHECK_ALL: _Array_ptr<_Ptr<struct general>> sus(struct general *x : itype(_Ptr<struct general>), _Ptr<struct general> y) : count(5);

struct general **foo() {
  //CHECK_NOALL: _Ptr<struct general *> foo(void) {
  //CHECK_ALL: _Array_ptr<_Ptr<struct general>> foo(void) : count(5) {
  struct general *x = malloc(sizeof(struct general));
  //CHECK: _Ptr<struct general> x = malloc<struct general>(sizeof(struct general));
  struct general *y = malloc(sizeof(struct general));
  //CHECK: _Ptr<struct general> y = malloc<struct general>(sizeof(struct general));

  struct general *curr = y;
  //CHECK: _Ptr<struct general> curr = y;
  int i;
  for (i = 1; i < 5; i++, curr = curr->next) {
    curr->data = i;
    curr->next = malloc(sizeof(struct general));
    curr->next->data = i + 1;
  }
  struct general **z = sus(x, y);
  //CHECK_NOALL: _Ptr<struct general *> z = sus(x, y);
  //CHECK_ALL: _Array_ptr<_Ptr<struct general>> z : count(5) = sus(x, y);
  return z;
}

struct general **bar() {
  //CHECK_NOALL: _Ptr<struct general *> bar(void) {
  //CHECK_ALL: _Array_ptr<_Ptr<struct general>> bar(void) : count(5) {
  struct general *x = malloc(sizeof(struct general));
  //CHECK: _Ptr<struct general> x = malloc<struct general>(sizeof(struct general));
  struct general *y = malloc(sizeof(struct general));
  //CHECK: _Ptr<struct general> y = malloc<struct general>(sizeof(struct general));

  struct general *curr = y;
  //CHECK: _Ptr<struct general> curr = y;
  int i;
  for (i = 1; i < 5; i++, curr = curr->next) {
    curr->data = i;
    curr->next = malloc(sizeof(struct general));
    curr->next->data = i + 1;
  }
  struct general **z = sus(x, y);
  //CHECK_NOALL: _Ptr<struct general *> z = sus(x, y);
  //CHECK_ALL: _Array_ptr<_Ptr<struct general>> z : count(5) = sus(x, y);
  return z;
}
