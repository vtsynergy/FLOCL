//===--- DurationRewriter.cpp - clang-tidy --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DurationRewriter.h"
#include "clang/Tooling/FixIt.h"
#include "llvm/ADT/IndexedMap.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace abseil {

struct DurationScale2IndexFunctor {
  using argument_type = DurationScale;
  unsigned operator()(DurationScale Scale) const {
    return static_cast<unsigned>(Scale);
  }
};

/// Returns an integer if the fractional part of a `FloatingLiteral` is `0`.
static llvm::Optional<llvm::APSInt>
truncateIfIntegral(const FloatingLiteral &FloatLiteral) {
  double Value = FloatLiteral.getValueAsApproximateDouble();
  if (std::fmod(Value, 1) == 0) {
    if (Value >= static_cast<double>(1u << 31))
      return llvm::None;

    return llvm::APSInt::get(static_cast<int64_t>(Value));
  }
  return llvm::None;
}

const std::pair<llvm::StringRef, llvm::StringRef> &
getDurationInverseForScale(DurationScale Scale) {
  static const llvm::IndexedMap<std::pair<llvm::StringRef, llvm::StringRef>,
                                DurationScale2IndexFunctor>
      InverseMap = []() {
        // TODO: Revisit the immediately invoked lamba technique when
        // IndexedMap gets an initializer list constructor.
        llvm::IndexedMap<std::pair<llvm::StringRef, llvm::StringRef>,
                         DurationScale2IndexFunctor>
            InverseMap;
        InverseMap.resize(6);
        InverseMap[DurationScale::Hours] =
            std::make_pair("::absl::ToDoubleHours", "::absl::ToInt64Hours");
        InverseMap[DurationScale::Minutes] =
            std::make_pair("::absl::ToDoubleMinutes", "::absl::ToInt64Minutes");
        InverseMap[DurationScale::Seconds] =
            std::make_pair("::absl::ToDoubleSeconds", "::absl::ToInt64Seconds");
        InverseMap[DurationScale::Milliseconds] = std::make_pair(
            "::absl::ToDoubleMilliseconds", "::absl::ToInt64Milliseconds");
        InverseMap[DurationScale::Microseconds] = std::make_pair(
            "::absl::ToDoubleMicroseconds", "::absl::ToInt64Microseconds");
        InverseMap[DurationScale::Nanoseconds] = std::make_pair(
            "::absl::ToDoubleNanoseconds", "::absl::ToInt64Nanoseconds");
        return InverseMap;
      }();

  return InverseMap[Scale];
}

/// If `Node` is a call to the inverse of `Scale`, return that inverse's
/// argument, otherwise None.
static llvm::Optional<std::string>
rewriteInverseDurationCall(const MatchFinder::MatchResult &Result,
                           DurationScale Scale, const Expr &Node) {
  const std::pair<llvm::StringRef, llvm::StringRef> &InverseFunctions =
      getDurationInverseForScale(Scale);
  if (const auto *MaybeCallArg = selectFirst<const Expr>(
          "e",
          match(callExpr(callee(functionDecl(hasAnyName(
                             InverseFunctions.first, InverseFunctions.second))),
                         hasArgument(0, expr().bind("e"))),
                Node, *Result.Context))) {
    return tooling::fixit::getText(*MaybeCallArg, *Result.Context).str();
  }

  return llvm::None;
}

/// Returns the factory function name for a given `Scale`.
llvm::StringRef getDurationFactoryForScale(DurationScale Scale) {
  switch (Scale) {
  case DurationScale::Hours:
    return "absl::Hours";
  case DurationScale::Minutes:
    return "absl::Minutes";
  case DurationScale::Seconds:
    return "absl::Seconds";
  case DurationScale::Milliseconds:
    return "absl::Milliseconds";
  case DurationScale::Microseconds:
    return "absl::Microseconds";
  case DurationScale::Nanoseconds:
    return "absl::Nanoseconds";
  }
  llvm_unreachable("unknown scaling factor");
}

