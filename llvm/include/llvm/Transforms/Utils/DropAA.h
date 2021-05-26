#ifndef LLVM_TRANSFORMS_DROPAA_H
#define LLVM_TRANSFORMS_DROPAA_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class DropAAPass: public PassInfoMixin<DropAAPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_DROPAA_H
