//===- IRBuilder.h --------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// IRBuilder is a class used as a convenient way to create MCLinker sections
// with a consistent and simplified interface.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_IRBUILDER_H
#define MCLD_IRBUILDER_H

#include <mcld/MC/MCLDInput.h>
#include <mcld/MC/InputBuilder.h>

#include <mcld/LD/LDSection.h>

#include <mcld/Fragment/RegionFragment.h>

#include <mcld/Support/Path.h>
#include <mcld/Support/FileHandle.h>
#include <mcld/Support/raw_mem_ostream.h>

namespace mcld {

class Module;
class LinkerConfig;
class InputTree;

/** \class IRBuilder
 *  \brief IRBuilder provides an uniform API for creating sections and
 *  inserting them into a input file.
 *
 *  Ahead-of-time virtual machines (VM) usually compiles an intermediate
 *  language into a system-dependent binary.  IRBuilder helps such kind of VMs
 *  to emit binaries in native object format, such as ELF or MachO.
 */
class IRBuilder
{
public:
  enum ObjectFormat {
    ELF,
    MachO,
    COFF
  };

public:
  IRBuilder(Module& pModule, InputTree& pInputs, const LinkerConfig& pConfig);

  ~IRBuilder();

/// @}
/// @name Input Files On The Command Line
/// @{

  /// CreateInput - Make a new input file and append it to the input tree.
  ///
  /// This function is like to add an input file in the command line.
  ///
  /// There are four types of the input files:
  ///   - relocatable objects,
  ///   - shared objects,
  ///   - archives,
  ///   - and user-defined objects.
  ///
  /// If Input::Unknown type is given, MCLinker will automatically
  /// open and read the input file, and create sections of the input. Otherwise,
  /// users need to manually create sections by IRBuilder.
  ///
  /// @see mcld::Input
  /// 
  /// @param pPath [in] The path of the input file.
  /// @param pType [in] The type of the input file. MCLinker will parse the
  ///                   input file to create sections only if pType is
  ///                   Input::Unknown.
  /// @return the created mcld::Input. The name of the input is set to
  /// the filename of the pPath.
  Input* CreateInput(const sys::fs::Path& pPath,
                     unsigned int pType = Input::Unknown);

  /// CreateInput - Make a new input file and append it to the input tree.
  ///
  /// This function is like to add an input file in the command line.
  ///
  /// @param pName [in] The name of the input file.
  /// @param pPath [in] The path of the input file.
  /// @param pType [in] The type of the input file. MCLinker will parse the
  ///                   input file to create sections only if pType is
  ///                   Input::Unknown.
  /// @return the created mcld::Input.
  Input* CreateInput(const std::string& pName,
                     const sys::fs::Path& pPath,
                     unsigned int pType = Input::Unknown);

  /// ReadInput - To read an input file and append it to the input tree.
  ///
  /// This function is equal to -l option. This function tells MCLinker to
  /// search for lib[pNameSpec].so or lib[pNameSpec].a in the search path.
  ///
  /// @param pNameSpec [in] The namespec of the input file.
  /// @return the created mcld::Input.
  Input* ReadInput(const std::string& pNameSpec);

  /// ReadInput - To read an input file and append it to the input tree.
  ///
  /// This function is like to add an input in the command line.
  ///
  /// LLVM compiler usually emits outputs by llvm::raw_ostream.
  /// mcld::raw_mem_ostream inherits llvm::raw_ostream and is suitable to be
  /// the output of LLVM compier. Users can connect LLVM compiler and MCLinker
  /// by passing mcld::raw_mem_ostream from LLVM compiler to MCLinker.
  ///
  /// @param pMemOStream [in] The input raw_mem_stream
  /// @param the create mcld::Input.
  Input* ReadInput(raw_mem_ostream& pMemOStream);

  /// ReadInput - To read an input file and append it to the input tree.
  ///
  /// This function is a conventient way to call
  /// @code{.cpp}
  ///   Input* input = IRBuilder::CreateInput(pName, pPath, Input::Unknown);
  /// @endcode
  Input* ReadInput(const std::string& pName, const sys::fs::Path& pFilePath);

  /// ReadInput - To read an input file and append it to the input tree.
  ///
  /// This function tells MCLinker to read file descriptor pFD. Users open
  /// object file manually and pass the file descriptor to MCLinker. MCLinker 
  /// must have the permission to read the file.
  Input* ReadInput(const std::string& pName, int pFD);

  /// ReadInput - To read an input file and append it to the input tree.
  /// Another way to open file manually. Use MCLinker's mcld::FileHandle.
  Input* ReadInput(FileHandle& pFileHandle);

  /// ReadInput - To read an input file and append it to the input tree.
  ///
  /// This function is like to add an input in the command line.
  ///
  /// This function tells MCLinker to read pRawMemory as an image of an object
  /// file. So far, MCLinekr only supports ELF object format, but it will
  /// support various object formats in the future. MCLinker relies triple to
  /// know the object format of pRawMemory.
  /// @param [in] pName      The name of the input file
  /// @param [in] pRawMemory An image of object file
  /// @param [in] pSize      The size of the memory
  /// @return The created mcld::Input
  Input* ReadInput(const std::string& pName, void* pRawMemory, size_t pSize);

  /// StartGroup - Add an opening tag of group.
  ///
  /// This function is equal to --start-group option. This function tells
  /// MCLinker to create a new archive group and to add the following archives
  /// in the created group. The archives in a group are searched repeatedly
  /// until no new undefined references are created.
  bool StartGroup();

