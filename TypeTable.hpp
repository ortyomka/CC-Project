#ifndef CC_PROJECT_TYPETABLE_HPP
#define CC_PROJECT_TYPETABLE_HPP

#include "TypeNode.hpp"
#include <unordered_map>

class TypeTable {
public:
  TypeTable();

  ~TypeTable() = default;

  bool addSimpleType(const std::string &cname, std::shared_ptr<TypeNode> type);

  bool addArrayType(const std::string &cname, CNode *expression, std::shared_ptr<TypeNode> type);

  bool addRecordType(const std::string &cname, CNode *record);

  std::shared_ptr<TypeNode> getType(const std::string &cname);

  bool isType(const std::string &cname);

  bool removeType(const std::string &cname);

private:
  std::unordered_map<std::string, std::shared_ptr<TypeNode>> types;
};

#endif // CC_PROJECT_TYPETABLE_HPP
