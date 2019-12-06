#include "semantic_analyzer/ControlTable.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>

ControlTable::ControlTable() {
  parent_.reset();
  type_table_ = std::make_unique<TypeTable>();
  symbol_table_ = std::make_unique<SymbolTable>();
  type_table_->addType("integer", std::make_shared<SimpleType>("integer"));
  type_table_->addType("real", std::make_shared<SimpleType>("real"));
  type_table_->addType("boolean", std::make_shared<SimpleType>("boolean"));
}

ControlTable::ControlTable(ControlTable *parent) {
  parent_ = parent->shared_from_this();
  type_table_ = std::make_unique<TypeTable>();
  symbol_table_ = std::make_unique<SymbolTable>();
}

std::shared_ptr<TypeNode> ControlTable::CNode2TypeNode(CNode *type) {
  if (type == nullptr)
    return nullptr;
  if (type->name != "type")
    return nullptr;

  CNode *realType = type->children[0];
  if (realType->name == "array_type") {
    CNode *exp = realType->children[0];
    CNode *itemType = realType->children[1];
    auto typeNode = CNode2TypeNode(itemType);
    if (typeNode == nullptr)
      return nullptr;
    return std::make_shared<ArrayType>(exp, typeNode);
  } else if (realType->name == "record_type") {
    CNode *fields = realType->children[0];
    std::vector<std::shared_ptr<VariableNode>> fields_list;
    if (fields != nullptr) {
      if (!CNode2FieldList(fields, fields_list))
        return nullptr;
    }
    return std::make_shared<RecordType>(fields_list);
  } else {
    return getType(realType->name);
  }
}

bool ControlTable::CNode2FieldList(
    CNode *fields, std::vector<std::shared_ptr<VariableNode>> &fields_list) {
  if (fields == nullptr)
    return false;
  if (fields->name != "variables_declaration") {
    return false;
  }
  int amount_children = fields->children.size();
  for (int i = 0; i < amount_children; i++) {
    CNode *child = fields->children[i];
    std::shared_ptr<TypeNode> type;
    if (child->name == "variable_declaration") {
      type = CNode2TypeNode(child->children[1]);
      if (type == nullptr)
        return false;
    } else if (child->name == "variable_declaration_auto") {
      type = std::make_shared<AutoType>();
    } else {
      return false;
    }
    auto new_field = std::make_shared<VariableNode>(child->children[0]->name,
                                                    type, child->children[1]);
    fields_list.push_back(new_field);
  }
  return true;
}

bool ControlTable::addAutoVariable(const std::string &name, CNode *expression) {
  if (expression == nullptr)
    return false;
  auto typeNode = std::make_shared<AutoType>();
  return symbol_table_->addVariable(name, typeNode, expression);
}

bool ControlTable::addFunction(const std::string &name, CNode *return_type,
                               CNode *parameters) {
  std::shared_ptr<TypeNode> typeNode = std::make_shared<NoTypeNode>();
  if (return_type != nullptr) {
    typeNode = CNode2TypeNode(return_type);
    if (typeNode == nullptr)
      return false;
  }

  std::vector<std::shared_ptr<VariableNode>> parameters_list = {};
  if (parameters != nullptr) {
    if (parameters->name != "parameters")
      return false;

    std::unordered_set<std::string> set = {};
    for (int i = 0; i < parameters->children.size(); i++) {
      if (parameters->children[i]->name != "parameter_declaration") {
        return false;
      }
      if (parameters->children[i]->children.size() != 2)
        return false;
      std::string param_name = parameters->children[i]->children[0]->name;
      if (set.find(param_name) != set.end()) {
        return false;
      }
      set.insert(param_name);
      auto type = getType(parameters->children[i]->children[1]->name);
      if (type == nullptr) {
        return false;
      }
      auto parameter =
          std::make_shared<VariableNode>(param_name, type, nullptr);
      parameters_list.push_back(parameter);
    }
  }

  if (symbol_table_->addFunction(name, typeNode, parameters_list)) {
    addSubScope(name);
    auto subScope = getSubScopeTable(name);
    for (int i = 0; i < parameters_list.size(); i++) {
      subScope->addVariable(parameters_list[i]->variable_name_,
                            parameters_list[i]->variable_type_,
                            parameters_list[i]->default_value_);
    }
    return true;
  }
  return false;
}

bool ControlTable::isVariable(const std::string &name) {
  if (symbol_table_->isVariable(name))
    return true;
  if (!parent_.expired())
    return parent_.lock()->isVariable(name);
  return false;
}

