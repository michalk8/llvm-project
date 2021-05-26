#include "llvm/Transforms/Utils/DropAA.h"
#include "llvm/Transforms/IPO/Attributor.h"

using namespace llvm;

PreservedAnalyses DropAAPass::run(Function &F,
                                  FunctionAnalysisManager &AM) {
  errs() << "Function name: " << F.getName() << "\n";
  return PreservedAnalyses::all();
}

