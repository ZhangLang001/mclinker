//===- ARMLDBackend.cpp ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <llvm/ADT/Triple.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/ELF.h>
#include <llvm/Support/ErrorHandling.h>

#include <mcld/LD/SectionMap.h>
#include <mcld/MC/MCLDInfo.h>
#include <mcld/MC/MCLDOutput.h>
#include <mcld/MC/MCLinker.h>
#include <mcld/MC/MCRegionFragment.h>
#include <mcld/Support/MemoryRegion.h>
#include <mcld/Support/TargetRegistry.h>

#include "ARM.h"
#include "ARMLDBackend.h"
#include "ARMRelocationFactory.h"

using namespace mcld;

ARMGNULDBackend::ARMGNULDBackend()
  : m_pRelocFactory(0),
    m_pGOT(0),
    m_pPLT(0),
    m_pRelDyn(0),
    m_pRelPLT(0),
    m_pEXIDX(0),
    m_pEXTAB(0),
    m_pAttributes(0) {
}

ARMGNULDBackend::~ARMGNULDBackend()
{
  if (m_pRelocFactory)
    delete m_pRelocFactory;
  if(m_pGOT)
    delete m_pGOT;
  if(m_pPLT)
    delete m_pPLT;
  if(m_pRelDyn)
    delete m_pRelDyn;
  if(m_pRelPLT)
    delete m_pRelPLT;

}

bool ARMGNULDBackend::initRelocFactory(const MCLinker& pLinker)
{
  if (NULL == m_pRelocFactory) {
    m_pRelocFactory = new ARMRelocationFactory(1024, *this);
    m_pRelocFactory->setLayout(pLinker.getLayout());
  }
  return true;
}

RelocationFactory* ARMGNULDBackend::getRelocFactory()
{
  assert(m_pRelocFactory!= NULL);
  return m_pRelocFactory;
}

uint32_t ARMGNULDBackend::machine() const
{
  return ELF::EM_ARM;
}

bool ARMGNULDBackend::isLittleEndian() const
{
  return true;
}

bool ARMGNULDBackend::initTargetSectionMap(SectionMap& pSectionMap)
{
  if (!pSectionMap.push_back(".ARM.exidx", ".ARM.exidx") ||
      !pSectionMap.push_back(".ARM.extab", ".ARM.extab") ||
      !pSectionMap.push_back(".ARM.attributes", ".ARM.attributes"))
    return false;
  return true;
}

void ARMGNULDBackend::initTargetSections(MCLinker& pLinker)
{
  m_pEXIDX        = &pLinker.getOrCreateOutputSectHdr(".ARM.exidx",
                                                      LDFileFormat::Target,
                                                      ELF::SHT_ARM_EXIDX,
                                                      ELF::SHF_ALLOC | ELF::SHF_LINK_ORDER);
  m_pEXTAB        = &pLinker.getOrCreateOutputSectHdr(".ARM.extab",
                                                      LDFileFormat::Target,
                                                      ELF::SHT_PROGBITS,
                                                      ELF::SHF_ALLOC);
  m_pAttributes   = &pLinker.getOrCreateOutputSectHdr(".ARM.attributes",
                                                      LDFileFormat::Target,
                                                      ELF::SHT_ARM_ATTRIBUTES,
                                                      0);
}

void ARMGNULDBackend::initTargetSymbols(MCLinker& pLinker)
{
}

void ARMGNULDBackend::doPreLayout(const Output& pOutput,
                                  const MCLDInfo& pInfo,
                                  MCLinker& pLinker)
{
  // when building shared object, the .got section is needed
  if(pOutput.type() == Output::DynObj && (NULL == m_pGOT)) {
      createARMGOT(pLinker, pOutput.type());
  }
}

void ARMGNULDBackend::doPostLayout(const Output& pOutput,
                                   const MCLDInfo& pInfo,
                                   MCLinker& pLinker)
{
  // emit program headers
  if(pOutput.type() == Output::DynObj || pOutput.type() == Output::Exec)
    emitProgramHdrs(pOutput);
}

