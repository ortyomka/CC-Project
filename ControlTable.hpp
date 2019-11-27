#ifndef TUTORIAL_CONTROLTABLE_HPP
#define TUTORIAL_CONTROLTABLE_HPP

#include "SymbolTable.hpp"
#include "TypeTable.hpp"
#include <memory>

class ControlTable {
public:
  ControlTable();
  ControlTable(std::shared_ptr<ControlTable> parent);
  ~ControlTable() = default;

  bool addSimpleType(const std::string &name,
                     const std::string originalTypeName);
  bool addArrayType(const std::string &name, const std::string &originalName,
                    CNode *expression);
  bool addRecordType(const std::string &name, CNode *fields);

  bool addVariable(const std::string &name, const std::string &type,
                   CNode *expression);
  bool addFunction(const std::string &name, const std::string &return_type,
                   CNode *paramaters);

  bool isVariable(const std::string &name);
  bool isFunction(const std::string &name);

  bool isType(const std::string &name);

  std::shared_ptr<ControlTable>
  getSubScopeTable(const std::string &scope_name) const;

  // add subScope
  bool addSubScope(const std::string &scope_name,
                   std::shared_ptr<ControlTable> sub_scope);

  // for inner structures
  // index will be number of scope
  // in superscope start from 1
  // return key
  std::string addSubScope(std::shared_ptr<ControlTable> sub_scope);

private:
  std::shared_ptr<VariableNode> getVariable(const std::string &name);
  std::shared_ptr<FunctionNode> getFunction(const std::string &name);
  std::shared_ptr<TypeNode> getType(const std::string &name);

  std::weak_ptr<ControlTable> parent_;
  std::unique_ptr<TypeTable> type_table_;
  std::unique_ptr<SymbolTable> symbol_table_;
  std::unordered_map<std::string, std::shared_ptr<ControlTable>> sub_scopes_;
};

#endif // TUTORIAL_CONTROLTABLE_HPP