bool ControlTable::isFunction(const std::string &name) {
  if (symbol_table_->isFunction(name))
    return true;
  if (!parent_.expired())
    return parent_.lock()->isFunction(name);
  return false;
}

bool ControlTable::isType(const std::string &name) {
  if (type_table_->isType(name))
    return true;
  if (!parent_.expired())
    return parent_.lock()->isType(name);
  return false;
}

std::shared_ptr<ControlTable>
ControlTable::getSubScopeTable(const std::string &scope_name) const {
  auto scope = sub_scopes_.find(scope_name);
  if (scope == sub_scopes_.end()) {
    return nullptr;
  } else {
    return (*scope).second;
  }
}

bool ControlTable::addSubScope(const std::string &scope_name) {
  auto scope = sub_scopes_.find(scope_name);
  if (scope != sub_scopes_.end())
    return false;
  auto sub_scope = std::make_shared<ControlTable>(this);
  sub_scopes_.insert(std::pair<std::string, std::shared_ptr<ControlTable>>(
      scope_name, sub_scope));
  return true;
}

std::string ControlTable::addSubScope() {
  std::string key_scope = std::to_string(sub_scopes_.size() + 1);
  if (addSubScope(key_scope))
    return key_scope;
  return "";
}

std::shared_ptr<VariableNode>
ControlTable::getVariable(const std::string &name) {
  auto result = symbol_table_->getVariable(name);
  if (result != nullptr)
    return result;
  if (!parent_.expired())
    return parent_.lock()->getVariable(name);
  return nullptr;
}

std::shared_ptr<FunctionNode>
ControlTable::getFunction(const std::string &name) {
  auto result = symbol_table_->getFunction(name);
  if (result != nullptr)
    return result;
  if (!parent_.expired())
    return parent_.lock()->getFunction(name);
  return nullptr;
}

std::shared_ptr<TypeNode> ControlTable::getType(const std::string &name) {
  auto result = type_table_->getType(name);
  if (result != nullptr)
    return result;
  if (!parent_.expired())
    return parent_.lock()->getType(name);
  return nullptr;
}
std::shared_ptr<ControlTable> ControlTable::getParent() const {
  if (parent_.expired())
    return nullptr;
  return parent_.lock();
}
bool ControlTable::addType(const std::string &name, CNode *type) {
  auto typeNode = CNode2TypeNode(type);
  if (typeNode == nullptr)
    return false;
  return type_table_->addType(name, typeNode);
}

bool ControlTable::addVariable(std::string name, CNode *type,
                               CNode *expression) {
  auto typeNode = CNode2TypeNode(type);
  if (typeNode == nullptr)
    return false;
  return symbol_table_->addVariable(name, typeNode, expression);
}

bool ControlTable::addType(const std::string &name,
                           std::shared_ptr<TypeNode> type) {
  if (type == nullptr)
    return false;
  return type_table_->addType(name, type);
}

bool ControlTable::addVariable(const std::string &name,
                               std::shared_ptr<TypeNode> type,
                               CNode *expression) {
  if (type == nullptr)
    return false;
  return symbol_table_->addVariable(name, type, expression);
}

bool ControlTable::checkFunctionCall(const std::string &functionName,
                                     CNode *arguments) {
  if (!isFunction(functionName)) {
    return false;
  }
  auto function = getFunction(functionName);
  auto param = function->parameters_;
  if (param.size() == 0 and arguments == nullptr)
    return true;
  else if (arguments != nullptr && param.size() == arguments->children.size()) {

    std::vector<std::shared_ptr<VariableNode>> arg_list = {};
    if (!CNode2ArgList(arguments, arg_list)) {
      return false;
    }
    // TODO Compare types and variables;
    return true;
  }
  return false;
}

bool ControlTable::CNode2ArgList(
    CNode *args, std::vector<std::shared_ptr<VariableNode>> &args_list) {
  // TODO After calculate expression
  return true;
}

