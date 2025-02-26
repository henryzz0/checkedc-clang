//===----------- ExprUtils.h: Utility functions for expressions ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the utility functions for expressions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_EXPRUTILS_H
#define LLVM_CLANG_EXPRUTILS_H

#include "clang/AST/Expr.h"
#include "clang/Sema/Sema.h"

namespace clang {

class ExprCreatorUtil {
public:
  // If Op is not a compound operator, CreateBinaryOperator returns a binary
  // operator LHS Op RHS. If Op is a compound operator @=, CreateBinaryOperator
  // returns a binary operator LHS @ RHS. LHS and RHS are cast to rvalues if
  // necessary.
  static BinaryOperator *CreateBinaryOperator(Sema &SemaRef,
                                              Expr *LHS, Expr *RHS,
                                              BinaryOperatorKind Op);

  // Create an unsigned integer literal.
  static IntegerLiteral *CreateUnsignedInt(Sema &SemaRef, unsigned Value);

  // Create an implicit cast expression.
  static ImplicitCastExpr *CreateImplicitCast(Sema &SemaRef, Expr *E,
                                              CastKind CK, QualType T);

  // Create a use of a VarDecl.
  static DeclRefExpr *CreateVarUse(Sema &SemaRef, VarDecl *V);

  // If e is an rvalue, EnsureRValue returns e. Otherwise, EnsureRValue
  // returns a cast of e to an rvalue, based on the type of e.
  static Expr *EnsureRValue(Sema &SemaRef, Expr *E);

  // Create an integer literal from I. I is interpreted as an unsigned
  // integer.
  static IntegerLiteral *CreateIntegerLiteral(ASTContext &Ctx,
                                              const llvm::APInt &I);

  // If Ty is a pointer type, CreateIntegerLiteral returns an integer literal
  // with a target-dependent bit width. If Ty is an integer type (char,
  // unsigned int, int, etc.), CreateIntegerLiteral returns an integer literal
  // with Ty type.  Otherwise, it returns nullptr.
  static IntegerLiteral *CreateIntegerLiteral(ASTContext &Ctx,
                                              int Value, QualType Ty);

  // Determine if the mathemtical value of I (an unsigned integer) fits within
  // the range of Ty, a signed integer type. APInt requires that bitsizes
  // match exactly, so if I does fit, return an APInt via Result with exactly
  // the bitsize of Ty.
  static bool Fits(ASTContext &Ctx, QualType Ty,
                   const llvm::APInt &I, llvm::APInt &Result);
};

} // end namespace clang
#endif
