//===--- StructPackAlignCheck.cpp - clang-tidy ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "StructPackAlignCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

constexpr int MAX_CONFIGURED_ALIGNMENT = 128; // 1 << 7;

void StructPackAlignCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(recordDecl(isStruct(), isDefinition()).bind("struct"),
                     this);
}

void StructPackAlignCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Struct = Result.Nodes.getNodeAs<RecordDecl>("struct");

  // Get sizing info for the struct
  std::vector<std::pair<unsigned int, unsigned int>> FieldSizes;
  unsigned int TotalBitSize = 0;
  for (const FieldDecl *StructField : Struct->fields()) {
    // For each StructField, record how big it is (in bits)
    // Would be good to use a pair of <offset, size> to advise a better
    // packing order
    unsigned int StructFieldWidth =
        (unsigned int)Result.Context
            ->getTypeInfo(StructField->getType().getTypePtr())
            .Width;
    FieldSizes.emplace_back(StructFieldWidth, StructField->getFieldIndex());
    // FIXME: Recommend a reorganization of the struct (sort by StructField
    // size, largest to smallest)
    TotalBitSize += StructFieldWidth;
  }
  // FIXME: Express this as CharUnit rather than a hardcoded 8-bits (Rshift3)i
  // After computing the minimum size in bits, check for an existing alignment
  // flag
  CharUnits CurrSize = Result.Context->getASTRecordLayout(Struct).getSize();
  CharUnits MinByteSize = CharUnits::fromQuantity((TotalBitSize + 7) / 8);
  CharUnits CurrAlign =
      Result.Context->getASTRecordLayout(Struct).getAlignment();
  CharUnits NewAlign = CharUnits::fromQuantity(1);
  if (!MinByteSize.isPowerOfTwo()) {
    int MSB = (int)MinByteSize.getQuantity();
    for (; MSB > 0; MSB /= 2) {
      NewAlign = NewAlign.alignTo(
          CharUnits::fromQuantity(((int)NewAlign.getQuantity()) * 2));
      // Abort if the computed alignment meets the maximum configured alignment
      if (NewAlign.getQuantity() >= MAX_CONFIGURED_ALIGNMENT)
        break;
    }
  } else {
    NewAlign = MinByteSize;
  }

  // Check if struct has a "packed" attribute
  bool IsPacked = false;
  if (Struct->hasAttrs()) {
    for (const Attr *StructAttribute : Struct->getAttrs()) {
      if (StructAttribute->getKind() == attr::Packed) {
        IsPacked = true;
        break;
      }
    }
  }

  // If it's using much more space than it needs, suggest packing.
  // (Do not suggest packing if it is currently explicitly aligned to what the
  // minimum byte size would suggest as the new alignment)
  if (MinByteSize < CurrSize &&
      ((Struct->getMaxAlignment() / 8) != NewAlign.getQuantity()) &&
      (CurrSize != NewAlign) && !IsPacked) {
    diag(Struct->getLocation(),
         "struct %0 has inefficient access due to padding, only needs %1 bytes "
         "but is using %2 bytes, use \"__attribute((packed))\"")
        << Struct << (int)MinByteSize.getQuantity()
        << (int)CurrSize.getQuantity();
  }

  // And suggest the minimum power-of-two alignment for the struct as a whole
  // (with and without packing)
  if (CurrAlign.getQuantity() != NewAlign.getQuantity()) {
    diag(Struct->getLocation(),
         "struct %0 has inefficient access due to poor alignment; currently "
         "aligned to %1 bytes, but size %3 bytes is large enough to benefit "
         "from \"__attribute((aligned(%2)))\"")
        << Struct << (int)CurrAlign.getQuantity() << (int)NewAlign.getQuantity()
        << (int)MinByteSize.getQuantity();
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
