//===- MipsRelocationFactory.h --------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MIPS_RELOCATION_FACTORY_H
#define MIPS_RELOCATION_FACTORY_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <mcld/LD/RelocationFactory.h>
#include <mcld/Support/GCFactory.h>
#include <mcld/Target/SymbolEntryMap.h>
#include "MipsLDBackend.h"

namespace mcld {

/** \class MipsRelocationFactory
 *  \brief MipsRelocationFactory creates and destroys the Mips relocations.
 */
class MipsRelocationFactory : public RelocationFactory
{
public:
  typedef SymbolEntryMap<GOT::Entry> SymGOTMap;

public:
  MipsRelocationFactory(size_t pNum, MipsGNULDBackend& pParent);

  Result applyRelocation(Relocation& pRelocation);

  MipsGNULDBackend& getTarget()
  { return m_Target; }

  const MipsGNULDBackend& getTarget() const
  { return m_Target; }

  // Get last calculated AHL.
  int32_t getAHL() const
  { return m_AHL; }

  // Set last calculated AHL.
  void setAHL(int32_t pAHL)
  { m_AHL = pAHL; }

  const char* getName(Relocation::Type pType) const;

  const SymGOTMap& getSymGOTMap() const { return m_SymGOTMap; }
  SymGOTMap&       getSymGOTMap()       { return m_SymGOTMap; }

private:
  MipsGNULDBackend& m_Target;
  int32_t m_AHL;
  SymGOTMap m_SymGOTMap;
};

} // namespace of mcld

#endif
