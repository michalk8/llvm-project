#include <random>
#include "llvm/ADT/SetVector.h"
#include "llvm/Transforms/Utils/DropAA.h"

using namespace llvm;

static cl::opt<unsigned>
    RandomSeed("dropaa-seed", cl::Hidden, cl::desc("Random seed."), cl::init(0));
static cl::opt<float>
    FnKeep("dropaa-fn-keep", cl::Hidden, cl::desc("f"), cl::init(1.0));
static cl::opt<float>
    FnArgKeep("dropaa-fna-keep", cl::Hidden, cl::desc("f"), cl::init(1.0));
static cl::opt<float>
    FnRetKeep("dropaa-fnr-keep", cl::Hidden, cl::desc("f"), cl::init(1.0));
static cl::opt<float>
    CbKeep("dropaa-cb-keep", cl::Hidden, cl::desc("f"), cl::init(1.0));
static cl::opt<float>
    CbArgKeep("dropaa-cba-keep", cl::Hidden, cl::desc("f"), cl::init(1.0));
static cl::opt<float>
    CbRetKeep("dropaa-cbr-keep", cl::Hidden, cl::desc("f"), cl::init(1.0));


SmallMap::ValueTy SmallMap::operator[](IRPosition::Kind kind) const {
  switch (kind) {
  case IRPosition::Kind::IRP_FUNCTION:
    return Storage[0];
  case IRPosition::Kind::IRP_ARGUMENT:
    return Storage[1];
  case IRPosition::Kind::IRP_RETURNED:
    return Storage[2];
  case IRPosition::Kind::IRP_CALL_SITE:
    return Storage[3];
  case IRPosition::Kind::IRP_CALL_SITE_ARGUMENT:
    return Storage[4];
  case IRPosition::Kind::IRP_CALL_SITE_RETURNED:
    return Storage[5];
  default:
    llvm_unreachable("Invalid IR position kind!");
  }
}

bool llvm::removeAttributes(IRPosition &IRP,
                            const SmallMap &FracToKeep,
                            std::mt19937 &Mt,
                            std::uniform_real_distribution<SmallMap::ValueTy> &Rng) {
  using ValueTy = SmallMap::ValueTy;
  AttributeList AttrList, NewAttrList;
  ValueTy Threshold = FracToKeep[IRP.getPositionKind()];
  bool changed = false;

  auto *CB = dyn_cast<CallBase>(&IRP.getAnchorValue());
  if (CB)
    AttrList = CB->getAttributes();
  else
    AttrList = IRP.getAssociatedFunction()->getAttributes();
  NewAttrList = AttrList;

  LLVMContext &Ctx = IRP.getAnchorValue().getContext();
  // TODO: inefficient
  for (auto &AS: AttrList) {
    for (auto &A: AS) {
      ValueTy Random = Rng(Mt);
      // TODO: add debug logging
      if (Random > Threshold) {
        NewAttrList = NewAttrList.removeAttribute(Ctx, IRP.getAttrIdx(), A.getKindAsEnum());
        changed = true;
      }
    }
  }

  if (CB)
    CB->setAttributes(NewAttrList);
  else
    IRP.getAssociatedFunction()->setAttributes(NewAttrList);

  return changed;
}

PreservedAnalyses DropAAPass::run(Module &M, ModuleAnalysisManager &AM) {
  SmallMap FracToKeep =
      SmallMap(FnKeep.getValue(), FnArgKeep.getValue(), FnRetKeep.getValue(),
               CbKeep.getValue(), CbArgKeep.getValue(), CbRetKeep.getValue());

  std::mt19937 Mt(RandomSeed.getValue());
  std::uniform_real_distribution<SmallMap::ValueTy> Rng(0.0, 1.0);
  bool changed = false;
  SetVector<Function *> Functions;

  // TODO: set needed?
  for (Function &F: M) {
    if (F.isDeclaration())
      continue;
    Functions.insert(&F);
  }

  for (Function *F: Functions) {
    // function attributes
    IRPosition FPos = IRPosition::function(*F);
    changed |= removeAttributes(FPos, FracToKeep, Mt, Rng);

    // return type attributes
    Type *ReturnType = F->getReturnType();
    if (!ReturnType->isVoidTy()) {
      IRPosition RetPos = IRPosition::returned(*F);
      changed |= removeAttributes(RetPos, FracToKeep, Mt, Rng);
    }

    // argument attributes
    for (Argument &Arg : F->args()) {
      IRPosition ArgPos = IRPosition::argument(Arg);
      changed |= removeAttributes(ArgPos, FracToKeep, Mt, Rng);
    }

    // TODO: verify it's correct (based on Attributor.cpp)
    // handle callsites
    for (auto &BB : *F) {
      for (auto &Inst : BB) {
        if (isa<InvokeInst>(Inst) || isa<CallBrInst>(Inst) ||
            isa<CallInst>(Inst)) {
          auto &CB = cast<CallBase>(Inst);
          // callsite return attributes
          IRPosition CBRetPos = IRPosition::callsite_returned(CB);
          changed |= removeAttributes(CBRetPos, FracToKeep, Mt, Rng);

          // callsite argument attributes
          for (int I = 0, E = CB.getNumArgOperands(); I < E; ++I) {
            IRPosition CBArgPos = IRPosition::callsite_argument(CB, I);
            changed |= removeAttributes(CBArgPos, FracToKeep, Mt, Rng);
          }
        }
      }
    }

    if (changed)
      return PreservedAnalyses::none();
    return PreservedAnalyses::all();
  }
}