  /// EndGroup - Add a closing tag of group.
  ///
  /// This function is equal to --end-group option. This function tells
  /// MCLinker to stop adding following archives in the created group.
  bool EndGroup();

/// @}
/// @name Positional Options On The Command Line
/// @{

  /// WholeArchive - Append a --whole-archive option on the command line
  ///
  /// This function is equal to --whole-archive option. This function tells
  /// MCLinker to include every object files in the following archives.
  void WholeArchive();

  /// NoWholeArchive - Append a --no-whole-archive option on the command line.
  ///
  /// This function is equal to --no-whole-archive option. This function tells
  /// MCLinker to stop including every object files in the following archives.
  /// Only used object files in the following archives are included.
  void NoWholeArchive();

  /// AsNeeded - Append a --as-needed option on the command line.
  ///
  /// This function is equal to --as-needed option. This function tells
  /// MCLinker to not add a DT_NEEDED tag in .dynamic sections for the
  /// following shared objects that are not really used. MCLinker will add tags
  //  only for the following shared objects which is really used.
  void AsNeeded();

  /// NoAsNeeded - Append a --no-as-needed option on the command line.
  ///
  /// This function is equal to --no-as-needed option. This function tells
  /// MCLinker to add a DT_NEEDED tag in .dynamic section for every shared
  /// objects that is created after this option.
  void NoAsNeeded();

  /// CopyDTNeeded - Append a --add-needed option on the command line.
  ///
  /// This function is equal to --add-needed option. This function tells
  /// NCLinker to copy all DT_NEEDED tags of every following shared objects
  /// to the output file.
  void CopyDTNeeded();

  /// NoCopyDTNeeded - Append a --no-add-needed option on the command line.
  ///
  /// This function is equal to --no-add-needed option. This function tells
  /// MCLinker to stop copying all DT_NEEDS tags in the following shared
  /// objects to the output file.
  void NoCopyDTNeeded();

  /// AgainstShared - Append a -Bdynamic option on the command line.
  ///
  /// This function is equal to -Bdynamic option. This function tells MCLinker
  /// to search shared objects before archives for the following namespec.
  void AgainstShared();

  /// AgainstStatic - Append a -static option on the command line.
  ///
  /// This function is equal to -static option. This function tells MCLinker to
  /// search archives before shared objects for the following namespec.
  void AgainstStatic();

/// @}
/// @name Input Methods
/// @{

  /// CreateSection - To create and append a section in the input file.
  ///
  /// @param pInput [in, out] The input file.
  /// @param pName  [in]      The name of the section.
  /// @param pType  [in]      The meaning of the content in the section. The
  ///                         value is format-dependent. In ELF, the value is
  ///                         SHT_* in normal.
  /// @param pFlag  [in]      The format-dependent flag. In ELF, the value is
  ///                         SHF_* in normal.
  /// @param pAlign [in]      The alignment constraint of the section
  /// @return The created section header and section data.
  template<unsigned int OF>
  LDSection* CreateSection(Input& pInput,
                           const std::string& pName,
                           uint32_t pType,
                           uint32_t pFlag,
                           uint32_t pAlign)
  { assert("use template partial specific function"); return NULL; }

  /// CreateRegion - To create a region fragment in the input file.
  /// This function tells MCLinker to read a piece of data from the input
  /// file, and to create a region fragment that carries the data. The data
  /// will be deallocated automatically when pInput is destroyed.
  ///
  /// @param pInput  [in, out] The input file.
  /// @param pOffset [in]      The starting file offset of the data
  /// @param pLength [in]      The number of bytes of the data
  RegionFragment* CreateRegion(Input& pInput, size_t pOffset, size_t pLength);

  /// CreateRegion - To create a region fragment wrapping the given memory
  /// This function tells MCLinker to create a region fragment by the data
  /// directly. Since the data is given, not in the input file, users should
  /// deallocated the data manually.
  ///
  /// @param pMemory [in] The start address of the given data
  /// @param pLength [in] The number of bytes of the data
  RegionFragment* CreateRegion(void* pMemory, size_t pLength);

  /// AppendFragment - To append a fragment in the section.
  /// To append pFrag and to increase the size of appended section.
  ///
  /// Different kinds of sections need different kind of fragments. For BSS
  /// sections, people should insert fillment fragments. For EH frame sections
  /// people should insert CIEs and FDEs. For relocation sections, people
  /// should insert Relocation fragments. For the other kind of sections,
  /// any kind of fragments can be inserted, but the most frequently used
  /// fragments are region fragments.
  ///
  /// @param [in, out] pFrag The appended fragment. The offset of the fragment
  ///                        is also be adjusted.
  /// @param [in, out] pSection The section being appended. The size of the
  ///                           section is adjusted. The offset of the other
  ///                           sections in the same input file are also be
  ///                           adjusted.
  bool AppendFragment(Fragment& pFrag, LDSection& pSection);

private:
  Module& m_Module;
  InputTree& m_InputTree;
  const LinkerConfig& m_Config;

  InputBuilder m_InputBuilder;
};

template<>
LDSection* IRBuilder::CreateSection<IRBuilder::ELF>(Input& pInput,
                                                    const std::string& pName,
                                                    uint32_t pType,
                                                    uint32_t pFlag,
                                                    uint32_t pAlign);
} // end of namespace mcld

#endif
