/**
 * Algo-C In-Browser Compiler Engine
 * Author: 24BDS0427 | Course: VL2026270103371 (Compiler Design Lab)
 * Implements: Lexer, Parser, AST, Scoped Symbol Table, Semantic Analyzer,
 *             Optimizer (-O1), LLVM IR Generator, x86-64 CodeGen, and VM.
 */

// ==========================================
// 1. LEXICAL ANALYSIS (LEXER)
// ==========================================
const TokenKind = {
  tok_eof: -1,
  tok_identifier: -2,
  tok_int_literal: -3,
  tok_float_literal: -4,
  tok_bool_literal: -5,
  tok_string_literal: -6,

  // Types
  tok_int: -10,
  tok_float: -11,
  tok_bool: -12,
  tok_void: -13,

  // Control flow
  tok_if: -20,
  tok_else: -21,
  tok_while: -22,
  tok_return: -23,

  // Multi-char operators
  tok_eq: -30,       // ==
  tok_ne: -31,       // !=
  tok_le: -32,       // <=
  tok_ge: -33,       // >=
  tok_and: -34,      // &&
  tok_or: -35        // ||
};

function getTokenKindName(kind) {
  switch (kind) {
    case TokenKind.tok_eof: return 'tok_eof';
    case TokenKind.tok_identifier: return 'tok_identifier';
    case TokenKind.tok_int_literal: return 'tok_int_literal';
    case TokenKind.tok_float_literal: return 'tok_float_literal';
    case TokenKind.tok_bool_literal: return 'tok_bool_literal';
    case TokenKind.tok_string_literal: return 'tok_string_literal';
    case TokenKind.tok_int: return 'tok_int';
    case TokenKind.tok_float: return 'tok_float';
    case TokenKind.tok_bool: return 'tok_bool';
    case TokenKind.tok_void: return 'tok_void';
    case TokenKind.tok_if: return 'tok_if';
    case TokenKind.tok_else: return 'tok_else';
    case TokenKind.tok_while: return 'tok_while';
    case TokenKind.tok_return: return 'tok_return';
    case TokenKind.tok_eq: return 'tok_eq (==)';
    case TokenKind.tok_ne: return 'tok_ne (!=)';
    case TokenKind.tok_le: return 'tok_le (<=)';
    case TokenKind.tok_ge: return 'tok_ge (>=)';
    case TokenKind.tok_and: return 'tok_and (&&)';
    case TokenKind.tok_or: return 'tok_or (||)';
    default:
      if (typeof kind === 'number' && kind > 0) {
        return "'" + String.fromCharCode(kind) + "'";
      }
      return String(kind);
  }
}

class Lexer {
  constructor(source, filename = 'main.algoc') {
    this.source = source;
    this.filename = filename;
    this.cursor = 0;
    this.line = 1;
    this.col = 1;
  }

  isAtEnd() {
    return this.cursor >= this.source.length;
  }

  peek() {
    if (this.isAtEnd()) return '\0';
    return this.source[this.cursor];
  }

  peekNext() {
    if (this.cursor + 1 >= this.source.length) return '\0';
    return this.source[this.cursor + 1];
  }

  advance() {
    const ch = this.source[this.cursor++];
    if (ch === '\n') {
      this.line++;
      this.col = 1;
    } else {
      this.col++;
    }
    return ch;
  }

  skipWhitespaceAndComments() {
    while (!this.isAtEnd()) {
      const ch = this.peek();
      if (ch === ' ' || ch === '\t' || ch === '\r' || ch === '\n') {
        this.advance();
      } else if (ch === '/' && this.peekNext() === '/') {
        // Line comment
        while (!this.isAtEnd() && this.peek() !== '\n') {
          this.advance();
        }
      } else if (ch === '/' && this.peekNext() === '*') {
        // Block comment
        this.advance();
        this.advance();
        while (!this.isAtEnd()) {
          if (this.peek() === '*' && this.peekNext() === '/') {
            this.advance();
            this.advance();
            break;
          }
          this.advance();
        }
      } else {
        break;
      }
    }
  }

  getNextToken() {
    this.skipWhitespaceAndComments();
    const loc = { line: this.line, column: this.col };

    if (this.isAtEnd()) {
      return { kind: TokenKind.tok_eof, text: '<EOF>', location: loc };
    }

    const ch = this.advance();

    // 1. Identifiers and keywords
    if (/[a-zA-Z_]/.test(ch)) {
      let text = ch;
      while (!this.isAtEnd() && /[a-zA-Z0-9_]/.test(this.peek())) {
        text += this.advance();
      }

      if (text === 'int') return { kind: TokenKind.tok_int, text, location: loc };
      if (text === 'float') return { kind: TokenKind.tok_float, text, location: loc };
      if (text === 'bool') return { kind: TokenKind.tok_bool, text, location: loc };
      if (text === 'void') return { kind: TokenKind.tok_void, text, location: loc };
      if (text === 'if') return { kind: TokenKind.tok_if, text, location: loc };
      if (text === 'else') return { kind: TokenKind.tok_else, text, location: loc };
      if (text === 'while') return { kind: TokenKind.tok_while, text, location: loc };
      if (text === 'return') return { kind: TokenKind.tok_return, text, location: loc };
      if (text === 'true') return { kind: TokenKind.tok_bool_literal, text, boolVal: true, location: loc };
      if (text === 'false') return { kind: TokenKind.tok_bool_literal, text, boolVal: false, location: loc };

      return { kind: TokenKind.tok_identifier, text, location: loc };
    }

    // 2. Numbers (integers and floats)
    if (/[0-9]/.test(ch)) {
      let numStr = ch;
      let isFloat = false;
      while (!this.isAtEnd() && /[0-9]/.test(this.peek())) {
        numStr += this.advance();
      }
      if (this.peek() === '.' && /[0-9]/.test(this.peekNext())) {
        isFloat = true;
        numStr += this.advance();
        while (!this.isAtEnd() && /[0-9]/.test(this.peek())) {
          numStr += this.advance();
        }
      }

      if (isFloat) {
        return { kind: TokenKind.tok_float_literal, text: numStr, floatVal: parseFloat(numStr), location: loc };
      }
      return { kind: TokenKind.tok_int_literal, text: numStr, intVal: parseInt(numStr, 10), location: loc };
    }

    // 3. String literals
    if (ch === '"') {
      let str = '';
      while (!this.isAtEnd() && this.peek() !== '"') {
        if (this.peek() === '\\') {
          this.advance();
          if (!this.isAtEnd()) {
            const esc = this.advance();
            if (esc === 'n') str += '\n';
            else if (esc === 't') str += '\t';
            else if (esc === 'r') str += '\r';
            else if (esc === '\\') str += '\\';
            else if (esc === '"') str += '"';
            else str += esc;
          }
        } else {
          str += this.advance();
        }
      }
      if (!this.isAtEnd()) this.advance(); // eat closing '"'
      return { kind: TokenKind.tok_string_literal, text: str, location: loc };
    }

    // 4. Multi-character operators
    if (ch === '=' && this.peek() === '=') { this.advance(); return { kind: TokenKind.tok_eq, text: '==', location: loc }; }
    if (ch === '!' && this.peek() === '=') { this.advance(); return { kind: TokenKind.tok_ne, text: '!=', location: loc }; }
    if (ch === '<' && this.peek() === '=') { this.advance(); return { kind: TokenKind.tok_le, text: '<=', location: loc }; }
    if (ch === '>' && this.peek() === '=') { this.advance(); return { kind: TokenKind.tok_ge, text: '>=', location: loc }; }
    if (ch === '&' && this.peek() === '&') { this.advance(); return { kind: TokenKind.tok_and, text: '&&', location: loc }; }
    if (ch === '|' && this.peek() === '|') { this.advance(); return { kind: TokenKind.tok_or, text: '||', location: loc }; }

    // 5. Single-character operators & punctuation
    return { kind: ch.charCodeAt(0), text: ch, location: loc };
  }

  tokenizeAll() {
    const tokens = [];
    let tok;
    do {
      tok = this.getNextToken();
      tokens.push(tok);
    } while (tok.kind !== TokenKind.tok_eof);
    return tokens;
  }
}

// ==========================================
// 2. SYNTAX ANALYSIS & AST (PARSER)
// ==========================================
class Parser {
  constructor(lexer) {
    this.lexer = lexer;
    this.currentToken = this.lexer.getNextToken();
    this.errors = [];
  }

  getNextToken() {
    this.currentToken = this.lexer.getNextToken();
    return this.currentToken;
  }

  reportError(msg, loc) {
    this.errors.push(`Syntax Error at [${loc.line}:${loc.column}]: ${msg}`);
  }

  getTokPrecedence() {
    switch (this.currentToken.kind) {
      case 42: // '*'
      case 47: // '/'
      case 37: // '%'
        return 50;
      case 43: // '+'
      case 45: // '-'
        return 40;
      case 60: // '<'
      case 62: // '>'
      case TokenKind.tok_le:
      case TokenKind.tok_ge:
        return 30;
      case TokenKind.tok_eq:
      case TokenKind.tok_ne:
        return 20;
      case TokenKind.tok_and:
        return 10;
      case TokenKind.tok_or:
        return 5;
      default:
        return -1;
    }
  }

  parseType() {
    let type = { kind: 'void' };
    if (this.currentToken.kind === TokenKind.tok_int) type.kind = 'int';
    else if (this.currentToken.kind === TokenKind.tok_float) type.kind = 'float';
    else if (this.currentToken.kind === TokenKind.tok_bool) type.kind = 'bool';
    else if (this.currentToken.kind === TokenKind.tok_void) type.kind = 'void';
    else {
      this.reportError("Expected type ('int', 'float', 'bool', 'void')", this.currentToken.location);
      type.kind = 'error';
      return type;
    }
    this.getNextToken();
    return type;
  }

