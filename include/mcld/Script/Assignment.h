//===- Assignment.h -------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_SCRIPT_ASSIGNMENT_COMMAND_INTERFACE_H
#define MCLD_SCRIPT_ASSIGNMENT_COMMAND_INTERFACE_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <mcld/Script/ScriptCommand.h>

namespace mcld
{

class Module;
class LinkerScript;
class RpnExpr;
class SymOperand;

/** \class Assignment
 *  \brief This class defines the interfaces to assignment command.
 */

class Assignment : public ScriptCommand
{
public:
  enum Level {
    OUTSIDES_SECTIONS, // outsied SECTIONS command
    OUTPUT_SECTION,    // related to an output section
    INPUT_SECTION      // related to an input section
  };

  enum Type {
    DEFAULT,
    HIDDEN,
    PROVIDE,
    PROVIDE_HIDDEN
  };

public:
  Assignment(const Module& pModule,
             LinkerScript& pScript,
             Level pLevel,
             Type pType,
             SymOperand& pSymbol,
             RpnExpr& pRpnExpr);

  ~Assignment();

  Assignment& operator=(const Assignment& pAssignment);

  Level level() const { return m_Level; }

  Type type() const { return m_Type; }

  const SymOperand& symbol() const { return m_Symbol; }
  SymOperand&       symbol()       { return m_Symbol; }

  const RpnExpr& getRpnExpr() const { return m_RpnExpr; }
  RpnExpr&       getRpnExpr()       { return m_RpnExpr; }

  void dump() const;

  static bool classof(const ScriptCommand* pCmd)
  {
    return pCmd->getKind() == ScriptCommand::ASSIGNMENT;
  }

  void activate();

  /// assign - evaluate the rhs and assign the result to lhs.
  bool assign();

private:
  const Module& m_Module;
  LinkerScript& m_Script;
  Level m_Level;
  Type m_Type;
  SymOperand& m_Symbol;
  RpnExpr& m_RpnExpr;
};

} // namespace of mcld

#endif