/// Returns the Time factory function name for a given `Scale`.
llvm::StringRef getTimeInverseForScale(DurationScale scale) {
  switch (scale) {
  case DurationScale::Hours:
    return "absl::ToUnixHours";
  case DurationScale::Minutes:
    return "absl::ToUnixMinutes";
  case DurationScale::Seconds:
    return "absl::ToUnixSeconds";
  case DurationScale::Milliseconds:
    return "absl::ToUnixMillis";
  case DurationScale::Microseconds:
    return "absl::ToUnixMicros";
  case DurationScale::Nanoseconds:
    return "absl::ToUnixNanos";
  }
  llvm_unreachable("unknown scaling factor");
}

#if (LLVM_PACKAGE_VERSION >= 900)
#else
extern const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt, InitListExpr> initListExpr;

AST_MATCHER_P(InitListExpr, hasSyntacticForm, ast_matchers::internal::Matcher<Expr>, InnerMatcher) {
	const Expr *SyntForm = Node.getSyntacticForm();
	return (SyntForm != nullptr && InnerMatcher.matches(*SyntForm, Finder, Builder));
}

AST_MATCHER_P2(InitListExpr, hasInit, unsigned, N, ast_matchers::internal::Matcher<Expr>, InnerMatcher) {
	return N < Node.getNumInits() && InnerMatcher.matches(*Node.getInit(N), Finder, Builder);
}
#endif


/// Returns `true` if `Node` is a value which evaluates to a literal `0`.
bool IsLiteralZero(const MatchFinder::MatchResult &Result, const Expr &Node) {
  auto ZeroMatcher =
      anyOf(integerLiteral(equals(0)), floatLiteral(equals(0.0)));

  // Check to see if we're using a zero directly.
  if (selectFirst<const clang::Expr>(
          "val", match(expr(ignoringImpCasts(ZeroMatcher)).bind("val"), Node,
                       *Result.Context)) != nullptr)
    return true;

  // Now check to see if we're using a functional cast with a scalar
  // initializer expression, e.g. `int{0}`.
  if (selectFirst<const clang::Expr>(
          "val", match(cxxFunctionalCastExpr(
                           hasDestinationType(
                               anyOf(isInteger(), realFloatingPointType())),
                           hasSourceExpression(initListExpr(
                               hasInit(0, ignoringParenImpCasts(ZeroMatcher)))))
                           .bind("val"),
                       Node, *Result.Context)) != nullptr)
    return true;

  return false;
}

llvm::Optional<std::string>
stripFloatCast(const ast_matchers::MatchFinder::MatchResult &Result,
               const Expr &Node) {
  if (const Expr *MaybeCastArg = selectFirst<const Expr>(
          "cast_arg",
          match(expr(anyOf(cxxStaticCastExpr(
                               hasDestinationType(realFloatingPointType()),
                               hasSourceExpression(expr().bind("cast_arg"))),
                           cStyleCastExpr(
                               hasDestinationType(realFloatingPointType()),
                               hasSourceExpression(expr().bind("cast_arg"))),
                           cxxFunctionalCastExpr(
                               hasDestinationType(realFloatingPointType()),
                               hasSourceExpression(expr().bind("cast_arg"))))),
                Node, *Result.Context)))
    return tooling::fixit::getText(*MaybeCastArg, *Result.Context).str();

  return llvm::None;
}

llvm::Optional<std::string>
stripFloatLiteralFraction(const MatchFinder::MatchResult &Result,
                          const Expr &Node) {
  if (const auto *LitFloat = llvm::dyn_cast<FloatingLiteral>(&Node))
    // Attempt to simplify a `Duration` factory call with a literal argument.
    if (llvm::Optional<llvm::APSInt> IntValue = truncateIfIntegral(*LitFloat))
      return IntValue->toString(/*radix=*/10);

  return llvm::None;
}

