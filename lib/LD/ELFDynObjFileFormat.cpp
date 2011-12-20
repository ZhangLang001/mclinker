//===- ELFDynObjFileFormat.cpp --------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/LD/ELFDynObjFileFormat.h>
#include <mcld/LD/LDFileFormat.h>
#include <mcld/LD/LDSection.h>
#include <mcld/MC/MCLinker.h>
#include <llvm/Support/ELF.h>

using namespace mcld;
using namespace llvm;

void ELFDynObjFileFormat::initObjectType(MCLinker& pLinker)
{
  SectionFactory &alloc = pLinker.getSectFactory();

  f_pDynSymTab    = alloc.produce(".dynsym",
                                 LDFileFormat::SymbolTable,
                                 ELF::SHT_DYNSYM,
                                 ELF::SHF_ALLOC);
  f_pDynStrTab    = alloc.produce(".dynstr",
                                 LDFileFormat::StringTable,
                                 ELF::SHT_STRTAB,
                                 ELF::SHF_ALLOC);
  f_pInterp       = alloc.produce(".interp",
                                 LDFileFormat::MetaData,
                                 ELF::SHT_PROGBITS,
                                 ELF::SHF_ALLOC);
  f_pHashTab      = alloc.produce(".hash",
                                 LDFileFormat::MetaData,
                                 ELF::SHT_HASH,
                                 ELF::SHF_ALLOC);
  f_pDynamic      = alloc.produce(".dynamic",
                                 LDFileFormat::MetaData,
                                 ELF::SHT_DYNAMIC,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  f_pRelaDyn      = alloc.produce(".rela.dyn",
                                 LDFileFormat::ReadOnly,
                                 ELF::SHT_RELA,
                                 ELF::SHF_ALLOC);
  f_pRelaPlt      = alloc.produce(".rela.plt",
                                 LDFileFormat::ReadOnly,
                                 ELF::SHT_RELA,
                                 ELF::SHF_ALLOC);
  f_pRelaDyn      = alloc.produce(".rel.dyn",
                                 LDFileFormat::ReadOnly,
                                 ELF::SHT_REL,
                                 ELF::SHF_ALLOC);
  f_pRelaPlt      = alloc.produce(".rel.plt",
                                 LDFileFormat::ReadOnly,
                                 ELF::SHT_REL,
                                 ELF::SHF_ALLOC);
  f_pGOT          = alloc.produce(".got",
                                 LDFileFormat::GOT,
                                 ELF::SHT_PROGBITS,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  f_pPLT          = alloc.produce(".plt",
                                 LDFileFormat::PLT,
                                 ELF::SHT_PROGBITS,
                                 ELF::SHF_ALLOC | ELF::SHF_EXECINSTR);
  f_pGOTPLT       = alloc.produce(".got.plt",
                                 LDFileFormat::GOT,
                                 ELF::SHT_PROGBITS,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  f_pPreInitArray = alloc.produce(".preinit_array",
                                 LDFileFormat::Data,
                                 ELF::SHT_PREINIT_ARRAY,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  f_pInitArray    = alloc.produce(".init_array",
                                 LDFileFormat::Data,
                                 ELF::SHT_INIT_ARRAY,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  f_pFiniArray    = alloc.produce(".fini_array",
                                 LDFileFormat::Data,
                                 ELF::SHT_FINI_ARRAY,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  f_pCtors        = alloc.produce(".ctors",
                                 LDFileFormat::Data,
                                 ELF::SHT_PROGBITS,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  f_pDtors        = alloc.produce(".dtors",
                                 LDFileFormat::Data,
                                 ELF::SHT_PROGBITS,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
}