void ARMGNULDBackend::createARMGOT(MCLinker& pLinker, unsigned int pType)
{
  // get .got LDSection and create MCSectionData
  if( pType == Output::DynObj ) {
    LDSection& got = getDynObjFileFormat()->getGOT();
    m_pGOT = new ARMGOT(got, pLinker.getOrCreateSectData(got));
  }
  else if( pType == Output::Exec) {
    LDSection& got = getExecFileFormat()->getGOT();
    m_pGOT = new ARMGOT(got, pLinker.getOrCreateSectData(got));
  }
  else
    llvm::report_fatal_error(llvm::Twine("GOT is not support in ") +
                             ("output file type ") +
                             llvm::Twine(pType));

  // define symbol _GLOBAL_OFFSET_TABLE_ when .got create
  pLinker.defineSymbol<MCLinker::Force>("_GLOBAL_OFFSET_TABLE_",
                                        false,
                                        ResolveInfo::Object,
                                        ResolveInfo::Define,
                                        ResolveInfo::Local,
                                        0, // size
                                        0, // value
                                        pLinker.getLayout().getFragmentRef(*(m_pGOT->begin()), 0),
                                        ResolveInfo::Hidden);
}

void ARMGNULDBackend::createARMPLTandRelPLT(MCLinker& pLinker,
                                            unsigned int pType)
{
  // Create .got section if it dosen't exist
  if(!m_pGOT)
    createARMGOT(pLinker, pType);

  // get .plt and .rel.plt LDSection
  if( pType == Output::DynObj ) {
    LDSection& plt = getDynObjFileFormat()->getPLT();
    LDSection& relplt = getDynObjFileFormat()->getRelPlt();
    // create MCSectionData and ARMPLT
    m_pPLT = new ARMPLT(plt, pLinker.getOrCreateSectData(plt), *m_pGOT);
    // set info of .rel.plt to .plt
    relplt.setLinkInfo(&plt);
    // create MCSectionData and ARMRelDynSection
    m_pRelPLT = new ARMDynRelSection(relplt,
                                     pLinker.getOrCreateSectData(relplt),
                                     8);
  }
  else if( pType == Output::Exec ) {
    LDSection& plt = getExecFileFormat()->getPLT();
    LDSection& relplt = getExecFileFormat()->getRelPlt();
    // create MCSectionData and ARMPLT
    m_pPLT = new ARMPLT(plt, pLinker.getOrCreateSectData(plt), *m_pGOT);
    // set info of .rel.plt to .plt
    relplt.setLinkInfo(&plt);
    // create MCSectionData and ARMRelDynSection
    m_pRelPLT = new ARMDynRelSection(relplt,
                                     pLinker.getOrCreateSectData(relplt),
                                     8);
  }
  else
    llvm::report_fatal_error(llvm::Twine("PLT is not support in ") +
                             ("output file type ") +
                             llvm::Twine(pType));
}

void ARMGNULDBackend::createARMRelDyn(MCLinker& pLinker,
                                      unsigned int pType)
{
  // get .rel.dyn LDSection and create MCSectionData
  if( pType == Output::DynObj ) {
    LDSection& reldyn = getDynObjFileFormat()->getRelDyn();
    // create MCSectionData and ARMRelDynSection
    m_pRelDyn = new ARMDynRelSection(reldyn,
                                     pLinker.getOrCreateSectData(reldyn),
                                     8);
  }
  else if( pType == Output::Exec) {
    LDSection& reldyn = getExecFileFormat()->getRelDyn();
    // create MCSectionData and ARMRelDynSection
    m_pRelDyn = new ARMDynRelSection(reldyn,
                                     pLinker.getOrCreateSectData(reldyn),
                                     8);
  }
  else
    llvm::report_fatal_error(llvm::Twine("Dynamic Relocation ") +
                             ("is not support in output file type ") +
                             llvm::Twine(pType));
}

bool ARMGNULDBackend::isSymbolNeedsPLT(const ResolveInfo& pSym,
                                       unsigned int pType,
                                       const MCLDInfo& pLDInfo)
{
  return((Output::DynObj == pType)
         &&(ResolveInfo::Function == pSym.type())
         &&(pSym.isDyn() || pSym.isUndef() ||
            isSymbolPreemtible(pSym, pType, pLDInfo))
        );
}

bool ARMGNULDBackend::isSymbolNeedsDynRel(const ResolveInfo& pSym,
                                          unsigned int pType,
                                          bool isAbsReloc)
{
  if(pSym.isUndef() && (pType==Output::Exec))
    return false;
  if(pSym.isAbsolute())
    return false;
  if(pType==Output::DynObj && isAbsReloc)
    return true;
  if(pSym.isDyn() || pSym.isUndef())
    return true;

  return false;
}