bool ControlTable::check_modifiable(CNode *node) {
  std::shared_ptr<TypeNode> currentType = nullptr;
  return check_modifiable(node, currentType);
}
bool ControlTable::check_modifiable(CNode *node,
                                    std::shared_ptr<TypeNode> &currentType) {
  if (node->name == "modifiable_primary") {
    if (currentType == nullptr) {
      auto var = getVariable(node->children[0]->name);
      if (var == nullptr) {
        return false;
      }
      currentType = var->variable_type_;
      return true;
    } else if (currentType->getType() == Types::Record) {
      auto fields = std::dynamic_pointer_cast<RecordType>(currentType)->fields;
      auto field = std::find_if(
          fields.begin(), fields.end(),
          [=](const std::shared_ptr<VariableNode> &field) {
            return field->variable_name_ == node->children[0]->name;
          });
      if (field == fields.end()) {
        return false;
      }
      currentType = (*field)->variable_type_;
      return true;
    }
  } else if (node->name == "modifiable_primary_array") {
    if (!check_modifiable(node->children[0], currentType)) {
      return false;
    }
    if (currentType->getType() != Types::Array) {
      return false;
    }
    currentType = std::dynamic_pointer_cast<ArrayType>(currentType)->arrayType;
    return processingExpression(node, 1);
  } else if (node->name == "modifiable_primary_field") {
    if (!check_modifiable(node->children[0], currentType)) {
      return false;
    }

    if (currentType->getType() != Types::Record) {
      return false;
    }

    return check_modifiable(node->children[1], currentType);
  }
  return false;
}

bool ControlTable::addCounter(const std::string &name) {
  return addVariable(name, getType("integer"), nullptr);
}

void changeChild(CNode *&src_node, CNode *res_node) {
  for (CNode *child : src_node->children) {
    if (res_node != child)
      delete child;
  }
  delete src_node;
  src_node = res_node;
}

int toInteger(const CNode *node) {
  if (node->name == "integer") {
    return std::stoi(node->children[0]->name);
  } else if (node->name == "real") {
    double d = std::stod(node->children[0]->name);
    int i = (int)d;
    if ((i + 0.5) <= d)
      i++;
    return i;
  } else if (node->name == "boolean") {
    if (node->children[0]->name == "true") {
      return 1;
    }
    return 0;
  }
  std::cerr << "Unknown type of CNode" << std::endl;
  std::exit(1);
}

bool toBoolean(const CNode *node) {
  if (node->name == "integer") {
    int g = std::stoi(node->children[0]->name);
    if (g == 1)
      return true;
    else if (g == 0)
      return false;
    else {
      std::cerr << "Cannot convert " << g << " to boolean" << std::endl;
      exit(1);
    }
  } else if (node->name == "real") {
    std::cerr << "Real " << node->children[0]->name
              << " cannot be converted to boolean" << std::endl;
    exit(1);
  } else if (node->name == "boolean") {
    return node->children[0]->name == "true";
  }
  std::cerr << "Unknown type of CNode" << std::endl;
  std::exit(1);
}

double toReal(const CNode *node) {
  if (node->name == "integer") {
    return std::stod(node->children[0]->name);
  } else if (node->name == "real") {
    return std::stod(node->children[0]->name);
  } else if (node->name == "boolean") {
    return node->children[0]->name == "true";
  }
  std::cerr << "Unknown type of CNode" << std::endl;
  std::exit(1);
}

