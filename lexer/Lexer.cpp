#include "Lexer.hpp"
#include "../yacc.hpp"

Lexer::Lexer(std::string src) {
  this->src = src;
  this->src_iter = this->src.begin();
  this->column = 0;
  createSymbolTable();
}

Token Lexer::next() {
  Token token(0, "");
  while (src_iter != src.end()) {
    if (*src_iter == ' ' || *src_iter == '\t' || *src_iter == '\n') {
      count(std::string(1, *src_iter));
      // Skip spaces
    } else if (isDigit(*src_iter)) {
      token = parseNumber();
      break;
    } else if (isLetter(*src_iter)) {
      token = parseIdentifier();
      break;
    } else if (isOtherSymbol(*src_iter)) {
      token = parseOtherSymbol();
      break;
    }

    src_iter++;
  }
  count(token.value);
  return token;
}

void Lexer::count(std::string str) {
  int i;

  for (i = 0; i < str.size(); i++)
    if (str[i] == '\n')
      column = 0;
    else if (str[i] == '\t')
      column += 8 - (column % 8);
    else
      column++;

  std::cout << str;
}

void Lexer::createSymbolTable() {
  // Feel the table keywords
  std::pair<std::string, int> keywords[] = {
      {"var", VAR},         {"is", IS},       {"type", TYPE},
      {"routine", ROUTINE}, {"end", END},     {"record", RECORD},
      {"array", ARRAY},     {"while", WHILE}, {"loop", LOOP},
      {"for", FOR},         {"in", IN},       {"reverse", REVERSE},
      {"return", RETURN},   {"if", IF},       {"then", THEN},
      {"else", ELSE},       {"and", AND},     {"or", OR},
      {"xor", XOR}, {"integer", INTEGER},     {"real", REAL},
      {"boolean", BOOLEAN},
  };

  for (auto keyword : keywords)
    symbol_table.insert(keyword.first, keyword.second);
}

bool Lexer::isDigit(char c) { return c >= '0' && c <= '9'; }

bool Lexer::isLetter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::isOtherSymbol(char c) {
  return (c >= '(' && c <= '/') || (c >= ':' && c <= '>') ||
  (c == '[') || (c = ']') || (c == '{') || (c == '}') || (c == '%');
}

Token Lexer::parseNumber() {
  Token token;
  std::string value = "";

  // Skip zeros
  while (src_iter != src.end() && *src_iter == '0') {
    src_iter++;
  }

  while (true) {
    if (isDigit(*src_iter)) {
      value += *src_iter;
    } else if (*src_iter == '.') {
      value += *src_iter;
      src_iter++;

      while (src_iter != src.end() && isDigit(*src_iter)) {
        value += *src_iter;
        src_iter++;
      }
      token.class_name = REAL_LITERAL;
      token.value = value;
      return token;
    } else {
      break;
    }
    src_iter++;
  }
  if (value == "")
    value = "0";

  token.class_name = INTEGER_LITERAL;
  token.value = value;
  return token;
}

Token Lexer::parseIdentifier() {
  Token token;
  std::string value = "";

  while (src_iter != src.end() && (isLetter(*src_iter) || isDigit(*src_iter))) {
    value += *src_iter;
    src_iter++;
  }

  int class_name = symbol_table.find(value);

  if (class_name == -1) {
    class_name = IDENTIFIER;
    symbol_table.insert(value, IDENTIFIER);
  }

  token.value = value;
  token.class_name = class_name;

  return token;
}

Token Lexer::parseOtherSymbol() {
  Token token = Token(-1, "");

  if (*src_iter == '+') {
    token.value = "+";
    token.class_name = PLUS_SIGN;
  } else if (*src_iter == '-') {
    token.value = "-";
    token.class_name = MINUS_SIGN;
  } else if (*src_iter == '*') {
    token.value = "*";
    token.class_name = MULT_SIGN;
  } else if (*src_iter == '/') {
    token.value = "/";
    token.class_name = DIV_SIGN;
  } else if (*src_iter == '%') {
    token.value = "%";
    token.class_name = MOD_SIGN;
  } else if (*src_iter == '=') {
    token.value = '=';
    token.class_name = EQ_SIGN;
  } else if (*src_iter == '<') {
    if (src_iter + 1 != src.end() && *(src_iter + 1) == '=') {
      src_iter++;
      token.value = "<=";
      token.class_name = LET_SIGN;
    } else {
      token.value = "<";
      token.class_name = LT_SIGN;
    }
  } else if (*src_iter == '>') {
    if (src_iter + 1 != src.end() && *(src_iter + 1) == '=') {
      src_iter++;
      token.value = ">=";
      token.class_name = GET_SIGN;
    } else {
      token.value = ">";
      token.class_name = GT_SIGN;
    }
  } else if (*src_iter == '[') {
    token.value = "[";
    token.class_name = L_SQ_BR;
  } else if (*src_iter == '(') {
    token.value = "(";
    token.class_name = L_BR;
  } else if (*src_iter == ']') {
    token.value = "]";
    token.class_name = R_SQ_BR;
  } else if (*src_iter == ')') {
    token.value = ")";
    token.class_name = R_BR;
  } else if (*src_iter == ':') {
    if (src_iter + 1 != src.end() && *(src_iter + 1) == '=') {
      src_iter++;
      token.value = ":=";
      token.class_name = ASSIGNMENT_SIGN;
    } else {
      token.value = ":";
      token.class_name = COLON;
    }
  } else if (*src_iter == ',') {
    token.value = ",";
    token.class_name = COMMA;
  } else if (*src_iter == '.') {
    token.value = ".";
    token.class_name = DOT;
  }
  src_iter++;
  return token;
}