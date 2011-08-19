/*****************************************************************************
 *   The MCLinker Project, Copyright (C), 2011 -                             *
 *   Embedded and Web Computing Lab, National Taiwan University              *
 *   MediaTek, Inc.                                                          *
 *                                                                           *
 *   Chun-Hung Lu <chun-hung.lu@mediatek.com>                                *
 ****************************************************************************/
#ifndef MCLD_DIRECTORY_H
#define MCLD_DIRECTORY_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif
#include <llvm/Support/Allocator.h>
#include <llvm/ADT/StringMap.h>
#include <mcld/Support/FileSystem.h>
#include <mcld/Support/Path.h>
#include <mcld/ADT/TypeTraits.h>

#ifdef MCLD_DEBUG
#include<iostream>
using namespace std;
#endif


namespace mcld {
namespace sys {
namespace fs {

class DirIterator;

/** \class Directory
 *  \brief A Directory object stores a Path object, a FileStatus object for
 *   non-symbolic link status, and a FileStatus object for symbolic link
 *   status. The FileStatus objects act as value caches.
 */
class Directory
{
friend void detail::directory_iterator_increment(DirIterator& pIter);
friend void detail::open_dir(Directory& pDir);
friend void detail::close_dir(Directory& pDir);
private:
  friend class DirIterator;
  typedef llvm::StringMap<sys::fs::Path, llvm::BumpPtrAllocator> PathCache;

public:
  typedef DirIterator iterator;

public:
  /// default constructor
  Directory();

  /// constructor - a directory whose path is pPath
  explicit Directory(const Path& pPath,
                     FileStatus st = FileStatus(),
                     FileStatus symlink_st = FileStatus());

  /// copy constructor
  /// when a copying construction happens, the cache is not copied.
  Directory(const Directory& pCopy);

  /// assignment
  /// When an assignment occurs, the cache is clear.
  Directory& operator=(const Directory& pCopy);

  /// destructor, inheritable.
  virtual ~Directory();

  /// Since we have default construtor, we must provide assign.
  void assign(const Path& pPath, 
              FileStatus st = FileStatus(),
              FileStatus symlink_st = FileStatus());

  /// path - the path of the directory
  const Path& path() const
  { return m_Path; }

  FileStatus status() const;
  FileStatus symlinkStatus() const;

  // -----  iterators  ----- //
  // While the iterators move, the direcotry is modified.
  // Thus, we only provide non-constant iterator.
  iterator begin();
  iterator end();

protected:
  mcld::sys::fs::Path m_Path;
  mutable FileStatus m_FileStatus;
  mutable FileStatus m_SymLinkStatus;
  intptr_t m_Handler;  
  // the cache of directory
  PathCache m_Cache;
};

/** \class DirIterator
 *  \brief A DirIterator object can traverse all entries in a Directory
 *
 *  DirIterator will open the directory and add entry into Directory::m_Cache
 *  DirIterator() is the end of a directory.
 *  If the end of the directory elements is reached, the iterator becomes
 *  equal to the end iterator value - DirIterator().
 *
 *  @see Directory
 */
class DirIterator
{
friend void detail::directory_iterator_increment(DirIterator& pIter);
friend class Directory;
private:
  typedef Directory::PathCache            DirCache;

public:
  typedef Directory                       value_type;
  typedef ConstTraits<Directory>          const_traits;
  typedef NonConstTraits<Directory>       non_const_traits;
  typedef std::input_iterator_tag         iterator_category;
  typedef size_t                          size_type;
  typedef ptrdiff_t                       difference_type;

private:
  explicit DirIterator(Directory& pDirectory, const DirCache::iterator& pIter);

public:
  DirIterator();
  DirIterator(const DirIterator &X);
  ~DirIterator();
  DirIterator& operator=(const DirIterator& pCopy);

  DirIterator& operator++();
  DirIterator operator++(int);

  Path* path()
  { return m_pPath; }

  const Path* path() const
  { return m_pPath; }

  bool operator==(const DirIterator& y) const;
  bool operator!=(const DirIterator& y) const;

private:
  Directory* m_pParent;
  Path* m_pPath;
  Directory::PathCache::iterator* m_pIdx;
};

} // namespace of fs
} // namespace of sys
} // namespace of mcld

#endif