bool ARMGNULDBackend::isSymbolPreemtible(const ResolveInfo& pSym,
                                         unsigned int pType,
                                         const MCLDInfo& pLDInfo)
{
  if(pSym.other() != ResolveInfo::Default)
    return false;

  if(pType != Output::DynObj)
    return false;

  if(pLDInfo.options().Bsymbolic())
    return false;

  return true;
}

/// checkValidReloc - When we attempt to generate a dynamic relocation for
/// ouput file, check if the relocation is supported by dynamic linker.
void ARMGNULDBackend::checkValidReloc(Relocation& pReloc,
                                      unsigned int pOutputType)
{
  // If not building shared object, no relocation type is invalid
  // FIXME: This should be check not building PIC
  if(pOutputType != Output::DynObj)
    return;

  switch(pReloc.type()) {
    case ELF::R_ARM_RELATIVE:
    case ELF::R_ARM_COPY:
    case ELF::R_ARM_GLOB_DAT:
    case ELF::R_ARM_JUMP_SLOT:
    case ELF::R_ARM_ABS32:
    case ELF::R_ARM_ABS32_NOI:
    case ELF::R_ARM_PC24:
    case ELF::R_ARM_TLS_DTPMOD32:
    case ELF::R_ARM_TLS_DTPOFF32:
    case ELF::R_ARM_TLS_TPOFF32:
      break;

    default:
      llvm::report_fatal_error(llvm::Twine("Attempt to generate unsupported") +
                               llvm::Twine("relocation type ") +
                               llvm::Twine(pReloc.type()) +
                               llvm::Twine(" for symbol ") +
                               llvm::Twine(pReloc.symInfo()->name()) +
                               llvm::Twine(", recompile with -fPIC")
                              );
      break;
  }
}

void ARMGNULDBackend::scanLocalReloc(Relocation& pReloc,
                                     MCLinker& pLinker,
                                     const MCLDInfo& pLDInfo,
                                     unsigned int pType)
{
  // rsym - The relocation target symbol
  ResolveInfo* rsym = pReloc.symInfo();

  switch(pReloc.type()){

    case ELF::R_ARM_ABS32:
    case ELF::R_ARM_ABS32_NOI: {
      // If buiding PIC object (shared library or PIC executable),
      // a dynamic relocations with RELATIVE type to this location is needed.
      // Reserve an entry in .rel.dyn
      if(Output::DynObj == pType) {
        // create .rel.dyn section if not exist
        if(!m_pRelDyn)
          createARMRelDyn(pLinker, pType);
        m_pRelDyn->reserveEntry(*m_pRelocFactory);
        // set Rel bit
        rsym->setReserved(rsym->reserved() | 0x1u);
        }
      return;
    }

    case ELF::R_ARM_ABS16:
    case ELF::R_ARM_ABS12:
    case ELF::R_ARM_THM_ABS5:
    case ELF::R_ARM_ABS8:
    case ELF::R_ARM_BASE_ABS:
    case ELF::R_ARM_MOVW_ABS_NC:
    case ELF::R_ARM_MOVT_ABS:
    case ELF::R_ARM_THM_MOVW_ABS_NC:
    case ELF::R_ARM_THM_MOVT_ABS: {
      // If building PIC object (shared library or PIC executable),
      // a dynamic relocation for this location is needed.
      // Reserve an entry in .rel.dyn
      if(Output::DynObj == pType) {
        checkValidReloc(pReloc, pType);
        // create .rel.dyn section if not exist
        if(!m_pRelDyn)
          createARMRelDyn(pLinker, pType);
        m_pRelDyn->reserveEntry(*m_pRelocFactory);
        // set Rel bit
        rsym->setReserved(rsym->reserved() | 0x1u);
      }
      return;
    }
    case ELF::R_ARM_GOTOFF32:
    case ELF::R_ARM_GOTOFF12: {
      // A GOT section is needed
      if(!m_pGOT)
        createARMGOT(pLinker, pType);
      return;
    }

    case ELF::R_ARM_GOT_BREL:
    case ELF::R_ARM_GOT_PREL: {
      // A GOT entry is needed for these relocation type.
      // return if we already create GOT for this symbol
      if(rsym->reserved() & 0x6u)
        return;
      if(!m_pGOT)
        createARMGOT(pLinker, pType);
      m_pGOT->reserveEntry();
      // If building shared object, a dynamic relocationi with
      // type RELATIVE is needed to relocate this GOT entry.
      // Reserve an entry in .rel.dyn
      if(Output::DynObj == pType) {
        // create .rel.dyn section if not exist
        if(!m_pRelDyn)
          createARMRelDyn(pLinker, pType);
        m_pRelDyn->reserveEntry(*m_pRelocFactory);
        // set GOTRel bit
        rsym->setReserved(rsym->reserved() | 0x4u);
        return;
      }
      // set GOT bit
      rsym->setReserved(rsym->reserved() | 0x2u);
      return;
    }

    case ELF::R_ARM_COPY:
    case ELF::R_ARM_GLOB_DAT:
    case ELF::R_ARM_JUMP_SLOT:
    case ELF::R_ARM_RELATIVE: {
      // These are relocation type for dynamic linker, shold not
      // appear in object file.
      llvm::report_fatal_error(llvm::Twine("unexpected reloc ") +
                               llvm::Twine(pReloc.type()) +
                               llvm::Twine("in object file"));
      break;
    }
    default: {
      break;
    }
  } // end switch
}

