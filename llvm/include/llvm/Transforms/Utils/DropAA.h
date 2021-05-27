#ifndef LLVM_TRANSFORMS_DROPAA_H
#define LLVM_TRANSFORMS_DROPAA_H

#include <random>
#include <string>
#include <unordered_map>
#include "llvm/Transforms/IPO/Attributor.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

class DropAAPass: public PassInfoMixin<DropAAPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

class SmallMap {
public:
  using StorageTy = SmallVector<float, 6>;
  using ValueTy = StorageTy::value_type;

  SmallMap(ValueTy Fn, ValueTy FnArg, ValueTy FnRet,
           ValueTy Cb, ValueTy CbArg, ValueTy CbRet) {
    Storage.resize(6);
    Storage[0] = clamp(Fn); Storage[1] = clamp(FnArg); Storage[2] = clamp(FnRet);
    Storage[3] = clamp(Cb); Storage[4] = clamp(CbArg); Storage[5] = clamp(CbRet);
  }

  ValueTy operator[](IRPosition::Kind) const;
  static ValueTy clamp(ValueTy Val) {
    return std::max(ValueTy(0), std::min(Val, ValueTy(1)));
  };
private:
  StorageTy Storage;
};

bool removeAttributes(IRPosition&,
                      const SmallMap&,
                      std::mt19937 &,
                      std::uniform_real_distribution<SmallMap::ValueTy>&);

} // namespace llvm

#endif // LLVM_TRANSFORMS_DROPAA_H
