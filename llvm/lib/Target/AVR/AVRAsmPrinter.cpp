//===-- AVRAsmPrinter.cpp - AVR LLVM assembly writer ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format AVR assembly language.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "avr-asm-printer"

#include "AVR.h"
#include "AVRMCInstLower.h"
#include "InstPrinter/AVRInstPrinter.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

namespace
{

class AVRAsmPrinter : public AsmPrinter
{
public:
  explicit AVRAsmPrinter(TargetMachine &TM, MCStreamer &Streamer) :
    AsmPrinter(TM, Streamer) {}

  const char *getPassName() const
  {
    return "AVR Assembly Printer";
  }

  void printOperand(const MachineInstr *MI, unsigned OpNo, raw_ostream &O,
                    const char *Modifier = 0);

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNum,
                       unsigned AsmVariant, const char *ExtraCode,
                       raw_ostream &O) LLVM_OVERRIDE;

  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNum,
                             unsigned AsmVariant, const char *ExtraCode,
                             raw_ostream &O) LLVM_OVERRIDE;

public: // AsmPrinter
  void EmitInstruction(const MachineInstr *MI);
};

} // end of anonymous namespace

void AVRAsmPrinter::printOperand(const MachineInstr *MI, unsigned OpNo,
                                 raw_ostream &O, const char *Modifier)
{
  const MachineOperand &MO = MI->getOperand(OpNo);

  switch (MO.getType())
  {
  case MachineOperand::MO_Register:
    O << AVRInstPrinter::getRegisterName(MO.getReg());
    break;
  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    break;
  case MachineOperand::MO_GlobalAddress:
    O << getSymbol(MO.getGlobal());
    break;
  case MachineOperand::MO_ExternalSymbol:
    O << *GetExternalSymbolSymbol(MO.getSymbolName());
    break;
  //:FIXME: readd this once needed
    /*
  case MachineOperand::MO_MachineBasicBlock:
    O << *MO.getMBB()->getSymbol();
    break;
    */
  default:
    llvm_unreachable("Not implemented yet!");
  }
}

bool AVRAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNum,
                                    unsigned AsmVariant, const char *ExtraCode,
                                    raw_ostream &O)
{
  // Default asm printer can only deal with some extra codes,
  // so try it first.
  bool Error = AsmPrinter::PrintAsmOperand(MI, OpNum, AsmVariant, ExtraCode, O);
  if (Error && ExtraCode && ExtraCode[0])
  {
    if (ExtraCode[1] != 0) return true; // Unknown modifier.

    if  (ExtraCode[0] >= 'A' && ExtraCode[0] <= 'Z')
    {
      const MachineOperand& RegOp = MI->getOperand(OpNum);

      assert(RegOp.isReg() && "Operand must be a register when you're"
                              "using 'A'..'Z' operand extracodes.");
      unsigned Reg = RegOp.getReg();

      unsigned ByteNumber = ExtraCode[0] -'A';

      unsigned OpFlags = MI->getOperand(OpNum-1).getImm();
      unsigned NumOpRegs = InlineAsm::getNumOperandRegisters(OpFlags);

      const TargetRegisterInfo *TRI = MF->getTarget().getRegisterInfo();

      unsigned BytesPerReg = TRI->getMinimalPhysRegClass(Reg)->getSize();

      assert(BytesPerReg <= 2 && "Only 8 and 16 bit regs are supported.");

      unsigned RegIdx = ByteNumber / BytesPerReg;
      assert(RegIdx < NumOpRegs && "Multibyte index out of range.");

      Reg = MI->getOperand(OpNum + RegIdx).getReg();

      if (BytesPerReg == 2)
      {
        Reg = TRI->getSubReg(Reg, ByteNumber % BytesPerReg ?
                                  AVR::sub_hi : AVR::sub_lo);
      }

      O << AVRInstPrinter::getRegisterName(Reg);
      return false;
    }
  }

  printOperand(MI, OpNum, O);

  return false;
}

bool AVRAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                          unsigned OpNum, unsigned AsmVariant,
                                          const char *ExtraCode, raw_ostream &O)
{
  if (ExtraCode && ExtraCode[0])
  {
    // TODO:
    llvm_unreachable("This branch is not implemented yet");
  }

  const MachineOperand &MO = MI->getOperand(OpNum);
  assert(MO.isReg() && "Unexpected inline asm memory operand");

  if (!MI->getOperand(OpNum).isReg())
  {
    return true;
  }

  // :FIXME: This fixme is related with another one in AVRInstPrinter, line 29:
  // this should be done somewhere else
  // check out the new feature about alternative reg names
  if (MI->getOperand(OpNum).getReg() == AVR::R31R30)
  {
    O << "Z";
  }
  else
  {
    assert(MI->getOperand(OpNum).getReg() == AVR::R29R28
           && "Wrong register class for memory operand.");
    O << "Y";
  }

  // If NumOpRegs == 2, then we assume it is product of a FrameIndex expansion
  // and the second operand is an Imm.
  // Though it is weird that imm is counted as register too.
  unsigned OpFlags = MI->getOperand(OpNum - 1).getImm();
  unsigned NumOpRegs = InlineAsm::getNumOperandRegisters(OpFlags);
  if (NumOpRegs == 2)
  {
    O << '+' << MI->getOperand(OpNum + 1).getImm();
  }

  return false;
}

//===----------------------------------------------------------------------===//
void AVRAsmPrinter::EmitInstruction(const MachineInstr *MI)
{
  AVRMCInstLower MCInstLowering(OutContext, *this);

  MCInst I;
  MCInstLowering.Lower(MI, I);
  OutStreamer.EmitInstruction(I, getSubtargetInfo());
}

//===----------------------------------------------------------------------===//
// Target Registry Stuff
//===----------------------------------------------------------------------===//

// Force static initialization.
extern "C" void LLVMInitializeAVRAsmPrinter()
{
  RegisterAsmPrinter<AVRAsmPrinter> X(TheAVRTarget);
}
