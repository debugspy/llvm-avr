//===-- AVRTargetMachine.h - Define TargetMachine for AVR -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the AVR specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRTARGETMACHINE_H__
#define __INCLUDE_AVRTARGETMACHINE_H__

#include "AVRFrameLowering.h"
#include "AVRISelLowering.h"
#include "AVRInstrInfo.h"
#include "AVRSelectionDAGInfo.h"
#include "AVRSubtarget.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm
{

class AVRTargetMachine : public LLVMTargetMachine
{
public:
  AVRTargetMachine(const Target &T, StringRef TT, StringRef CPU, StringRef FS,
                   const TargetOptions &Options, Reloc::Model RM,
                   CodeModel::Model CM, CodeGenOpt::Level OL);
public: // TargetMachine
  const AVRSubtarget *getSubtargetImpl() const;
  const AVRInstrInfo *getInstrInfo() const;
  const TargetFrameLowering *getFrameLowering() const;
  const AVRTargetLowering *getTargetLowering() const;
  const AVRSelectionDAGInfo *getSelectionDAGInfo() const;
  const DataLayout *getDataLayout() const;
  const AVRRegisterInfo *getRegisterInfo() const;
public:
  // Pass Pipeline Configuration.
  TargetPassConfig *createPassConfig(PassManagerBase &PM);
private:
  AVRSubtarget SubTarget;
  const DataLayout DL;
  AVRInstrInfo InstrInfo;
  AVRFrameLowering FrameLowering;
  AVRTargetLowering TLInfo;
  AVRSelectionDAGInfo TSInfo;
};

} // end namespace llvm

#endif //__INCLUDE_AVRTARGETMACHINE_H__
