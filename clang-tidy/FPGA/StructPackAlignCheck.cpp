//===--- StructPackAlignCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MAX_ALIGN_POWER_OF_TWO
#define MAX_ALIGN_POWER_OF_TWO 7
#endif

#include "StructPackAlignCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

void StructPackAlignCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(recordDecl(isStruct()).bind("struct"), this);
  //Finder->addMatcher(recordDecl().bind("struct"), this);
}

void StructPackAlignCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *Struct = Result.Nodes.getNodeAs<RecordDecl>("struct");
  const auto *StructDef = Struct->getDefinition();
  if (Struct != StructDef) return; //Do nothing, we only want to deal with definitions
  std::vector<std::pair<unsigned int, unsigned int>> fieldSizes;
  unsigned int totalBitSize = 0;
  for (RecordDecl::field_iterator fields = Struct->field_begin(); fields != Struct->field_end(); fields++) {
    //For each field, record how big it is (in bits)
    // Would be good to use a pair of <offset, size> to advise a better packing order
    //fieldSizes.push_back(std::pair<unsigned int, unsigned int>((*fields)->getBitWidthValue(*Result.Context), (*fields)->getFieldIndex())i);
    unsigned int fieldWidth = (unsigned int)Result.Context->getTypeInfo((*fields)->getType().getTypePtr()).Width;
    fieldSizes.push_back(std::pair<unsigned int, unsigned int>(fieldWidth, (*fields)->getFieldIndex()));
    //TODO: Recommend a reorganization of the struct (sort by field size, largesst to smallest)
//	llvm::errs() << "fieldWidth: " << fieldWidth << "\n";
    totalBitSize += fieldWidth;
  }
  //unsigned int minByteSize = (totalBitSize + 7) >> 3; //TODO: Express this as CharUnit rather than a hardcoded 8-bits (Rshift3)i
  //const auto Layout = Result.Context->getASTRecordLayout(Struct);
  //After computing the minimum size in bits, check for an existing alignment flag
  //CharUnits currAlign = Layout.getAlignment();
  CharUnits currSize = Result.Context->getASTRecordLayout(Struct).getSize();
  CharUnits minByteSize = CharUnits::fromQuantity((totalBitSize +7) >> 3);
  CharUnits currAlign = Result.Context->getASTRecordLayout(Struct).getAlignment();
  CharUnits newAlign = CharUnits::fromQuantity(1);
  if (!minByteSize.isPowerOfTwo()) {
    int msb = (int)minByteSize.getQuantity();
    for (; msb > 0; msb>>=1) {
//	llvm::errs() << "msb: " << msb << "\n";
      newAlign = newAlign.alignTo(CharUnits::fromQuantity(((int)newAlign.getQuantity()) << 1));
      //Abort if the computed alignment meets the maximum configured alignment
      if (newAlign.getQuantity() >= (1 << MAX_ALIGN_POWER_OF_TWO)) break; 
   }
  } else {
    newAlign = minByteSize;
  }
  //if (!minByteSize.isPowerOfTwo()) {
  //  newAlign = minByteSize.alignTo(CharUnits::fromQuantity(((minByteSize.getQuantity()) & (-(minByteSize.getQuantity()))) * 2));
  //}
 //   diag(Struct->getLocation(), "struct %0 has size %1, alignment %2, and will be aligned to %3, with minimum pack size %4")
//	<< Struct
//	<< (int)currSize.getQuantity()
//	<< (int)currAlign.getQuantity()
//	<< (int)newAlign.getQuantity()
//	<< (int)minByteSize.getQuantity();
  //If it's using much more space than it needs, suggest packing.
  // (Do not suggest packing if it is currently explicitly aligned to what the minimum byte size would suggest as the new alignment
  if (minByteSize < currSize && ((Struct->getMaxAlignment()>>3) != newAlign.getQuantity())) {
    diag(Struct->getLocation(), "struct %0 has inefficient access due to padding, only needs %1 bytes but is using %2 bytes, use \"__attribute((packed))\"")
	<< Struct
	<< (int)minByteSize.getQuantity()
	<< (int)currSize.getQuantity();
  }
  // and suggest the minimum power-of-two alignment for the struct as a whole (with and without packing>
  if (currAlign.getQuantity() != newAlign.getQuantity()) {
    diag(Struct->getLocation(), "struct %0 has inefficient access due to poor alignment. Currently aligned to %1 bytes, but size %3 bytes is large enough to benefit from \"__attribute((aligned(%2)))\"")
	<< Struct
	<< (int)currAlign.getQuantity()
	<< (int)newAlign.getQuantity()
	<< (int)minByteSize.getQuantity(); 
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