std::string simplifyDurationFactoryArg(const MatchFinder::MatchResult &Result,
                                       const Expr &Node) {
  // Check for an explicit cast to `float` or `double`.
  if (llvm::Optional<std::string> MaybeArg = stripFloatCast(Result, Node))
    return *MaybeArg;

  // Check for floats without fractional components.
  if (llvm::Optional<std::string> MaybeArg =
          stripFloatLiteralFraction(Result, Node))
    return *MaybeArg;

  // We couldn't simplify any further, so return the argument text.
  return tooling::fixit::getText(Node, *Result.Context).str();
}

llvm::Optional<DurationScale> getScaleForDurationInverse(llvm::StringRef Name) {
  static const llvm::StringMap<DurationScale> ScaleMap(
      {{"ToDoubleHours", DurationScale::Hours},
       {"ToInt64Hours", DurationScale::Hours},
       {"ToDoubleMinutes", DurationScale::Minutes},
       {"ToInt64Minutes", DurationScale::Minutes},
       {"ToDoubleSeconds", DurationScale::Seconds},
       {"ToInt64Seconds", DurationScale::Seconds},
       {"ToDoubleMilliseconds", DurationScale::Milliseconds},
       {"ToInt64Milliseconds", DurationScale::Milliseconds},
       {"ToDoubleMicroseconds", DurationScale::Microseconds},
       {"ToInt64Microseconds", DurationScale::Microseconds},
       {"ToDoubleNanoseconds", DurationScale::Nanoseconds},
       {"ToInt64Nanoseconds", DurationScale::Nanoseconds}});

  auto ScaleIter = ScaleMap.find(std::string(Name));
  if (ScaleIter == ScaleMap.end())
    return llvm::None;

  return ScaleIter->second;
}

llvm::Optional<DurationScale> getScaleForTimeInverse(llvm::StringRef Name) {
  static const llvm::StringMap<DurationScale> ScaleMap(
      {{"ToUnixHours", DurationScale::Hours},
       {"ToUnixMinutes", DurationScale::Minutes},
       {"ToUnixSeconds", DurationScale::Seconds},
       {"ToUnixMillis", DurationScale::Milliseconds},
       {"ToUnixMicros", DurationScale::Microseconds},
       {"ToUnixNanos", DurationScale::Nanoseconds}});

  auto ScaleIter = ScaleMap.find(std::string(Name));
  if (ScaleIter == ScaleMap.end())
    return llvm::None;

  return ScaleIter->second;
}

std::string rewriteExprFromNumberToDuration(
    const ast_matchers::MatchFinder::MatchResult &Result, DurationScale Scale,
    const Expr *Node) {
  const Expr &RootNode = *Node->IgnoreParenImpCasts();

  // First check to see if we can undo a complimentary function call.
  if (llvm::Optional<std::string> MaybeRewrite =
          rewriteInverseDurationCall(Result, Scale, RootNode))
    return *MaybeRewrite;

  if (IsLiteralZero(Result, RootNode))
    return std::string("absl::ZeroDuration()");

  return (llvm::Twine(getDurationFactoryForScale(Scale)) + "(" +
          simplifyDurationFactoryArg(Result, RootNode) + ")")
      .str();
}

bool isNotInMacro(const MatchFinder::MatchResult &Result, const Expr *E) {
#if (LLVM_PACKAGE_VERSION >= 900)
  if (!E->getBeginLoc().isMacroID())
#else
  if (!E->getLocStart().isMacroID())
#endif
    return true;

#if (LLVM_PACKAGE_VERSION >= 900)
  SourceLocation Loc = E->getBeginLoc();
#else
  SourceLocation Loc = E->getLocStart();
#endif
  // We want to get closer towards the initial macro typed into the source only
  // if the location is being expanded as a macro argument.
  while (Result.SourceManager->isMacroArgExpansion(Loc)) {
    // We are calling getImmediateMacroCallerLoc, but note it is essentially
    // equivalent to calling getImmediateSpellingLoc in this context according
    // to Clang implementation. We are not calling getImmediateSpellingLoc
    // because Clang comment says it "should not generally be used by clients."
    Loc = Result.SourceManager->getImmediateMacroCallerLoc(Loc);
  }
  return !Loc.isMacroID();
}

} // namespace abseil
} // namespace tidy
} // namespace clang