void ARMGNULDBackend::scanGlobalReloc(Relocation& pReloc,
                                      MCLinker& pLinker,
                                      const MCLDInfo& pLDInfo,
                                      unsigned int pType)
{
  // rsym - The relocation target symbol
  ResolveInfo* rsym = pReloc.symInfo();

  switch(pReloc.type()) {

    case ELF::R_ARM_ABS32:
    case ELF::R_ARM_ABS16:
    case ELF::R_ARM_ABS12:
    case ELF::R_ARM_THM_ABS5:
    case ELF::R_ARM_ABS8:
    case ELF::R_ARM_BASE_ABS:
    case ELF::R_ARM_MOVW_ABS_NC:
    case ELF::R_ARM_MOVT_ABS:
    case ELF::R_ARM_THM_MOVW_ABS_NC:
    case ELF::R_ARM_THM_MOVT_ABS:
    case ELF::R_ARM_ABS32_NOI: {
      // Absolute relocation type, symbol may needs PLT entry or
      // dynamic relocation entry
      if(isSymbolNeedsPLT(*rsym, pType, pLDInfo)) {
        // create plt for this symbol if it does not have one
        if(!(rsym->reserved() & 0x8u)){
          // create .plt and .rel.plt if not exist
          if(!m_pPLT)
            createARMPLTandRelPLT(pLinker, pType);
          // Symbol needs PLT entry, we need to reserve a PLT entry
          // and the corresponding GOT and dynamic relocation entry
          // in .got and .rel.plt. (GOT entry will be reserved simultaneously
          // when calling ARMPLT->reserveEntry())
          m_pPLT->reserveEntry();
          m_pRelPLT->reserveEntry(*m_pRelocFactory);
          // set PLT bit
          rsym->setReserved(rsym->reserved() | 0x8u);
        }
      }

      if(isSymbolNeedsDynRel(*rsym, pType, true)) {
        checkValidReloc(pReloc, pType);
        // symbol needs dynamic relocation entry, reserve an entry in .rel.dyn
        // create .rel.dyn section if not exist
        if(!m_pRelDyn)
          createARMRelDyn(pLinker, pType);
        m_pRelDyn->reserveEntry(*m_pRelocFactory);
        // set Rel bit
        rsym->setReserved(rsym->reserved() | 0x1u);
      }
      return;
    }

    case ELF::R_ARM_GOTOFF32:
    case ELF::R_ARM_GOTOFF12: {
      // A GOT section is needed
      if(!m_pGOT)
        createARMGOT(pLinker, pType);
      return;
    }

    case ELF::R_ARM_REL32:
    case ELF::R_ARM_LDR_PC_G0:
    case ELF::R_ARM_SBREL32:
    case ELF::R_ARM_THM_PC8:
    case ELF::R_ARM_BASE_PREL:
    case ELF::R_ARM_MOVW_PREL_NC:
    case ELF::R_ARM_MOVT_PREL:
    case ELF::R_ARM_THM_MOVW_PREL_NC:
    case ELF::R_ARM_THM_MOVT_PREL:
    case ELF::R_ARM_THM_ALU_PREL_11_0:
    case ELF::R_ARM_THM_PC12:
    case ELF::R_ARM_REL32_NOI:
    case ELF::R_ARM_ALU_PC_G0_NC:
    case ELF::R_ARM_ALU_PC_G0:
    case ELF::R_ARM_ALU_PC_G1_NC:
    case ELF::R_ARM_ALU_PC_G1:
    case ELF::R_ARM_ALU_PC_G2:
    case ELF::R_ARM_LDR_PC_G1:
    case ELF::R_ARM_LDR_PC_G2:
    case ELF::R_ARM_LDRS_PC_G0:
    case ELF::R_ARM_LDRS_PC_G1:
    case ELF::R_ARM_LDRS_PC_G2:
    case ELF::R_ARM_LDC_PC_G0:
    case ELF::R_ARM_LDC_PC_G1:
    case ELF::R_ARM_LDC_PC_G2:
    case ELF::R_ARM_ALU_SB_G0_NC:
    case ELF::R_ARM_ALU_SB_G0:
    case ELF::R_ARM_ALU_SB_G1_NC:
    case ELF::R_ARM_ALU_SB_G1:
    case ELF::R_ARM_ALU_SB_G2:
    case ELF::R_ARM_LDR_SB_G0:
    case ELF::R_ARM_LDR_SB_G1:
    case ELF::R_ARM_LDR_SB_G2:
    case ELF::R_ARM_LDRS_SB_G0:
    case ELF::R_ARM_LDRS_SB_G1:
    case ELF::R_ARM_LDRS_SB_G2:
    case ELF::R_ARM_LDC_SB_G0:
    case ELF::R_ARM_LDC_SB_G1:
    case ELF::R_ARM_LDC_SB_G2:
    case ELF::R_ARM_MOVW_BREL_NC:
    case ELF::R_ARM_MOVT_BREL:
    case ELF::R_ARM_MOVW_BREL:
    case ELF::R_ARM_THM_MOVW_BREL_NC:
    case ELF::R_ARM_THM_MOVT_BREL:
    case ELF::R_ARM_THM_MOVW_BREL: {
      // Relative addressing relocation, may needs dynamic relocation
      if(isSymbolNeedsDynRel(*rsym, pType, false)) {
        checkValidReloc(pReloc, pType);
        // create .rel.dyn section if not exist
        if(!m_pRelDyn)
          createARMRelDyn(pLinker, pType);
        m_pRelDyn->reserveEntry(*m_pRelocFactory);
        // set Rel bit
        rsym->setReserved(rsym->reserved() | 0x1u);
      }
      return;
    }

    case ELF::R_ARM_THM_CALL:
    case ELF::R_ARM_PLT32:
    case ELF::R_ARM_CALL:
    case ELF::R_ARM_JUMP24:
    case ELF::R_ARM_THM_JUMP24:
    case ELF::R_ARM_SBREL31:
    case ELF::R_ARM_PREL31:
    case ELF::R_ARM_THM_JUMP19:
    case ELF::R_ARM_THM_JUMP6:
    case ELF::R_ARM_THM_JUMP11:
    case ELF::R_ARM_THM_JUMP8: {
      // These are branch relocation (except PREL31)
      // A PLT entry is needed when building shared library

      // return if we already create plt for this symbol
      if(rsym->reserved() & 0x8u)
        return;

      // if symbol is defined in the ouput file and it's not
      // preemptible, no need plt
      if(rsym->isDefine() && !rsym->isDyn() &&
         !isSymbolPreemtible(*rsym, pType, pLDInfo)) {
        return;
      }

      // create .plt and .rel.plt if not exist
      if(!m_pPLT)
         createARMPLTandRelPLT(pLinker, pType);
      // Symbol needs PLT entry, we need to reserve a PLT entry
      // and the corresponding GOT and dynamic relocation entry
      // in .got and .rel.plt. (GOT entry will be reserved simultaneously
      // when calling ARMPLT->reserveEntry())
      m_pPLT->reserveEntry();
      m_pRelPLT->reserveEntry(*m_pRelocFactory);
      // set PLT bit
      rsym->setReserved(rsym->reserved() | 0x8u);
      return;
    }

    case ELF::R_ARM_GOT_BREL:
    case ELF::R_ARM_GOT_ABS:
    case ELF::R_ARM_GOT_PREL: {
      // Symbol needs GOT entry, reserve entry in .got
      // return if we already create GOT for this symbol
      if(rsym->reserved() & 0x6u)
        return;
      if(!m_pGOT)
        createARMGOT(pLinker, pType);
      m_pGOT->reserveEntry();
      // If building shared object or the symbol is undefined, a dynamic
      // relocation is needed to relocate this GOT entry. Reserve an
      // entry in .rel.dyn
      if(Output::DynObj == pType || rsym->isUndef() || rsym->isDyn()) {
        // create .rel.dyn section if not exist
        if(!m_pRelDyn)
          createARMRelDyn(pLinker, pType);
        m_pRelDyn->reserveEntry(*m_pRelocFactory);
        // set GOTRel bit
        rsym->setReserved(rsym->reserved() | 0x4u);
        return;
      }
      // set GOT bit
      rsym->setReserved(rsym->reserved() | 0x2u);
      return;
    }

    case ELF::R_ARM_COPY:
    case ELF::R_ARM_GLOB_DAT:
    case ELF::R_ARM_JUMP_SLOT:
    case ELF::R_ARM_RELATIVE: {
      // These are relocation type for dynamic linker, shold not
      // appear in object file.
      llvm::report_fatal_error(llvm::Twine("Unexpected reloc ") +
                               llvm::Twine(pReloc.type()) +
                               llvm::Twine("in object file"));
      break;
    }
    default: {
      break;
    }
  } // end switch
}