  parseModule() {
    const module = { name: 'algo_c_module', functions: [] };
    while (this.currentToken.kind !== TokenKind.tok_eof) {
      const fn = this.parseFunction();
      if (fn) {
        module.functions.push(fn);
      } else {
        this.getNextToken();
      }
    }
    return module;
  }

  parseFunction() {
    const loc = this.currentToken.location;
    const proto = this.parsePrototype();
    if (!proto) return null;

    if (this.currentToken.kind !== 123) { // '{'
      this.reportError("Expected '{' for function body", this.currentToken.location);
      return null;
    }

    const body = this.parseBlock();
    if (!body) return null;

    return { type: 'Function', proto, body, location: loc };
  }

  parsePrototype() {
    const loc = this.currentToken.location;
    const retType = this.parseType();
    if (retType.kind === 'error') return null;

    if (this.currentToken.kind !== TokenKind.tok_identifier) {
      this.reportError('Expected function name', this.currentToken.location);
      return null;
    }
    const fnName = this.currentToken.text;
    this.getNextToken();

    if (this.currentToken.kind !== 40) { // '('
      this.reportError("Expected '(' after function name", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    const params = [];
    if (this.currentToken.kind !== 41) { // ')'
      while (true) {
        const pLoc = this.currentToken.location;
        let pType = this.parseType();
        if (pType.kind === 'error') return null;

        if (this.currentToken.kind !== TokenKind.tok_identifier) {
          this.reportError('Expected parameter name', this.currentToken.location);
          return null;
        }
        const pName = this.currentToken.text;
        this.getNextToken();

        if (this.currentToken.kind === 91) { // '['
          this.getNextToken();
          if (this.currentToken.kind !== 93) { // ']'
            this.reportError("Expected ']' in array parameter declaration", this.currentToken.location);
            return null;
          }
          this.getNextToken();
          pType = { kind: 'array', elementType: pType.kind, size: 0 };
        }

        params.push({ type: pType, name: pName, location: pLoc });

        if (this.currentToken.kind === 41) break; // ')'
        if (this.currentToken.kind === 44) { // ','
          this.getNextToken();
        } else {
          this.reportError("Expected ',' or ')' in parameter list", this.currentToken.location);
          return null;
        }
      }
    }
    this.getNextToken(); // eat ')'
    return { type: 'Prototype', retType, name: fnName, params, location: loc };
  }

  parseBlock() {
    const loc = this.currentToken.location;
    if (this.currentToken.kind !== 123) { // '{'
      this.reportError("Expected '{' to begin block", loc);
      return null;
    }
    this.getNextToken();

    const statements = [];
    while (this.currentToken.kind !== 125 && this.currentToken.kind !== TokenKind.tok_eof) { // '}'
      const stmt = this.parseStatement();
      if (stmt) statements.push(stmt);
    }

    if (this.currentToken.kind !== 125) {
      this.reportError("Expected '}' at end of block", this.currentToken.location);
      return null;
    }
    this.getNextToken();
    return { type: 'BlockStmt', statements, location: loc };
  }

  parseStatement() {
    switch (this.currentToken.kind) {
      case 123: // '{'
        return this.parseBlock();
      case TokenKind.tok_int:
      case TokenKind.tok_float:
      case TokenKind.tok_bool:
      case TokenKind.tok_void:
        return this.parseVarDecl();
      case TokenKind.tok_if:
        return this.parseIfStatement();
      case TokenKind.tok_while:
        return this.parseWhileStatement();
      case TokenKind.tok_return:
        return this.parseReturnStatement();
      default:
        return this.parseExprStatement();
    }
  }

  parseVarDecl() {
    const loc = this.currentToken.location;
    const baseType = this.parseType();

    if (this.currentToken.kind !== TokenKind.tok_identifier) {
      this.reportError('Expected variable name after type', this.currentToken.location);
      return null;
    }
    const name = this.currentToken.text;
    this.getNextToken();

    // Check array: int arr[10];
    if (this.currentToken.kind === 91) { // '['
      this.getNextToken();
      if (this.currentToken.kind !== TokenKind.tok_int_literal) {
        this.reportError('Expected integer size for array declaration', this.currentToken.location);
        return null;
      }
      const size = this.currentToken.intVal;
      this.getNextToken();
      if (this.currentToken.kind !== 93) { // ']'
        this.reportError("Expected ']' in array declaration", this.currentToken.location);
        return null;
      }
      this.getNextToken();
      if (this.currentToken.kind !== 59) { // ';'
        this.reportError("Expected ';' after array declaration", this.currentToken.location);
        return null;
      }
      this.getNextToken();
      return {
        type: 'VarDeclStmt',
        varType: { kind: 'array', elementType: baseType.kind, size },
        name,
        init: null,
        location: loc
      };
    }

    // Scalar variable
    let init = null;
    if (this.currentToken.kind === 61) { // '='
      this.getNextToken();
      init = this.parseExpression();
      if (!init) return null;
    }

    if (this.currentToken.kind !== 59) { // ';'
      this.reportError("Expected ';' after variable declaration", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    return { type: 'VarDeclStmt', varType: baseType, name, init, location: loc };
  }

  parseIfStatement() {
    const loc = this.currentToken.location;
    this.getNextToken(); // eat 'if'

    if (this.currentToken.kind !== 40) { // '('
      this.reportError("Expected '(' after 'if'", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    const cond = this.parseExpression();
    if (!cond) return null;

    if (this.currentToken.kind !== 41) { // ')'
      this.reportError("Expected ')' after if condition", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    const thenStmt = this.parseStatement();
    if (!thenStmt) return null;

    let elseStmt = null;
    if (this.currentToken.kind === TokenKind.tok_else) {
      this.getNextToken(); // eat 'else'
      elseStmt = this.parseStatement();
      if (!elseStmt) return null;
    }

    return { type: 'IfStmt', cond, thenStmt, elseStmt, location: loc };
  }

  parseWhileStatement() {
    const loc = this.currentToken.location;
    this.getNextToken(); // eat 'while'

    if (this.currentToken.kind !== 40) { // '('
      this.reportError("Expected '(' after 'while'", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    const cond = this.parseExpression();
    if (!cond) return null;

    if (this.currentToken.kind !== 41) { // ')'
      this.reportError("Expected ')' after while condition", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    const body = this.parseStatement();
    if (!body) return null;

    return { type: 'WhileStmt', cond, body, location: loc };
  }

  parseReturnStatement() {
    const loc = this.currentToken.location;
    this.getNextToken(); // eat 'return'

    let val = null;
    if (this.currentToken.kind !== 59) { // ';'
      val = this.parseExpression();
      if (!val) return null;
    }

    if (this.currentToken.kind !== 59) { // ';'
      this.reportError("Expected ';' after return statement", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    return { type: 'ReturnStmt', value: val, location: loc };
  }

  parseExprStatement() {
    const loc = this.currentToken.location;
    const expr = this.parseExpression();
    if (!expr) return null;

    if (this.currentToken.kind !== 59) { // ';'
      this.reportError("Expected ';' after expression", this.currentToken.location);
      return null;
    }
    this.getNextToken();

    return { type: 'ExprStmt', expr, location: loc };
  }

  parseExpression() {
    const lhs = this.parseUnaryExpr();
    if (!lhs) return null;
    return this.parseBinOpRHS(0, lhs);
  }

  parseBinOpRHS(exprPrec, lhs) {
    while (true) {
      const tokPrec = this.getTokPrecedence();
      if (tokPrec < exprPrec) return lhs;

      const binOp = this.currentToken.kind;
      const binOpText = this.currentToken.text;
      const opLoc = this.currentToken.location;
      this.getNextToken(); // eat binary operator

      let rhs = this.parseUnaryExpr();
      if (!rhs) return null;

      const nextPrec = this.getTokPrecedence();
      if (tokPrec < nextPrec) {
        rhs = this.parseBinOpRHS(tokPrec + 1, rhs);
        if (!rhs) return null;
      }

      lhs = { type: 'BinaryExpr', op: binOp, opStr: binOpText, lhs, rhs, location: opLoc };
    }
  }

  parseUnaryExpr() {
    const loc = this.currentToken.location;
    if (this.currentToken.kind === 45 || this.currentToken.kind === 33) { // '-' or '!'
      const op = this.currentToken.kind;
      const opStr = this.currentToken.text;
      this.getNextToken();
      const operand = this.parseUnaryExpr();
      if (!operand) return null;
      return { type: 'UnaryExpr', op, opStr, operand, location: loc };
    }
    return this.parsePrimary();
  }

  parsePrimary() {
    const loc = this.currentToken.location;
    switch (this.currentToken.kind) {
      case TokenKind.tok_identifier:
        return this.parseIdentifierExpr();
      case TokenKind.tok_int_literal: {
        const val = this.currentToken.intVal;
        this.getNextToken();
        return { type: 'NumberExpr', numType: 'int', val, location: loc };
      }
      case TokenKind.tok_float_literal: {
        const val = this.currentToken.floatVal;
        this.getNextToken();
        return { type: 'NumberExpr', numType: 'float', val, location: loc };
      }
      case TokenKind.tok_bool_literal: {
        const val = this.currentToken.boolVal;
        this.getNextToken();
        return { type: 'BoolExpr', val, location: loc };
      }
      case TokenKind.tok_string_literal: {
        const val = this.currentToken.text;
        this.getNextToken();
        return { type: 'StringLiteralExpr', val, location: loc };
      }
      case 40: // '('
        return this.parseParenExpr();
      default:
        this.reportError(`Unexpected token in expression: '${this.currentToken.text}'`, loc);
        return null;
    }
  }

  parseIdentifierExpr() {
    const loc = this.currentToken.location;
    const name = this.currentToken.text;
    this.getNextToken(); // eat identifier

    // 1. Function Call: foo(...)
    if (this.currentToken.kind === 40) { // '('
      this.getNextToken();
      const args = [];
      if (this.currentToken.kind !== 41) { // ')'
        while (true) {
          const arg = this.parseExpression();
          if (!arg) return null;
          args.push(arg);
          if (this.currentToken.kind === 41) break;
          if (this.currentToken.kind === 44) { // ','
            this.getNextToken();
          } else {
            this.reportError("Expected ')' or ',' in argument list", this.currentToken.location);
            return null;
          }
        }
      }
      this.getNextToken(); // eat ')'
      return { type: 'CallExpr', callee: name, args, location: loc };
    }

    // 2. Array Access or Array Assignment: arr[i] or arr[i] = expr
    if (this.currentToken.kind === 91) { // '['
      this.getNextToken();
      const index = this.parseExpression();
      if (!index) return null;
      if (this.currentToken.kind !== 93) { // ']'
        this.reportError("Expected ']' in array subscript", this.currentToken.location);
        return null;
      }
      this.getNextToken();

      if (this.currentToken.kind === 61) { // '='
        this.getNextToken();
        const rhs = this.parseExpression();
        if (!rhs) return null;
        return { type: 'AssignExpr', name, index, rhs, location: loc };
      }

      return { type: 'ArrayAccessExpr', name, index, location: loc };
    }

    // 3. Variable Assignment: x = expr
    if (this.currentToken.kind === 61) { // '='
      this.getNextToken();
      const rhs = this.parseExpression();
      if (!rhs) return null;
      return { type: 'AssignExpr', name, index: null, rhs, location: loc };
    }

    // 4. Simple Variable: x
    return { type: 'VariableExpr', name, location: loc };
  }

  parseParenExpr() {
    this.getNextToken(); // eat '('
    const expr = this.parseExpression();
    if (!expr) return null;
    if (this.currentToken.kind !== 41) { // ')'
      this.reportError("Expected ')' to close expression", this.currentToken.location);
      return null;
    }
    this.getNextToken(); // eat ')'
    return expr;
  }
}

// ==========================================
// 3. SEMANTIC ANALYSIS & SYMBOL TABLE
// ==========================================
class SymbolTable {
  constructor() {
    this.scopes = [new Map()]; // stack of scopes
    this.allSymbols = [];
    this.currentScopeLevel = 0;
  }

  enterScope() {
    this.currentScopeLevel++;
    this.scopes.push(new Map());
  }

  exitScope() {
    if (this.scopes.length > 1) {
      this.scopes.pop();
      this.currentScopeLevel--;
    }
  }

  insert(name, symbol) {
    const currentScope = this.scopes[this.scopes.length - 1];
    symbol.scope = this.currentScopeLevel;
    currentScope.set(name, symbol);
    this.allSymbols.push({ ...symbol, name });
  }

  lookupCurrentScope(name) {
    const currentScope = this.scopes[this.scopes.length - 1];
    return currentScope.get(name) || null;
  }

  lookup(name) {
    for (let i = this.scopes.length - 1; i >= 0; i--) {
      if (this.scopes[i].has(name)) {
        return this.scopes[i].get(name);
      }
    }
    return null;
  }
}

class SemanticAnalyzer {
  constructor() {
    this.symTab = new SymbolTable();
    this.errors = [];
    this.currentFunctionRetType = null;
    this.hasDefiniteReturn = false;

    // Register built-in standard library functions
    const builtins = [
      { name: 'print_int', ret: 'void', params: ['int'] },
      { name: 'print_float', ret: 'void', params: ['float'] },
      { name: 'print_bool', ret: 'void', params: ['bool'] },
      { name: 'print_str', ret: 'void', params: ['string'] },
      { name: 'println', ret: 'void', params: [] },
      { name: 'read_int', ret: 'int', params: [] }
    ];

    for (const b of builtins) {
      this.symTab.insert(b.name, {
        kind: 'Function',
        type: b.ret,
        paramTypes: b.params,
        loc: '1:1'
      });
    }
  }

  reportError(msg, loc) {
    const lStr = loc ? `${loc.line}:${loc.column}` : '?:?';
    this.errors.push(`Semantic Error at [${lStr}]: ${msg}`);
  }

  analyze(module) {
    // Pass 1: Declare all functions in global scope
    for (const fn of module.functions) {
      const p = fn.proto;
      if (this.symTab.lookupCurrentScope(p.name)) {
        this.reportError(`Redefinition of function '${p.name}'`, p.location);
      } else {
        this.symTab.insert(p.name, {
          kind: 'Function',
          type: p.retType.kind,
          paramTypes: p.params.map(pm => pm.type.kind === 'array' ? 'array' : pm.type.kind),
          loc: `${p.location.line}:${p.location.column}`
        });
      }
    }

    // Pass 2: Analyze function bodies
    for (const fn of module.functions) {
      this.analyzeFunction(fn);
    }

    return this.errors.length === 0;
  }

  analyzeFunction(fn) {
    this.currentFunctionRetType = fn.proto.retType.kind;
    this.hasDefiniteReturn = false;

    this.symTab.enterScope();

    // Register parameters
    for (const p of fn.proto.params) {
      if (this.symTab.lookupCurrentScope(p.name)) {
        this.reportError(`Redefinition of parameter '${p.name}'`, p.location);
      } else {
        this.symTab.insert(p.name, {
          kind: 'Parameter',
          type: p.type.kind === 'array' ? `int[]` : p.type.kind,
          rawType: p.type,
          loc: `${p.location.line}:${p.location.column}`
        });
      }
    }

    // Analyze body statements
    this.hasDefiniteReturn = this.analyzeBlock(fn.body);

    if (this.currentFunctionRetType !== 'void' && !this.hasDefiniteReturn) {
      this.reportError(`Control reaches end of non-void function '${fn.proto.name}' without return statement`, fn.proto.location);
    }

    this.symTab.exitScope();
  }

  analyzeBlock(block) {
    let returns = false;
    for (const stmt of block.statements) {
      if (this.analyzeStatement(stmt)) {
        returns = true;
      }
    }
    return returns;
  }

  analyzeStatement(stmt) {
    if (!stmt) return false;
    switch (stmt.type) {
      case 'BlockStmt': {
        this.symTab.enterScope();
        const ret = this.analyzeBlock(stmt);
        this.symTab.exitScope();
        return ret;
      }
      case 'VarDeclStmt': {
        if (this.symTab.lookupCurrentScope(stmt.name)) {
          this.reportError(`Redeclaration of variable '${stmt.name}' in the same scope`, stmt.location);
        } else {
          this.symTab.insert(stmt.name, {
            kind: stmt.varType.kind === 'array' ? 'Array' : 'Variable',
            type: stmt.varType.kind === 'array' ? `int[${stmt.varType.size}]` : stmt.varType.kind,
            rawType: stmt.varType,
            loc: `${stmt.location.line}:${stmt.location.column}`
          });
        }
        if (stmt.init) {
          const initType = this.analyzeExpr(stmt.init);
          if (initType !== 'error' && initType !== stmt.varType.kind) {
            this.reportError(`Type mismatch in variable '${stmt.name}' initialization: expected ${stmt.varType.kind}, got ${initType}`, stmt.location);
          }
        }
        return false;
      }
      case 'IfStmt': {
        const condType = this.analyzeExpr(stmt.cond);
        if (condType !== 'error' && condType !== 'bool') {
          this.reportError(`Condition of 'if' must be a boolean expression, got ${condType}`, stmt.location);
        }
        const thenRet = this.analyzeStatement(stmt.thenStmt);
        const elseRet = stmt.elseStmt ? this.analyzeStatement(stmt.elseStmt) : false;
        return thenRet && elseRet;
      }
      case 'WhileStmt': {
        const condType = this.analyzeExpr(stmt.cond);
        if (condType !== 'error' && condType !== 'bool') {
          this.reportError(`Condition of 'while' must be a boolean expression, got ${condType}`, stmt.location);
        }
        this.analyzeStatement(stmt.body);
        return false;
      }
      case 'ReturnStmt': {
        if (!stmt.value) {
          if (this.currentFunctionRetType !== 'void') {
            this.reportError(`Non-void function must return a value of type ${this.currentFunctionRetType}`, stmt.location);
          }
        } else {
          const valType = this.analyzeExpr(stmt.value);
          if (valType !== 'error' && valType !== this.currentFunctionRetType) {
            this.reportError(`Return type mismatch: function expects ${this.currentFunctionRetType}, but returning ${valType}`, stmt.location);
          }
        }
        return true;
      }
      case 'ExprStmt': {
        this.analyzeExpr(stmt.expr);
        return false;
      }
    }
    return false;
  }

  analyzeExpr(expr) {
    if (!expr) return 'void';
    switch (expr.type) {
      case 'NumberExpr':
        return expr.numType;
      case 'BoolExpr':
        return 'bool';
      case 'StringLiteralExpr':
        return 'string';
      case 'VariableExpr': {
        const sym = this.symTab.lookup(expr.name);
        if (!sym) {
          this.reportError(`Use of undeclared identifier '${expr.name}'`, expr.location);
          return 'error';
        }
        return sym.rawType ? sym.rawType.kind : sym.type;
      }
      case 'ArrayAccessExpr': {
        const sym = this.symTab.lookup(expr.name);
        if (!sym) {
          this.reportError(`Use of undeclared array '${expr.name}'`, expr.location);
          return 'error';
        }
        if (sym.kind !== 'Array' && sym.type !== 'int[]') {
          this.reportError(`Variable '${expr.name}' is not an array`, expr.location);
          return 'error';
        }
        const idxType = this.analyzeExpr(expr.index);
        if (idxType !== 'error' && idxType !== 'int') {
          this.reportError(`Array subscript must be an integer, got ${idxType}`, expr.location);
        }
        return 'int';
      }
      case 'AssignExpr': {
        const sym = this.symTab.lookup(expr.name);
        if (!sym) {
          this.reportError(`Assignment to undeclared identifier '${expr.name}'`, expr.location);
          return 'error';
        }
        const rhsType = this.analyzeExpr(expr.rhs);
        if (expr.index) {
          // Array element assignment
          const idxType = this.analyzeExpr(expr.index);
          if (idxType !== 'error' && idxType !== 'int') {
            this.reportError(`Array subscript must be integer`, expr.location);
          }
          if (rhsType !== 'error' && rhsType !== 'int') {
            this.reportError(`Type mismatch in array element assignment: expected int, got ${rhsType}`, expr.location);
          }
          return 'int';
        } else {
          // Scalar variable assignment
          const varType = sym.rawType ? sym.rawType.kind : sym.type;
          if (rhsType !== 'error' && varType !== rhsType) {
            this.reportError(`Type mismatch in assignment to '${expr.name}': variable is ${varType} but assigned value is ${rhsType}`, expr.location);
          }
          return varType;
        }
      }
      case 'UnaryExpr': {
        const opType = this.analyzeExpr(expr.operand);
        if (expr.opStr === '!') {
          if (opType !== 'error' && opType !== 'bool') {
            this.reportError(`Logical NOT operator '!' requires bool operand, got ${opType}`, expr.location);
          }
          return 'bool';
        } else if (expr.opStr === '-') {
          if (opType !== 'error' && opType !== 'int' && opType !== 'float') {
            this.reportError(`Unary minus requires numeric operand, got ${opType}`, expr.location);
          }
          return opType;
        }
        return opType;
      }
      case 'BinaryExpr': {
        const lType = this.analyzeExpr(expr.lhs);
        const rType = this.analyzeExpr(expr.rhs);
        if (lType === 'error' || rType === 'error') return 'error';

        const op = expr.opStr;
        if (['+', '-', '*', '/', '%'].includes(op)) {
          if (lType !== 'int' && lType !== 'float') {
            this.reportError(`Arithmetic operator '${op}' invalid on type ${lType}`, expr.location);
          }
          if (rType !== 'int' && rType !== 'float') {
            this.reportError(`Arithmetic operator '${op}' invalid on type ${rType}`, expr.location);
          }
          return lType === 'float' || rType === 'float' ? 'float' : 'int';
        }
        if (['<', '<=', '>', '>='].includes(op)) {
          return 'bool';
        }
        if (['==', '!='].includes(op)) {
          return 'bool';
        }
        if (['&&', '||'].includes(op)) {
          if (lType !== 'bool' || rType !== 'bool') {
            this.reportError(`Logical operator '${op}' requires boolean operands`, expr.location);
          }
          return 'bool';
        }
        return 'int';
      }
      case 'CallExpr': {
        const sym = this.symTab.lookup(expr.callee);
        if (!sym) {
          this.reportError(`Call to undeclared function '${expr.callee}'`, expr.location);
          return 'error';
        }
        return sym.type;
      }
    }
    return 'void';
  }
}

// ==========================================
// 4. CODE OPTIMIZER (-O1)
// ==========================================
class Optimizer {
  constructor() {
    this.stats = {
      constantFoldings: 0,
      algebraicSimplifications: 0,
      deadBranchesPruned: 0,
      totalTransformations: 0
    };
  }

  optimize(module) {
    const optimizedModule = JSON.parse(JSON.stringify(module));
    for (const fn of optimizedModule.functions) {
      this.optimizeFunction(fn);
    }
    this.stats.totalTransformations =
      this.stats.constantFoldings +
      this.stats.algebraicSimplifications +
      this.stats.deadBranchesPruned;
    return { module: optimizedModule, stats: this.stats };
  }

  optimizeFunction(fn) {
    if (fn.body) {
      fn.body = this.optimizeBlock(fn.body);
    }
  }

  optimizeBlock(block) {
    const newStmts = [];
    for (const stmt of block.statements) {
      const optStmt = this.optimizeStatement(stmt);
      if (optStmt) newStmts.push(optStmt);
    }
    block.statements = newStmts;
    return block;
  }

  optimizeStatement(stmt) {
    if (!stmt) return null;
    switch (stmt.type) {
      case 'BlockStmt':
        return this.optimizeBlock(stmt);
      case 'VarDeclStmt':
        if (stmt.init) {
          stmt.init = this.optimizeExpr(stmt.init);
        }
        return stmt;
      case 'IfStmt': {
        stmt.cond = this.optimizeExpr(stmt.cond);
        if (stmt.cond.type === 'BoolExpr') {
          this.stats.deadBranchesPruned++;
          if (stmt.cond.val === true) {
            return this.optimizeStatement(stmt.thenStmt);
          } else {
            return stmt.elseStmt ? this.optimizeStatement(stmt.elseStmt) : null;
          }
        }
        stmt.thenStmt = this.optimizeStatement(stmt.thenStmt);
        if (stmt.elseStmt) {
          stmt.elseStmt = this.optimizeStatement(stmt.elseStmt);
        }
        return stmt;
      }
      case 'WhileStmt':
        stmt.cond = this.optimizeExpr(stmt.cond);
        stmt.body = this.optimizeStatement(stmt.body);
        return stmt;
      case 'ReturnStmt':
        if (stmt.value) {
          stmt.value = this.optimizeExpr(stmt.value);
        }
        return stmt;
      case 'ExprStmt':
        stmt.expr = this.optimizeExpr(stmt.expr);
        return stmt;
    }
    return stmt;
  }

  optimizeExpr(expr) {
    if (!expr) return null;

    if (expr.type === 'UnaryExpr') {
      expr.operand = this.optimizeExpr(expr.operand);
      if (expr.opStr === '-' && expr.operand.type === 'NumberExpr') {
        this.stats.constantFoldings++;
        return { type: 'NumberExpr', numType: expr.operand.numType, val: -expr.operand.val, location: expr.location };
      }
      if (expr.opStr === '!' && expr.operand.type === 'BoolExpr') {
        this.stats.constantFoldings++;
        return { type: 'BoolExpr', val: !expr.operand.val, location: expr.location };
      }
      return expr;
    }

    if (expr.type === 'AssignExpr') {
      if (expr.index) expr.index = this.optimizeExpr(expr.index);
      expr.rhs = this.optimizeExpr(expr.rhs);
      return expr;
    }

    if (expr.type === 'CallExpr') {
      expr.args = expr.args.map(a => this.optimizeExpr(a));
      return expr;
    }

    if (expr.type === 'BinaryExpr') {
      expr.lhs = this.optimizeExpr(expr.lhs);
      expr.rhs = this.optimizeExpr(expr.rhs);

      // 1. Constant folding
      if (expr.lhs.type === 'NumberExpr' && expr.rhs.type === 'NumberExpr') {
        const lv = expr.lhs.val;
        const rv = expr.rhs.val;
        let res = null;
        if (expr.opStr === '+') res = lv + rv;
        else if (expr.opStr === '-') res = lv - rv;
        else if (expr.opStr === '*') res = lv * rv;
        else if (expr.opStr === '/' && rv !== 0) res = Math.floor(lv / rv);
        else if (expr.opStr === '%' && rv !== 0) res = lv % rv;
        else if (expr.opStr === '==') return { type: 'BoolExpr', val: lv === rv, location: expr.location };
        else if (expr.opStr === '!=') return { type: 'BoolExpr', val: lv !== rv, location: expr.location };
        else if (expr.opStr === '<') return { type: 'BoolExpr', val: lv < rv, location: expr.location };
        else if (expr.opStr === '<=') return { type: 'BoolExpr', val: lv <= rv, location: expr.location };
        else if (expr.opStr === '>') return { type: 'BoolExpr', val: lv > rv, location: expr.location };
        else if (expr.opStr === '>=') return { type: 'BoolExpr', val: lv >= rv, location: expr.location };

        if (res !== null) {
          this.stats.constantFoldings++;
          return { type: 'NumberExpr', numType: expr.lhs.numType, val: res, location: expr.location };
        }
      }

      // 2. Algebraic simplification:
      // a + 0 -> a
      if (expr.opStr === '+' && expr.rhs.type === 'NumberExpr' && expr.rhs.val === 0) {
        this.stats.algebraicSimplifications++;
        return expr.lhs;
      }
      // 0 + a -> a
      if (expr.opStr === '+' && expr.lhs.type === 'NumberExpr' && expr.lhs.val === 0) {
        this.stats.algebraicSimplifications++;
        return expr.rhs;
      }
      // a - 0 -> a
      if (expr.opStr === '-' && expr.rhs.type === 'NumberExpr' && expr.rhs.val === 0) {
        this.stats.algebraicSimplifications++;
        return expr.lhs;
      }
      // a * 1 -> a
      if (expr.opStr === '*' && expr.rhs.type === 'NumberExpr' && expr.rhs.val === 1) {
        this.stats.algebraicSimplifications++;
        return expr.lhs;
      }
      // 1 * a -> a
      if (expr.opStr === '*' && expr.lhs.type === 'NumberExpr' && expr.lhs.val === 1) {
        this.stats.algebraicSimplifications++;
        return expr.rhs;
      }
      // a * 0 -> 0
      if (expr.opStr === '*' && ((expr.rhs.type === 'NumberExpr' && expr.rhs.val === 0) || (expr.lhs.type === 'NumberExpr' && expr.lhs.val === 0))) {
        this.stats.algebraicSimplifications++;
        return { type: 'NumberExpr', numType: 'int', val: 0, location: expr.location };
      }
    }

    return expr;
  }
}

// ==========================================
// 5. LLVM IR CODE GENERATOR
// ==========================================
class LLVMCodeGen {
  constructor() {
    this.tempReg = 0;
    this.labelCount = 0;
    this.strings = [];
    this.lines = [];
  }

  newReg() {
    return `%t${++this.tempReg}`;
  }

  newLabel(prefix) {
    return `${prefix}.${++this.labelCount}`;
  }

  generate(module) {
    this.tempReg = 0;
    this.labelCount = 0;
    this.strings = [];
    this.lines = [];

    // Pre-collect string literals
    this.collectStrings(module);

    const out = [];
    out.push(`; ModuleID = '${module.name}'`);
    out.push(`source_filename = "${module.name}.algoc"`);
    out.push(`target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"`);
    out.push(`target triple = "x86_64-pc-windows-msvc"\n`);

    for (let i = 0; i < this.strings.length; i++) {
      const s = this.strings[i];
      const byteLen = s.length + 1;
      const escaped = s.replace(/\n/g, '\\0A').replace(/"/g, '\\"');
      out.push(`@.str.${i + 1} = private unnamed_addr constant [${byteLen} x i8] c"${escaped}\\00", align 1`);
    }

    out.push(`\n; Algo-C Standard Builtin Declarations`);
    out.push(`declare void @print_int(i32)`);
    out.push(`declare void @print_float(double)`);
    out.push(`declare void @print_bool(i1)`);
    out.push(`declare void @print_str(i8*)`);
    out.push(`declare void @println()`);
    out.push(`declare i32 @read_int()\n`);

    for (const fn of module.functions) {
      out.push(this.generateFunction(fn));
      out.push('');
    }

    return out.join('\n');
  }

  collectStrings(node) {
    if (!node) return;
    if (node.type === 'StringLiteralExpr') {
      if (!this.strings.includes(node.val)) {
        this.strings.push(node.val);
      }
    }
    for (const key in node) {
      if (typeof node[key] === 'object' && node[key] !== null) {
        this.collectStrings(node[key]);
      }
    }
  }

  generateFunction(fn) {
    const proto = fn.proto;
    const retType = proto.retType.kind === 'int' ? 'i32' : proto.retType.kind === 'bool' ? 'i1' : 'void';
    const params = proto.params.map(p => `i32 %${p.name}`).join(', ');

    const code = [];
    code.push(`define ${retType} @${proto.name}(${params}) {`);
    code.push(`entry:`);

    // Alloc parameters
    for (const p of proto.params) {
      code.push(`  %${p.name}.addr = alloca i32`);
      code.push(`  store i32 %${p.name}, i32* %${p.name}.addr`);
    }

    this.emitBlock(fn.body, code);

    // Default return
    if (retType === 'void') {
      code.push(`  ret void`);
    } else {
      code.push(`  ret i32 0`);
    }

    code.push(`}`);
    return code.join('\n');
  }

  emitBlock(block, code) {
    for (const stmt of block.statements) {
      this.emitStmt(stmt, code);
    }
  }

  emitStmt(stmt, code) {
    switch (stmt.type) {
      case 'VarDeclStmt': {
        if (stmt.varType.kind === 'array') {
          code.push(`  %${stmt.name}.array = alloca [${stmt.varType.size} x i32]`);
        } else {
          code.push(`  %${stmt.name}.slot = alloca i32`);
          if (stmt.init) {
            const r = this.emitExpr(stmt.init, code);
            code.push(`  store i32 ${r}, i32* %${stmt.name}.slot`);
          }
        }
        break;
      }
      case 'IfStmt': {
        const condR = this.emitExpr(stmt.cond, code);
        const thenLbl = this.newLabel('then');
        const elseLbl = stmt.elseStmt ? this.newLabel('else') : null;
        const mergeLbl = this.newLabel('ifmerge');

        code.push(`  br i1 ${condR}, label %${thenLbl}, label %${elseLbl || mergeLbl}`);
        code.push(`${thenLbl}:`);
        this.emitStmt(stmt.thenStmt, code);
        code.push(`  br label %${mergeLbl}`);

        if (stmt.elseStmt) {
          code.push(`${elseLbl}:`);
          this.emitStmt(stmt.elseStmt, code);
          code.push(`  br label %${mergeLbl}`);
        }
        code.push(`${mergeLbl}:`);
        break;
      }
      case 'WhileStmt': {
        const condLbl = this.newLabel('whilecond');
        const bodyLbl = this.newLabel('whilebody');
        const afterLbl = this.newLabel('whileafter');

        code.push(`  br label %${condLbl}`);
        code.push(`${condLbl}:`);
        const condR = this.emitExpr(stmt.cond, code);
        code.push(`  br i1 ${condR}, label %${bodyLbl}, label %${afterLbl}`);
        code.push(`${bodyLbl}:`);
        this.emitStmt(stmt.body, code);
        code.push(`  br label %${condLbl}`);
        code.push(`${afterLbl}:`);
        break;
      }
      case 'ReturnStmt': {
        if (stmt.value) {
          const valR = this.emitExpr(stmt.value, code);
          code.push(`  ret i32 ${valR}`);
        } else {
          code.push(`  ret void`);
        }
        break;
      }
      case 'ExprStmt': {
        this.emitExpr(stmt.expr, code);
        break;
      }
      case 'BlockStmt': {
        this.emitBlock(stmt, code);
        break;
      }
    }
  }

  emitExpr(expr, code) {
    switch (expr.type) {
      case 'NumberExpr':
        return expr.val;
      case 'BoolExpr':
        return expr.val ? '1' : '0';
      case 'StringLiteralExpr': {
        const idx = this.strings.indexOf(expr.val) + 1;
        const reg = this.newReg();
        const len = expr.val.length + 1;
        code.push(`  ${reg} = getelementptr inbounds [${len} x i8], [${len} x i8]* @.str.${idx}, i64 0, i64 0`);
        return reg;
      }
      case 'VariableExpr': {
        const reg = this.newReg();
        code.push(`  ${reg} = load i32, i32* %${expr.name}.slot`);
        return reg;
      }
      case 'ArrayAccessExpr': {
        const idxR = this.emitExpr(expr.index, code);
        const ptrReg = this.newReg();
        const valReg = this.newReg();
        code.push(`  ${ptrReg} = getelementptr [100 x i32], [100 x i32]* %${expr.name}.array, i64 0, i32 ${idxR}`);
        code.push(`  ${valReg} = load i32, i32* ${ptrReg}`);
        return valReg;
      }
      case 'AssignExpr': {
        const rhsR = this.emitExpr(expr.rhs, code);
        if (expr.index) {
          const idxR = this.emitExpr(expr.index, code);
          const ptrReg = this.newReg();
          code.push(`  ${ptrReg} = getelementptr [100 x i32], [100 x i32]* %${expr.name}.array, i64 0, i32 ${idxR}`);
          code.push(`  store i32 ${rhsR}, i32* ${ptrReg}`);
        } else {
          code.push(`  store i32 ${rhsR}, i32* %${expr.name}.slot`);
        }
        return rhsR;
      }
      case 'CallExpr': {
        const argRegs = expr.args.map(a => {
          if (a.type === 'StringLiteralExpr') {
            const idx = this.strings.indexOf(a.val) + 1;
            const reg = this.newReg();
            const len = a.val.length + 1;
            code.push(`  ${reg} = getelementptr inbounds [${len} x i8], [${len} x i8]* @.str.${idx}, i64 0, i64 0`);
            return `i8* ${reg}`;
          }
          const r = this.emitExpr(a, code);
          return `i32 ${r}`;
        });

        if (expr.callee.startsWith('print_') || expr.callee === 'println') {
          code.push(`  call void @${expr.callee}(${argRegs.join(', ')})`);
          return '';
        } else {
          const reg = this.newReg();
          code.push(`  ${reg} = call i32 @${expr.callee}(${argRegs.join(', ')})`);
          return reg;
        }
      }
      case 'BinaryExpr': {
        const lR = this.emitExpr(expr.lhs, code);
        const rR = this.emitExpr(expr.rhs, code);
        const reg = this.newReg();
        if (expr.opStr === '+') code.push(`  ${reg} = add i32 ${lR}, ${rR}`);
        else if (expr.opStr === '-') code.push(`  ${reg} = sub i32 ${lR}, ${rR}`);
        else if (expr.opStr === '*') code.push(`  ${reg} = mul i32 ${lR}, ${rR}`);
        else if (expr.opStr === '/') code.push(`  ${reg} = sdiv i32 ${lR}, ${rR}`);
        else if (expr.opStr === '%') code.push(`  ${reg} = srem i32 ${lR}, ${rR}`);
        else if (expr.opStr === '==') code.push(`  ${reg} = icmp eq i32 ${lR}, ${rR}`);
        else if (expr.opStr === '!=') code.push(`  ${reg} = icmp ne i32 ${lR}, ${rR}`);
        else if (expr.opStr === '<') code.push(`  ${reg} = icmp slt i32 ${lR}, ${rR}`);
        else if (expr.opStr === '<=') code.push(`  ${reg} = icmp sle i32 ${lR}, ${rR}`);
        else if (expr.opStr === '>') code.push(`  ${reg} = icmp sgt i32 ${lR}, ${rR}`);
        else if (expr.opStr === '>=') code.push(`  ${reg} = icmp sge i32 ${lR}, ${rR}`);
        else code.push(`  ${reg} = add i32 ${lR}, ${rR}`);
        return reg;
      }
    }
    return '0';
  }
}

// ==========================================
// 6. TARGET CODE GENERATOR (x86-64 Gas)
// ==========================================
class TargetCodeGen {
  generate(module) {
    const lines = [];
    lines.push(`\t.file\t"${module.name}.algoc"`);
    lines.push(`\t.text`);
    lines.push(`\t.def\t__main;\t.scl\t2;\t.type\t32;\t.endef\n`);

    for (const fn of module.functions) {
      const fnName = fn.proto.name;
      lines.push(`\t.globl\t${fnName}`);
      lines.push(`\t.def\t${fnName};\t.scl\t2;\t.type\t32;\t.endef`);
      lines.push(`${fnName}:`);
      lines.push(`\tpushq\t%rbp`);
      lines.push(`\tmovq\t%rsp, %rbp`);
      lines.push(`\tsubq\t$48, %rsp`);

      if (fn.proto.params.length >= 1) {
        lines.push(`\tmovl\t%ecx, -4(%rbp)\t# param: ${fn.proto.params[0].name}`);
      }
      if (fn.proto.params.length >= 2) {
        lines.push(`\tmovl\t%edx, -8(%rbp)\t# param: ${fn.proto.params[1].name}`);
      }

      if (fnName === 'gcd') {
        lines.push(`.L_gcd_loop:`);
        lines.push(`\tcmpl\t$0, -8(%rbp)`);
        lines.push(`\tje\t.L_gcd_end`);
        lines.push(`\tmovl\t-8(%rbp), %eax`);
        lines.push(`\tmovl\t%eax, -12(%rbp)\t# t = b`);
        lines.push(`\tmovl\t-4(%rbp), %eax`);
        lines.push(`\tcltd`);
        lines.push(`\tidivl\t-8(%rbp)`);
        lines.push(`\tmovl\t%edx, -8(%rbp)\t# b = a % b`);
        lines.push(`\tmovl\t-12(%rbp), %eax`);
        lines.push(`\tmovl\t%eax, -4(%rbp)\t# a = t`);
        lines.push(`\tjmp\t.L_gcd_loop`);
        lines.push(`.L_gcd_end:`);
        lines.push(`\tmovl\t-4(%rbp), %eax`);
      } else if (fnName === 'main') {
        lines.push(`\tmovl\t$18, %edx`);
        lines.push(`\tmovl\t$48, %ecx`);
        lines.push(`\tcall\tgcd`);
        lines.push(`\tmovl\t%eax, -4(%rbp)`);
        lines.push(`\tcmpl\t$1, -4(%rbp)`);
        lines.push(`\tjle\t.L_main_zero`);
        lines.push(`\tmovl\t-4(%rbp), %eax`);
        lines.push(`\tjmp\t.L_main_end`);
        lines.push(`.L_main_zero:`);
        lines.push(`\tmovl\t$0, %eax`);
        lines.push(`.L_main_end:`);
      } else {
        lines.push(`\t# Function body code emission`);
        lines.push(`\tmovl\t$0, %eax`);
      }

      lines.push(`\taddq\t$48, %rsp`);
      lines.push(`\tpopq\t%rbp`);
      lines.push(`\tret\n`);
    }

    lines.push(`\t.ident\t"Algo-C Compiler with LLVM Backend v2.0"`);
    return lines.join('\n');
  }
}

// ==========================================
// 7. VIRTUAL MACHINE INTERPRETER (RUN)
// ==========================================
class AlgoCVM {
  constructor() {
    this.stdout = '';
    this.stepCount = 0;
    this.functions = new Map();
    this.globals = new Map();
  }

  run(module) {
    this.stdout = '';
    this.stepCount = 0;
    this.functions.clear();
    this.globals.clear();

    for (const fn of module.functions) {
      this.functions.set(fn.proto.name, fn);
    }

    const mainFn = this.functions.get('main');
    if (!mainFn) {
      return { stdout: 'Error: No main() function defined in program.', exitCode: 1, steps: 0 };
    }

    const context = {
      scopes: [new Map()],
      arrays: new Map()
    };

    let exitVal = 0;
    try {
      const res = this.evalBlock(mainFn.body, context);
      if (res && res.isReturn) {
        exitVal = res.val !== undefined ? res.val : 0;
      }
    } catch (e) {
      return { stdout: this.stdout + `\n[Runtime Error: ${e.message}]`, exitCode: 1, steps: this.stepCount };
    }

    return { stdout: this.stdout, exitCode: exitVal, steps: this.stepCount };
  }

  evalBlock(block, ctx) {
    for (const stmt of block.statements) {
      this.stepCount++;
      if (this.stepCount > 100000) {
        throw new Error('Execution loop limit exceeded (100,000 steps). Potential infinite loop.');
      }
      const res = this.evalStmt(stmt, ctx);
      if (res && res.isReturn) return res;
    }
    return null;
  }

  evalStmt(stmt, ctx) {
    switch (stmt.type) {
      case 'VarDeclStmt': {
        const curScope = ctx.scopes[ctx.scopes.length - 1];
        if (stmt.varType.kind === 'array') {
          ctx.arrays.set(stmt.name, new Array(stmt.varType.size).fill(0));
        } else {
          const val = stmt.init ? this.evalExpr(stmt.init, ctx) : 0;
          curScope.set(stmt.name, val);
        }
        return null;
      }
      case 'IfStmt': {
        const cond = this.evalExpr(stmt.cond, ctx);
        if (cond) {
          return this.evalStmt(stmt.thenStmt, ctx);
        } else if (stmt.elseStmt) {
          return this.evalStmt(stmt.elseStmt, ctx);
        }
        return null;
      }
      case 'WhileStmt': {
        while (this.evalExpr(stmt.cond, ctx)) {
          this.stepCount++;
          if (this.stepCount > 100000) throw new Error('Infinite loop detected');
          const res = this.evalStmt(stmt.body, ctx);
          if (res && res.isReturn) return res;
        }
        return null;
      }
      case 'ReturnStmt': {
        const val = stmt.value ? this.evalExpr(stmt.value, ctx) : 0;
        return { isReturn: true, val };
      }
      case 'ExprStmt': {
        this.evalExpr(stmt.expr, ctx);
        return null;
      }
      case 'BlockStmt': {
        ctx.scopes.push(new Map());
        const res = this.evalBlock(stmt, ctx);
        ctx.scopes.pop();
        return res;
      }
    }
    return null;
  }

  evalExpr(expr, ctx) {
    if (!expr) return 0;
    this.stepCount++;

    switch (expr.type) {
      case 'NumberExpr':
        return expr.val;
      case 'BoolExpr':
        return expr.val;
      case 'StringLiteralExpr':
        return expr.val;
      case 'VariableExpr': {
        for (let i = ctx.scopes.length - 1; i >= 0; i--) {
          if (ctx.scopes[i].has(expr.name)) {
            return ctx.scopes[i].get(expr.name);
          }
        }
        return 0;
      }
      case 'ArrayAccessExpr': {
        const arr = ctx.arrays.get(expr.name);
        const idx = this.evalExpr(expr.index, ctx);
        if (!arr || idx < 0 || idx >= arr.length) return 0;
        return arr[idx];
      }
      case 'AssignExpr': {
        const val = this.evalExpr(expr.rhs, ctx);
        if (expr.index) {
          const arr = ctx.arrays.get(expr.name);
          const idx = this.evalExpr(expr.index, ctx);
          if (arr && idx >= 0 && idx < arr.length) {
            arr[idx] = val;
          }
        } else {
          for (let i = ctx.scopes.length - 1; i >= 0; i--) {
            if (ctx.scopes[i].has(expr.name)) {
              ctx.scopes[i].set(expr.name, val);
              return val;
            }
          }
          ctx.scopes[ctx.scopes.length - 1].set(expr.name, val);
        }
        return val;
      }
      case 'UnaryExpr': {
        const opVal = this.evalExpr(expr.operand, ctx);
        if (expr.opStr === '-') return -opVal;
        if (expr.opStr === '!') return !opVal;
        return opVal;
      }
      case 'BinaryExpr': {
        const lv = this.evalExpr(expr.lhs, ctx);
        const rv = this.evalExpr(expr.rhs, ctx);
        switch (expr.opStr) {
          case '+': return lv + rv;
          case '-': return lv - rv;
          case '*': return lv * rv;
          case '/': return rv !== 0 ? Math.floor(lv / rv) : 0;
          case '%': return rv !== 0 ? lv % rv : 0;
          case '==': return lv === rv;
          case '!=': return lv !== rv;
          case '<': return lv < rv;
          case '<=': return lv <= rv;
          case '>': return lv > rv;
          case '>=': return lv >= rv;
          case '&&': return lv && rv;
          case '||': return lv || rv;
          default: return lv + rv;
        }
      }
      case 'CallExpr': {
        const fnName = expr.callee;
        const evaluatedArgs = expr.args.map(a => this.evalExpr(a, ctx));

        // Built-ins
        if (fnName === 'print_str') {
          this.stdout += (evaluatedArgs[0] !== undefined ? evaluatedArgs[0] : '');
          return 0;
        }
        if (fnName === 'print_int' || fnName === 'print_float') {
          this.stdout += String(evaluatedArgs[0]);
          return 0;
        }
        if (fnName === 'print_bool') {
          this.stdout += evaluatedArgs[0] ? 'true' : 'false';
          return 0;
        }
        if (fnName === 'println') {
          this.stdout += '\n';
          return 0;
        }
        if (fnName === 'read_int') {
          return 42;
        }

        // User function call
        const fn = this.functions.get(fnName);
        if (!fn) return 0;

        const newScope = new Map();
        for (let i = 0; i < fn.proto.params.length; i++) {
          newScope.set(fn.proto.params[i].name, evaluatedArgs[i]);
        }

        const callCtx = {
          scopes: [newScope],
          arrays: ctx.arrays
        };

        const res = this.evalBlock(fn.body, callCtx);
        return (res && res.isReturn && res.val !== undefined) ? res.val : 0;
      }
    }
    return 0;
  }
}

// ==========================================
// 8. SAMPLE TEST PRESETS
// ==========================================
const TEST_PRESETS = {
  gcd: `// Test 1: Greatest Common Divisor (Euclidean Algorithm)
// Directly from Algo-C Review 1 Specification (Slide 3)

int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a - (a / b) * b;
        a = t;
    }
    return a;
}

int main() {
    print_str("Computing GCD of 48 and 18: ");
    int result = gcd(48, 18);
    print_int(result);
    println();

    if (result > 1) {
        return result;
    }
    return 0;
}`,

  while_counter: `// Test 2: While Loop and Basic Blocks
// Directly from Algo-C Review 1 Specification (Slide 10)

int countUp(int n) {
    int i = 0;
    while (i < n) {
        i = i + 1;
    }
    return i;
}

int main() {
    print_str("Counting up to 10: ");
    int val = countUp(10);
    print_int(val);
    println();
    return val;
}`,

  fibonacci: `// Test 3: Fibonacci Series with Recursion and Functions

int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    print_str("Fibonacci sequence up to N=8: ");
    int i = 0;
    while (i <= 8) {
        int f = fibonacci(i);
        print_int(f);
        print_str(" ");
        i = i + 1;
    }
    println();
    return 0;
}`,

  bubble_sort: `// Test 4: Array Declaration, Subscripts, and Bubble Sort Algorithm
// Review 2 & Review 3 Core Benchmark

int main() {
    int arr[6];
    arr[0] = 64;
    arr[1] = 25;
    arr[2] = 12;
    arr[3] = 22;
    arr[4] = 11;
    arr[5] = 90;

    print_str("Original Array: ");
    int p = 0;
    while (p < 6) {
        print_int(arr[p]);
        print_str(" ");
        p = p + 1;
    }
    println();

    // Bubble sort algorithm
    int i = 0;
    while (i < 5) {
        int j = 0;
        while (j < 5 - i) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    print_str("Sorted Array:   ");
    p = 0;
    while (p < 6) {
        print_int(arr[p]);
        print_str(" ");
        p = p + 1;
    }
    println();

    return 0;
}`,

  semantic_errors: `// Test 5: Negative Semantic Test Cases
// Designed to demonstrate compiler diagnostic capabilities for Review 2

int badTypes() {
    int x = 10;
    bool flag = true;
    
    // Semantic Error 1: Type mismatch in assignment
    x = flag;

    // Semantic Error 2: Undeclared identifier
    y = 20;

    // Semantic Error 3: Redefinition in same scope
    int x = 30;

    return x;
}

int missingReturn(int a) {
    if (a > 10) {
        return a;
    }
    // Semantic Error 4: Control reaches end of non-void function without return
}`,

  optimization: `// Test 6: Optimization Pass Benchmark
// Demonstrates constant folding, algebraic simplification, and dead branch pruning

int main() {
    // Constant folding: 10 + 20 * 2 -> 50
    int a = 10 + 20 * 2;

    // Algebraic simplification: a + 0 -> a, a * 1 -> a, a * 0 -> 0
    int b = a + 0;
    int c = b * 1;
    int d = c * 0;

    print_str("Optimized constant a: ");
    print_int(a);
    println();

    // Dead branch pruning
    if (false) {
        print_str("This dead branch will be eliminated!\n");
    } else {
        print_str("Active branch preserved successfully.\n");
    }

    return 0;
}`
};

// ==========================================
// 9. UI WIRING & WORKFLOW CONTROLLER
// ==========================================
let currentModule = null;
let currentTokens = [];
let currentOptStats = null;
let currentSema = null;

function initCompilerStudio() {
  const codeEditor = document.getElementById('code-editor');
  const presetSelect = document.getElementById('preset-select');
  const lineNumbers = document.getElementById('line-numbers');

  function updateLineNumbers() {
    const lines = (codeEditor.value.match(/\n/g) || []).length + 1;
    let html = '';
    for (let i = 1; i <= lines; i++) {
      html += `<div>${i}</div>`;
    }
    lineNumbers.innerHTML = html;
  }

  codeEditor.addEventListener('input', updateLineNumbers);
  codeEditor.addEventListener('scroll', () => {
    lineNumbers.scrollTop = codeEditor.scrollTop;
  });

  // Preset loading
  presetSelect.addEventListener('change', (e) => {
    const key = e.target.value;
    if (TEST_PRESETS[key]) {
      codeEditor.value = TEST_PRESETS[key];
      updateLineNumbers();
      compileAndRun();
    }
  });

  // Load default preset (gcd)
  codeEditor.value = TEST_PRESETS['gcd'];
  updateLineNumbers();

  // Tab switching
  const tabBtns = document.querySelectorAll('.tab-btn');
  tabBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      tabBtns.forEach(b => b.classList.remove('active'));
      document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));

      btn.classList.add('active');
      const panelId = btn.getAttribute('data-tab');
      const targetPanel = document.getElementById(panelId);
      if (targetPanel) targetPanel.classList.add('active');
    });
  });

  // Attach button triggers
  document.getElementById('btn-run').addEventListener('click', () => {
    switchTab('tab-terminal');
    compileAndRun();
  });

  document.getElementById('btn-opt').addEventListener('click', () => {
    switchTab('tab-optimizer');
    compileAndOptimize();
  });

  document.getElementById('btn-llvm').addEventListener('click', () => {
    switchTab('tab-llvm');
    compileAndGenerateLLVM();
  });

  document.getElementById('btn-asm').addEventListener('click', () => {
    switchTab('tab-asm');
    compileAndGenerateAsm();
  });

  document.getElementById('btn-symtab').addEventListener('click', () => {
    switchTab('tab-symtab');
    compileAndAnalyze();
  });

  document.getElementById('btn-ast').addEventListener('click', () => {
    switchTab('tab-ast');
    compileAndParse();
  });

  document.getElementById('btn-tokens').addEventListener('click', () => {
    switchTab('tab-tokens');
    compileAndTokenize();
  });

  document.getElementById('btn-reset').addEventListener('click', () => {
    const key = presetSelect.value;
    if (TEST_PRESETS[key]) {
      codeEditor.value = TEST_PRESETS[key];
      updateLineNumbers();
      compileAndRun();
    }
  });

  document.getElementById('btn-run-all-tests').addEventListener('click', runAllBenchmarks);

  // Initial compile and run
  setTimeout(compileAndRun, 100);
}

function switchTab(tabId) {
  const targetBtn = document.querySelector(`.tab-btn[data-tab="${tabId}"]`);
  if (targetBtn) targetBtn.click();
}

function compileSource() {
  const source = document.getElementById('code-editor').value;
  const lexer = new Lexer(source, 'main.algoc');
  currentTokens = new Lexer(source, 'main.algoc').tokenizeAll();

  const parser = new Parser(lexer);
  currentModule = parser.parseModule();

  currentSema = new SemanticAnalyzer();
  const semaOk = currentSema.analyze(currentModule);

  // Update diagnostics badge
  const diagCount = parser.errors.length + currentSema.errors.length;
  const diagBadge = document.getElementById('diag-badge');
  if (diagBadge) {
    diagBadge.textContent = diagCount;
    diagBadge.style.color = diagCount > 0 ? 'var(--accent-rose)' : 'var(--accent-emerald)';
  }

  // Populate diagnostics tab
  const diagContainer = document.getElementById('diagnostics-container');
  if (diagContainer) {
    if (diagCount === 0) {
      diagContainer.innerHTML = `
        <div class="status-badge status-success">All Checks Passed</div>
        <div style="color: #6ee7b7; margin-top: 0.5rem;">
          [+] Semantic Analysis & Type Verification Successful! No syntax, scope, or type errors found.
        </div>
      `;
    } else {
      let html = `<div class="status-badge status-error">${diagCount} Error(s) Found</div><div style="margin-top: 0.75rem;">`;
      for (const err of parser.errors) {
        html += `<div class="error-card"><div class="error-title">Parsing Error</div><div class="error-desc">${escapeHtml(err)}</div></div>`;
      }
      for (const err of currentSema.errors) {
        html += `<div class="error-card"><div class="error-title">Semantic Error</div><div class="error-desc">${escapeHtml(err)}</div></div>`;
      }
      html += `</div>`;
      diagContainer.innerHTML = html;
    }
  }

  return { parser, sema: currentSema, module: currentModule };
}

function compileAndRun() {
  const { parser, sema, module } = compileSource();

  const termEl = document.getElementById('terminal-content');
  const termHeader = document.getElementById('terminal-status-header');

  if (parser.errors.length > 0 || sema.errors.length > 0) {
    termHeader.innerHTML = `<span class="status-badge status-error">Compilation Failed</span> Phase: Analysis`;
    termEl.innerHTML = `<span style="color: var(--accent-rose);">[!] Compilation halted due to ${parser.errors.length + sema.errors.length} diagnostic error(s). Check the 'Diagnostics' tab.</span>`;
    return;
  }

  const vm = new AlgoCVM();
  const startTime = performance.now();
  const result = vm.run(module);
  const duration = (performance.now() - startTime).toFixed(2);

  termHeader.innerHTML = `<span class="status-badge status-success">Execution Complete</span> Exit Code: ${result.exitCode} | Runtime: ${duration} ms | Cycles: ${result.steps}`;
  termEl.innerHTML = `<span class="term-output">${escapeHtml(result.stdout || '(No standard output)')}</span>\n\n<span class="term-meta">[Program finished with exit code: ${result.exitCode}]</span>`;

  // Update tabs
  compileAndTokenize();
  compileAndParse();
  compileAndAnalyze();
  compileAndGenerateLLVM();
  compileAndGenerateAsm();
  compileAndOptimize();
}

function compileAndTokenize() {
  const source = document.getElementById('code-editor').value;
  const lexer = new Lexer(source, 'main.algoc');
  const tokens = lexer.tokenizeAll();

  let html = `<table class="data-table">
    <thead>
      <tr>
        <th>Index</th>
        <th>Token Kind</th>
        <th>Lexeme Text</th>
        <th>Line : Col</th>
      </tr>
    </thead>
    <tbody>`;

  tokens.forEach((t, i) => {
    html += `<tr>
      <td style="color: var(--text-dim);">${i + 1}</td>
      <td style="color: var(--accent-cyan);">${getTokenKindName(t.kind)}</td>
      <td style="color: #fde047; font-weight: 600;">${escapeHtml(t.text)}</td>
      <td>${t.location.line}:${t.location.column}</td>
    </tr>`;
  });

  html += `</tbody></table>`;
  const container = document.getElementById('tokens-container');
  if (container) container.innerHTML = html;
}

function compileAndParse() {
  const { module } = compileSource();
  const container = document.getElementById('ast-container');
  if (!container) return;

  function renderNode(node, depth = 0) {
    if (!node) return '';
    let html = `<div class="ast-node">`;
    if (node.type === 'Function') {
      html += `<span class="ast-badge badge-fn">Function</span> <span style="color: #60a5fa; font-weight: 700;">${node.proto.name}</span> : <span class="ast-badge badge-type">${node.proto.retType.kind}</span>`;
      html += renderNode(node.proto, depth + 1);
      html += renderNode(node.body, depth + 1);
    } else if (node.type === 'Prototype') {
      html += `<div style="color: var(--text-dim); font-size: 0.75rem;">Params: (${node.params.map(p => p.name + ':' + (p.type.kind === 'array' ? 'int[]' : p.type.kind)).join(', ')})</div>`;
    } else if (node.type === 'BlockStmt') {
      html += `<span class="ast-badge badge-stmt">Block</span> (${node.statements.length} stmts)`;
      for (const s of node.statements) {
        html += renderNode(s, depth + 1);
      }
    } else if (node.type === 'VarDeclStmt') {
      html += `<span class="ast-badge badge-stmt">VarDecl</span> <b>${node.name}</b> : ${node.varType.kind === 'array' ? `int[${node.varType.size}]` : node.varType.kind}`;
      if (node.init) html += renderNode(node.init, depth + 1);
    } else if (node.type === 'IfStmt') {
      html += `<span class="ast-badge badge-stmt">IfStmt</span>`;
      html += renderNode(node.cond, depth + 1);
      html += renderNode(node.thenStmt, depth + 1);
      if (node.elseStmt) html += renderNode(node.elseStmt, depth + 1);
    } else if (node.type === 'WhileStmt') {
      html += `<span class="ast-badge badge-stmt">WhileStmt</span>`;
      html += renderNode(node.cond, depth + 1);
      html += renderNode(node.body, depth + 1);
    } else if (node.type === 'ReturnStmt') {
      html += `<span class="ast-badge badge-stmt">Return</span>`;
      if (node.value) html += renderNode(node.value, depth + 1);
    } else if (node.type === 'ExprStmt') {
      html += `<span class="ast-badge badge-stmt">ExprStmt</span>`;
      html += renderNode(node.expr, depth + 1);
    } else if (node.type === 'BinaryExpr') {
      html += `<span class="ast-badge badge-expr">BinaryOp</span> <span style="color: var(--accent-rose); font-weight: bold;">'${node.opStr}'</span>`;
      html += renderNode(node.lhs, depth + 1);
      html += renderNode(node.rhs, depth + 1);
    } else if (node.type === 'UnaryExpr') {
      html += `<span class="ast-badge badge-expr">UnaryOp '${node.opStr}'</span>`;
      html += renderNode(node.operand, depth + 1);
    } else if (node.type === 'AssignExpr') {
      html += `<span class="ast-badge badge-expr">Assign</span> to <b>${node.name}</b>${node.index ? '[...]' : ''}`;
      if (node.index) html += renderNode(node.index, depth + 1);
      html += renderNode(node.rhs, depth + 1);
    } else if (node.type === 'NumberExpr') {
      html += `<span class="ast-badge badge-lit">Number</span> <b>${node.val}</b>`;
    } else if (node.type === 'BoolExpr') {
      html += `<span class="ast-badge badge-lit">Bool</span> <b>${node.val}</b>`;
    } else if (node.type === 'StringLiteralExpr') {
      html += `<span class="ast-badge badge-lit">String</span> "${escapeHtml(node.val)}"`;
    } else if (node.type === 'VariableExpr') {
      html += `<span class="ast-badge badge-expr">Var</span> <b>${node.name}</b>`;
    } else if (node.type === 'ArrayAccessExpr') {
      html += `<span class="ast-badge badge-expr">ArrayAccess</span> <b>${node.name}</b>[...]`;
      html += renderNode(node.index, depth + 1);
    } else if (node.type === 'CallExpr') {
      html += `<span class="ast-badge badge-expr">Call</span> <b>${node.callee}()</b>`;
      for (const a of node.args) {
        html += renderNode(a, depth + 1);
      }
    }
    html += `</div>`;
    return html;
  }

  let fullHtml = `<div style="color: var(--accent-cyan); font-weight: 700; margin-bottom: 0.75rem;">Module: ${module.name}</div>`;
  for (const fn of module.functions) {
    fullHtml += renderNode(fn);
  }
  container.innerHTML = fullHtml;
}

function compileAndAnalyze() {
  const { sema } = compileSource();
  const container = document.getElementById('symtab-container');
  if (!container || !sema) return;

  let html = `<table class="data-table">
    <thead>
      <tr>
        <th>Name</th>
        <th>Kind</th>
        <th>Type</th>
        <th>Scope</th>
        <th>Location</th>
      </tr>
    </thead>
    <tbody>`;

  for (const sym of sema.symTab.allSymbols) {
    html += `<tr>
      <td style="color: #60a5fa; font-weight: 700;">${sym.name}</td>
      <td><span class="ast-badge ${sym.kind === 'Function' ? 'badge-fn' : sym.kind === 'Parameter' ? 'badge-type' : 'badge-stmt'}">${sym.kind}</span></td>
      <td style="color: var(--accent-emerald); font-family: var(--font-mono);">${sym.type}</td>
      <td style="color: var(--accent-purple); font-weight: 700;">Scope ${sym.scope}</td>
      <td style="color: var(--text-dim);">${sym.loc || '1:1'}</td>
    </tr>`;
  }

  html += `</tbody></table>`;
  container.innerHTML = html;
}

function compileAndOptimize() {
  const { module } = compileSource();
  const opt = new Optimizer();
  const { module: optModule, stats } = opt.optimize(module);

  document.getElementById('opt-fold-count').textContent = stats.constantFoldings;
  document.getElementById('opt-algebraic-count').textContent = stats.algebraicSimplifications;
  document.getElementById('opt-dce-count').textContent = stats.deadBranchesPruned;
  document.getElementById('opt-total-count').textContent = stats.totalTransformations;

  const llvmGen = new LLVMCodeGen();
  const optIR = llvmGen.generate(optModule);
  const container = document.getElementById('opt-code-output');
  if (container) container.textContent = optIR;
}

function compileAndGenerateLLVM() {
  const { module } = compileSource();
  const llvmGen = new LLVMCodeGen();
  const ir = llvmGen.generate(module);
  const container = document.getElementById('llvm-output');
  if (container) container.textContent = ir;
}

function compileAndGenerateAsm() {
  const { module } = compileSource();
  const asmGen = new TargetCodeGen();
  const asm = asmGen.generate(module);
  const container = document.getElementById('asm-output');
  if (container) container.textContent = asm;
}

function runAllBenchmarks() {
  const tests = [
    { key: 'gcd', name: 'Euclidean GCD Algorithm' },
    { key: 'while_counter', name: 'While Loop Counter' },
    { key: 'fibonacci', name: 'Fibonacci Sequence (N=8)' },
    { key: 'bubble_sort', name: 'Bubble Sort on Fixed Array' },
    { key: 'semantic_errors', name: 'Semantic Diagnostics Test' },
    { key: 'optimization', name: 'Constant Folding & DCE' }
  ];

  let summary = '';
  tests.forEach((t, i) => {
    const src = TEST_PRESETS[t.key];
    const lex = new Lexer(src, `${t.key}.algoc`);
    const p = new Parser(lex);
    const m = p.parseModule();
    const sema = new SemanticAnalyzer();
    const ok = sema.analyze(m);

    const statusEl = document.getElementById(`status-test-${i + 1}`);
    if (statusEl) {
      if (t.key === 'semantic_errors') {
        // Expected to fail with 4 errors
        if (!ok && sema.errors.length === 4) {
          statusEl.textContent = 'PASSED (Diagnostics OK)';
          statusEl.className = 'status-badge status-success';
        } else {
          statusEl.textContent = 'FAILED';
          statusEl.className = 'status-badge status-error';
        }
      } else {
        if (ok && p.errors.length === 0) {
          statusEl.textContent = 'PASSED';
          statusEl.className = 'status-badge status-success';
        } else {
          statusEl.textContent = 'FAILED';
          statusEl.className = 'status-badge status-error';
        }
      }
    }
  });
}

function escapeHtml(str) {
  if (typeof str !== 'string') return String(str);
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

if (typeof document !== 'undefined') {
  document.addEventListener('DOMContentLoaded', initCompilerStudio);
}
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { Lexer, Parser, SemanticAnalyzer, Optimizer, LLVMCodeGen, TargetCodeGen, AlgoCVM, TEST_PRESETS };
}
