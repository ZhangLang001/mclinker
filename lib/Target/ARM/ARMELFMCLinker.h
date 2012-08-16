//===- ARMELFMCLinker.h ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef ARM_ELF_SECTION_LINKER_H
#define ARM_ELF_SECTION_LINKER_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif
#include <mcld/CodeGen/MCLinker.h>

namespace mcld {

class Module;

/** \class ARMELFMCLinker
 *  \brief ARMELFMCLinker sets up the environment for linking.
 */
class ARMELFMCLinker : public MCLinker
{
public:
  ARMELFMCLinker(SectLinkerOption &pOption,
                 mcld::TargetLDBackend &pLDBackend,
                 mcld::Module& pModule);

  ~ARMELFMCLinker();
};

} // namespace of mcld

#endif