void ARMGNULDBackend::scanRelocation(Relocation& pReloc,
                                     MCLinker& pLinker,
                                     const MCLDInfo& pLDInfo,
                                     unsigned int pType)
{
  // rsym - The relocation target symbol
  ResolveInfo* rsym = pReloc.symInfo();
  assert(0 != rsym && "ResolveInfo of relocation not set while scanRelocation");

  // Scan relocation type to determine if an GOT/PLT/Dynamic Relocation
  // entries should be created.
  // FIXME: Below judgements concern only .so is generated as output
  // FIXME: Below judgements concren nothing about TLS related relocation

  // A refernece to symbol _GLOBAL_OFFSET_TABLE_ implies that a .got section
  // is needed
  if((NULL == m_pGOT) && (0 == strcmp(rsym->name(), "_GLOBAL_OFFSET_TABLE_"))) {
    createARMGOT(pLinker, pType);
  }

  // rsym is local
  if(rsym->isLocal())
    scanLocalReloc(pReloc, pLinker, pLDInfo, pType);

  // rsym is global
  else if(rsym->isGlobal())
    scanGlobalReloc(pReloc, pLinker, pLDInfo, pType);

}

uint64_t ARMGNULDBackend::emitSectionData(const Output& pOutput,
                                          const LDSection& pSection,
                                          const MCLDInfo& pInfo,
                                          MemoryRegion& pRegion) const
{
  assert(pRegion.size() && "Size of MemoryRegion is zero!");

  ELFDynObjFileFormat* FileFormat = getDynObjFileFormat();
  assert(FileFormat &&
         "DynObjFileFormat is NULL in ARMGNULDBackend::emitSectionData!");

  unsigned int EntrySize = 0;
  uint64_t RegionSize = 0;

  if (&pSection == m_pAttributes) {
    // FIXME: Currently Emitting .ARM.attributes directly from the input file.
    const llvm::MCSectionData* SectionData = pSection.getSectionData();
    assert(SectionData &&
           "Emit .ARM.attribute failed, MCSectionData doesn't exist!");

    llvm::MCSectionData::const_iterator it = SectionData->begin();
    memcpy(pRegion.start(),
           llvm::cast<MCRegionFragment>(*it).getRegion().start(),
           pRegion.size());
  }

  else if (&pSection == &(FileFormat->getPLT())) {
    assert(m_pPLT && "emitSectionData failed, m_pPLT is NULL!");

    unsigned char* buffer = pRegion.getBuffer();

    m_pPLT->applyPLT0();
    m_pPLT->applyPLT1();

    ARMPLT::iterator it = m_pPLT->begin();
    unsigned int plt0_size = llvm::cast<ARMPLT0>((*it)).getEntrySize();

    memcpy(buffer, llvm::cast<ARMPLT0>((*it)).getContent(), plt0_size);
    RegionSize += plt0_size;
    ++it;

    ARMPLT1* plt1 = 0;
    ARMPLT::iterator ie = m_pPLT->end();
    while (it != ie) {
      plt1 = &(llvm::cast<ARMPLT1>(*it));
      EntrySize = plt1->getEntrySize();
      memcpy(buffer + RegionSize, plt1->getContent(), EntrySize);
      RegionSize += EntrySize;
      ++it;
    }
  }

  else if (&pSection == &(FileFormat->getGOT())) {
    assert(m_pGOT && "emitSectionData failed, m_pGOT is NULL!");

    if(pOutput.type() == Output::DynObj)
      m_pGOT->applyGOT0(FileFormat->getDynamic().addr());
    else
      m_pGOT->applyGOT0(0);

    uint32_t* buffer = reinterpret_cast<uint32_t*>(pRegion.getBuffer());

    GOTEntry* got = 0;
    EntrySize = m_pGOT->getEntrySize();

    for (ARMGOT::iterator it = m_pGOT->begin(),
         ie = m_pGOT->end(); it != ie; ++it, ++buffer) {
      got = &(llvm::cast<GOTEntry>((*it)));
      *buffer = static_cast<uint32_t>(got->getContent());
      RegionSize += EntrySize;
    }
  }

  else
    llvm::report_fatal_error("unsupported section name "
                             + pSection.name() + " !");

  pRegion.sync();

  return RegionSize;
}

