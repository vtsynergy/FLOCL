//===--- KernelNameRestrictionCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "KernelNameRestrictionCheck.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

namespace {
class KernelNameRestrictionPPCallbacks : public PPCallbacks {
public:
  explicit KernelNameRestrictionPPCallbacks(ClangTidyCheck &Check, SourceManager &SM)
      : Check(Check), SM(SM) {}
  
  void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok,
                          StringRef FileName, bool IsAngled,
                          CharSourceRange FileNameRange, const FileEntry *File,
                          StringRef SearchPath, StringRef RelativePath,
                          const Module *Imported,
                          SrcMgr::CharacteristicKind FileType) override;

  void EndOfMainFile() override;

private:
  struct IncludeDirective {
    SourceLocation Loc;    ///< '#' Location in the include directive
    std::string Filename;  ///< Filename as a string
  };
  std::vector<IncludeDirective> IncludeDirectives;

  ClangTidyCheck &Check;
  SourceManager &SM; 
};
} // namespace

void KernelNameRestrictionCheck::registerPPCallbacks(CompilerInstance &Compiler) {
  Compiler.getPreprocessor().addPPCallbacks(
      ::llvm::make_unique<KernelNameRestrictionPPCallbacks>(
          *this, Compiler.getSourceManager()));
}

void KernelNameRestrictionPPCallbacks::InclusionDirective(
    SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
    bool IsAngled, CharSourceRange FilenameRange, const FileEntry *File,
    StringRef SearchPath, StringRef RelativePath, const Module *Imported,
    SrcMgr::CharacteristicKind FileType) {
  // We recognize the first include as a special main module header and want
  // to leave it in the top position.
  IncludeDirective ID = {HashLoc, FileName};
  IncludeDirectives.push_back(std::move(ID));
}

void KernelNameRestrictionPPCallbacks::EndOfMainFile() {
  if (IncludeDirectives.empty())
    return;

  // Check included files for restricted names
  for (unsigned I = 0; I < IncludeDirectives.size(); ++I) {
    IncludeDirective &ID = IncludeDirectives[I];
    StringRef FileName = StringRef(ID.Filename);
    if (FileName.endswith_lower("/kernel.cl") || 
        FileName.endswith_lower("/verilog.cl") || 
        FileName.endswith_lower("/vhdl.cl")) {
      Check.diag(ID.Loc, "The imported kernel source file is named \"kernel.cl\","
          "\"Verilog.cl\", or \"VHDL.cl\", which could cause compile errors.");
    }
  }

  // Check main file for restricted names
  const FileEntry *Entry = SM.getFileEntryForID(SM.getMainFileID());
  StringRef FileName = Entry->getName();
  if (FileName.endswith_lower("/kernel.cl") || 
      FileName.endswith_lower("/verilog.cl") || 
      FileName.endswith_lower("/vhdl.cl")) {
    Check.diag(SM.getLocForStartOfFile(SM.getMainFileID()),
        "Naming your OpenCL kernel source file \"kernel.cl\", \"Verilog.cl\", or \"VHDL.cl\" could cause compilation errors.");
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