CNode *ControlTable::calculate(CNode *node) {
  CNode *res_node = nullptr;
  if (node->name == "expression") {
    res_node = calculate(node->children[0]);

    if (res_node != node->children[0])
      changeChild(node->children[0], res_node);

    if (node->children.size() == 3) {
      auto second_node = calculate(node->children[2]);
      if (second_node != node->children[2])
        changeChild(node->children[2], second_node);

      if (!(res_node->name == "integer" || res_node->name == "boolean")) {
        if (res_node->name == "real") {
          std::cerr << "Real " << res_node->children[0]->name
                    << " cannot be converted to boolean" << std::endl;
          exit(1);
        }
        return node;
      }

      if (!(second_node->name == "integer" || second_node->name == "boolean")) {
        if (second_node->name == "real") {
          std::cerr << "Real " << second_node->children[0]->name
                    << " cannot be converted to boolean" << std::endl;
          exit(1);
        }
        return node;
      }

      bool real_l = toBoolean(res_node);
      bool real_r = toBoolean(second_node);

      std::string op = node->children[1]->name;
      bool res;
      if (op == "and") {
        res = real_l && real_r;
      } else if (op == "or") {
        res = real_l || real_r;
      } else if (op == "xor") {
        res = (real_l || real_r) && !(real_l && real_r);
      } else {
        // ERROR
        return node;
      }

      CNode *resultNode = new CNode("boolean");
      if (res) {
        resultNode->children.push_back(new CNode("true"));
      } else {
        resultNode->children.push_back(new CNode("false"));
      }
      return resultNode;
    }
    return node->children[0];
  } else if (node->name == "relation") {
    res_node = calculate(node->children[0]);

    if (res_node != node->children[0])
      changeChild(node->children[0], res_node);
    if (node->children.size() == 3) {
      auto second_node = calculate(node->children[2]);
      if (second_node != node->children[2])
        changeChild(node->children[2], second_node);

      if (!(res_node->name == "integer" || res_node->name == "real")) {
        if (res_node->name == "boolean") {
          std::cerr << "Cannot use comparing with boolean" << std::endl;
          exit(1);
        }
        return node;
      }

      if (!(second_node->name == "integer" || second_node->name == "real")) {
        if (second_node->name == "boolean") {
          std::cerr << "Cannot use comparing with boolean" << std::endl;
          exit(1);
        }
        return node;
      }

      double real_l = toReal(res_node);
      double real_r = toReal(second_node);

      std::string op = node->children[1]->name;
      bool res;
      if (op == "<") {
        res = real_l < real_r;
      } else if (op == "<=") {
        res = real_l <= real_r;
      } else if (op == ">") {
        res = real_l > real_r;
      } else if (op == ">=") {
        res = real_l >= real_r;
      } else if (op == "=") {
        res = real_l == real_r;
      } else if (op == "/=") {
        res = real_l != real_r;
      } else {
        // ERROR
        exit(1);
      }

      CNode *resultNode = new CNode("boolean");
      if (res) {
        resultNode->children.push_back(new CNode("true"));
      } else {
        resultNode->children.push_back(new CNode("false"));
      }
      return resultNode;
    }
    return node->children[0];

  } else if (node->name == "simple") {
    res_node = calculate(node->children[0]);

    if (res_node != node->children[0])
      changeChild(node->children[0], res_node);
    if (node->children.size() == 3) {
      auto second_node = calculate(node->children[2]);
      if (second_node != node->children[2])
        changeChild(node->children[2], second_node);

      if (!(res_node->name == "integer" || res_node->name == "real")) {
        if (res_node->name == "boolean") {
          std::cerr << "Cannot use arithmetic operations with boolean"
                    << std::endl;
          exit(1);
        }
        return node;
      }

      if (!(second_node->name == "integer" || second_node->name == "real")) {
        if (second_node->name == "boolean") {
          std::cerr << "Cannot use arithmetic operations with boolean"
                    << std::endl;
          exit(1);
        }
        return node;
      }

      std::string op = node->children[1]->name;
      if (res_node->name == "integer" && second_node->name == "integer") {
        int l = toInteger(res_node);
        int r = toInteger(second_node);
        int res = 0;
        if (op == "/") {
          if (r == 0) {
            std::cerr << "Сannot be divided by zero" << std::endl;
            exit(1);
          }
          res = l / r;
        } else if (op == "*") {
          res = l * r;
        } else if (op == "%") {
          if (r == 0) {
            std::cerr << "Сannot be divided by zero" << std::endl;
            exit(1);
          }
          res = l % r;
        }

        CNode *resultNode = new CNode("integer");
        resultNode->children.push_back(new CNode(std::to_string(res)));
        return resultNode;
      } else {
        double l = toReal(res_node);
        double r = toReal(second_node);

        double res = 0;
        if (op == "/") {
          if (r == 0) {
            std::cerr << "Сannot be divided by zero" << std::endl;
            exit(1);
          }
          res = l / r;
        } else if (op == "*") {
          res = l * r;
        } else if (op == "%") {
          std::cerr << "Not mod operation for real numbers" << std::endl;
          exit(1);
        }

        CNode *resultNode = new CNode("real");
        resultNode->children.push_back(new CNode(std::to_string(res)));
        return resultNode;
      }
    }
    return node->children[0];
  } else if (node->name == "not_factor") {
    auto res = calculate(node->children[1]);
    if (res != node->children[1])
      changeChild(node->children[1], res);

    if (res->name == "real") {
      std::cerr << "Real " << res->children[0]->name
                << " cannot be converted to boolean" << std::endl;
      exit(1);
    }

    if (!(res->name == "integer" || res->name == "boolean")) {
      return node;
    }

    bool real_a = toBoolean(res);

    real_a = !real_a;

    CNode *resultNode = new CNode("boolean");
    if (real_a) {
      resultNode->children.push_back(new CNode("true"));
    } else {
      resultNode->children.push_back(new CNode("false"));
    }
    return resultNode;
  } else if (node->name == "unary_factor") {
    auto res = calculate(node->children[1]);
    if (res != node->children[1])
      changeChild(node->children[1], res);

    if (res->name == "boolean") {
      std::cerr << "Cannot use unary signs with Boolean: "
                << res->children[0]->name << std::endl;
      exit(1);
    }

    std::string op = node->children[0]->name;
    std::string a = res->children[0]->name;
    if (res->name == "integer") {
      if (op == "+") {
        return res;
      }
      int result = -std::stoi(a);
      CNode *resultNode = new CNode("integer");
      resultNode->children.push_back(new CNode(std::to_string(result)));
      return resultNode;
    } else if (res->name == "real") {
      if (op == "+") {
        return res;
      }
      double result = -std::stod(a);
      CNode *resultNode = new CNode("real");
      resultNode->children.push_back(new CNode(std::to_string(result)));
      return resultNode;
    } else {
      return node;
    }

  } else if (node->name == "factor") {
    res_node = calculate(node->children[0]);
    if (res_node != node->children[0])
      changeChild(node->children[0], res_node);

    if (node->children.size() == 3) {
      auto second_node = calculate(node->children[2]);
      if (second_node != node->children[2])
        changeChild(node->children[2], second_node);

      if (!(res_node->name == "integer" || res_node->name == "real")) {
        if (res_node->name == "boolean") {
          std::cerr << "Cannot use arithmetic operations with boolean"
                    << std::endl;
          exit(1);
        }
        return node;
      }

      if (!(second_node->name == "integer" || second_node->name == "real")) {
        if (second_node->name == "boolean") {
          std::cerr << "Cannot use arithmetic operations with boolean"
                    << std::endl;
          exit(1);
        }
        return node;
      }

      if (res_node->name == "integer" && second_node->name == "integer") {
        int real_l = toInteger(res_node);
        int real_r = toInteger(second_node);

        std::string op = node->children[1]->name;
        int res;
        if (op == "+") {
          res = real_l + real_r;
        } else if (op == "-") {
          res = real_l - real_r;
        } else {
          // ERROR
          exit(1);
        }

        CNode *resultNode = new CNode("integer");
        resultNode->children.push_back(new CNode(std::to_string(res)));
        return resultNode;
      } else {
        double real_l = toReal(res_node);
        double real_r = toReal(second_node);

        std::string op = node->children[1]->name;
        double res;
        if (op == "+") {
          res = real_l + real_r;
        } else if (op == "-") {
          res = real_l - real_r;
        } else {
          // ERROR
          exit(1);
        }

        CNode *resultNode = new CNode("real");
        resultNode->children.push_back(new CNode(std::to_string(res)));
        return resultNode;
      }
    }
    return node->children[0];
  } else if (node->name == "summand") {
    return calculate(node->children[0]);

  } else if (node->name == "integer" || node->name == "boolean" ||
             node->name == "real") {
    return node;
  } else if (node->name == "modifiable_primary" ||
             node->name == "modifiable_primary_array" ||
             node->name == "modifiable_primary_field") {
    if (!check_modifiable(node)) {
      return nullptr;
    }
    return node;
  }
  return nullptr;
}