/// finalizeSymbol - finalize the symbol value
/// If the symbol's reserved field is not zero, MCLinker will call back this
/// function to ask the final value of the symbol
bool ARMGNULDBackend::finalizeSymbol(LDSymbol& pSymbol) const
{
  return false;
}

ARMGOT& ARMGNULDBackend::getGOT()
{
  assert(0 != m_pGOT && "GOT section not exist");
  return *m_pGOT;
}

const ARMGOT& ARMGNULDBackend::getGOT() const
{
  assert(0 != m_pGOT && "GOT section not exist");
  return *m_pGOT;
}

ARMPLT& ARMGNULDBackend::getPLT()
{
  assert(0 != m_pPLT && "PLT section not exist");
  return *m_pPLT;
}

const ARMPLT& ARMGNULDBackend::getPLT() const
{
  assert(0 != m_pPLT && "PLT section not exist");
  return *m_pPLT;
}

ARMDynRelSection& ARMGNULDBackend::getRelDyn()
{
  assert(0 != m_pRelDyn && ".rel.dyn section not exist");
  return *m_pRelDyn;
}

const ARMDynRelSection& ARMGNULDBackend::getRelDyn() const
{
  assert(0 != m_pRelDyn && ".rel.dyn section not exist");
  return *m_pRelDyn;
}

