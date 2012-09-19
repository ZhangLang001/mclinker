//===- GroupReader.h ------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_GROUPREADER_H
#define MCLD_GROUPREADER_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <mcld/Module.h>

namespace mcld
{
class ObjectReader;
class DynObjReader;
class ArchiveReader;
class Archive;

/** \class GroupReader
 *  \brief GroupReader handles the Group Node in InputTree
 *
 *  Group Node is the root of sub-tree in InputTree which includes the iputs in
 *  the command line options --start-group and --end-group options
 */
class GroupReader
{
public:
  GroupReader(Module& pModule,
              ObjectReader& pObjectReader,
              DynObjReader& pDynObjReader,
              ArchiveReader& pArchiveReader);

  ~GroupReader();

  /// readGroup - handle the input sub-tree wich its root is pRoot
  /// @param pRoot - the root Group node of the sub-tree
  bool readGroup(Module::input_iterator pRoot,
                 InputBuilder& pBuilder,
                 const std::string& pTriple);

private:
  /// ArchiveListEntry - record the Archive and the corresponding input iterator
  /// of the archive node
  struct ArchiveListEntry {
    ArchiveListEntry(Archive& pArchive, Module::input_iterator pIterator)
      : archive(pArchive), input(pIterator) {
    }
    Archive& archive;
    Module::input_iterator input;
  };
  typedef struct ArchiveListEntry ArchiveListEntryType;
  typedef std::vector<ArchiveListEntryType*> ArchiveListType;

private:
  Module& m_Module;
  ObjectReader& m_ObjectReader;
  DynObjReader& m_DynObjReader;
  ArchiveReader& m_ArchiveReader;
};

} // namespace of mcld

#endif
