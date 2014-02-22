//===-- AVRTargetMachine.cpp - Define TargetMachine for AVR ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the AVR specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#include "AVRTargetMachine.h"
#include "AVR.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

AVRTargetMachine::AVRTargetMachine(const Target &T, StringRef TT, StringRef CPU,
                                   StringRef FS, const TargetOptions &Options,
                                   Reloc::Model RM, CodeModel::Model CM,
                                   CodeGenOpt::Level OL) :
  LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
  SubTarget(TT, CPU, FS),
  DL("e-p:16:8:8-i8:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-n8"),
  InstrInfo(),
  FrameLowering(),
  TLInfo(*this),
  TSInfo(*this)
{
  initAsmInfo();
}

namespace
{
/// AVR Code Generator Pass Configuration Options.
class AVRPassConfig : public TargetPassConfig
{
public:
  AVRPassConfig(AVRTargetMachine *TM, PassManagerBase &PM) :
    TargetPassConfig(TM, PM) {}

  AVRTargetMachine &getAVRTargetMachine() const
  {
    return getTM<AVRTargetMachine>();
  }

  bool addInstSelector();
  bool addPreSched2();
  bool addPreRegAlloc();
  bool addPreEmitPass();
  FunctionPass *createTargetRegisterAllocator(bool Optimized);
};
} // namespace

TargetPassConfig *AVRTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AVRPassConfig(this, PM);
}

extern "C" void LLVMInitializeAVRTarget() {
  // Register the target.
  RegisterTargetMachine<AVRTargetMachine> X(TheAVRTarget);
}

const AVRSubtarget *AVRTargetMachine::getSubtargetImpl() const
{
  return &SubTarget;
}

const AVRInstrInfo *AVRTargetMachine::getInstrInfo() const
{
  return &InstrInfo;
}

const TargetFrameLowering *AVRTargetMachine::getFrameLowering() const
{
  return &FrameLowering;
}

const AVRTargetLowering *AVRTargetMachine::getTargetLowering() const
{
  return &TLInfo;
}

const AVRSelectionDAGInfo *AVRTargetMachine::getSelectionDAGInfo() const
{
  return &TSInfo;
}

const DataLayout *AVRTargetMachine::getDataLayout() const
{
  return &DL;
}

const AVRRegisterInfo *AVRTargetMachine::getRegisterInfo() const
{
  return &InstrInfo.getRegisterInfo();
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

bool AVRPassConfig::addInstSelector()
{
  // Install an instruction selector.
  addPass(createAVRISelDag(getAVRTargetMachine(), getOptLevel()));
  // Create the frame analyzer pass used by the PEI pass.
  addPass(createAVRFrameAnalyzerPass());

  return false;
}

bool AVRPassConfig::addPreRegAlloc()
{
  // Create the dynalloc SP save/restore pass to handle variable sized allocas.
  addPass(createAVRDynAllocaSRPass());

  return false;
}

bool AVRPassConfig::addPreSched2()
{
  addPass(createAVRExpandPseudoPass());

  return true;
}

bool AVRPassConfig::addPreEmitPass()
{
  // Must run branch selection immediately preceding the asm printer.
  addPass(createAVRBranchSelectionPass());

  return false;
}

FunctionPass *AVRPassConfig::createTargetRegisterAllocator(bool Optimized)
{
  // Unconditionally use our custom greedy register allocator.
  return createGreedyRegisterAllocator();
}