ARMDynRelSection& ARMGNULDBackend::getRelPLT()
{
  assert(0 != m_pRelPLT && ".rel.plt section not exist");
  return *m_pRelPLT;
}

const ARMDynRelSection& ARMGNULDBackend::getRelPLT() const
{
  assert(0 != m_pRelPLT && ".rel.plt section not exist");
  return *m_pRelPLT;
}

unsigned int
ARMGNULDBackend::getTargetSectionOrder(const LDSection& pSectHdr) const
{
  if (pSectHdr.name() == ".got" || pSectHdr.name() == ".got.plt")
    return SHO_DATA;

  if (pSectHdr.name() == ".plt")
    return SHO_PLT;

  return SHO_UNDEFINED;
}

namespace mcld {

//===----------------------------------------------------------------------===//
/// createARMLDBackend - the help funtion to create corresponding ARMLDBackend
///
TargetLDBackend* createARMLDBackend(const llvm::Target& pTarget,
                                    const std::string& pTriple)
{
  Triple theTriple(pTriple);
  if (theTriple.isOSDarwin()) {
    assert(0 && "MachO linker is not supported yet");
    /**
    return new ARMMachOLDBackend(createARMMachOArchiveReader,
                               createARMMachOObjectReader,
                               createARMMachOObjectWriter);
    **/
  }
  if (theTriple.isOSWindows()) {
    assert(0 && "COFF linker is not supported yet");
    /**
    return new ARMCOFFLDBackend(createARMCOFFArchiveReader,
                               createARMCOFFObjectReader,
                               createARMCOFFObjectWriter);
    **/
  }
  return new ARMGNULDBackend();
}

} // namespace of mcld

//=============================
// Force static initialization.
extern "C" void LLVMInitializeARMLDBackend() {
  // Register the linker backend
  mcld::TargetRegistry::RegisterTargetLDBackend(TheARMTarget, createARMLDBackend);
}