bool ControlTable::processingExpression(CNode *&parent, int idChild) {
  CNode *res = calculate(parent->children[idChild]);
  if (res == nullptr)
    return false;
  if (res != parent->children[idChild])
    changeChild(parent->children[idChild], res);
  return true;
}

int ControlTable::countVariables(bool itself) const {
  int res = 0;
  if (!parent_.expired()) {
    res += parent_.lock()->countVariables(true);
  }
  res += symbol_table_->getCountVariables();
  if (itself) {
    return res;
  }
  for (const auto &scope : sub_scopes_) {
    res += scope.second->countVariables(false);
  }
  return res;
}
GeneratorType ControlTable::getGeneratedType(std::string name) {
  auto variableNode = getVariable(name);
  if (variableNode == nullptr)
    return Error;
  auto typeNode = variableNode->variable_type_;
  if (typeNode->getType() == Types ::Simple) {
    std::string typeName =
        std::dynamic_pointer_cast<SimpleType>(typeNode)->name;
    if (typeName == "boolean")
      return Boolean;
    if (typeName == "integer")
      return Integer;
    if (typeName == "real")
      return Real;
  }

  if (typeNode->getType() == Types ::Array) {
    std::string typeName =
        std::dynamic_pointer_cast<SimpleType>(
            std::dynamic_pointer_cast<ArrayType>(typeNode)->arrayType)
            ->name;
    if (typeName == "boolean")
      return Arr_Boolean;
    if (typeName == "integer")
      return Arr_Integer;
    if (typeName == "real")
      return Arr_Real;
  }

  return Error;
}
