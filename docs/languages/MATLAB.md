**MATLAB Language Specification (subset implemented)**

- **Purpose:** Describe the lightweight MATLAB subset supported by the parser and LSP server in this repository.

- **Supported Lexical Elements:**
  - Identifiers: [A-Za-z_][A-Za-z0-9_]*
  - Numbers: integers, floating point
  - Strings: single quotes '...'
  - Comments: `%` until end-of-line
  - Delimiters: `;`, `,`, `(`, `)`, `=`, `end`

- **Supported Grammar (high level):**
  - Script and function files
  - Function definitions: `function [out] = name(args)` ... `end`
  - Assignments and simple expressions
  - Basic `if`, `for`, `while` blocks

- **AST Nodes (exposed by parser):**
  - TranslationUnit
  - FunctionDecl(name, args, body)
  - Assignment(lhs, rhs)
  - IfStmt(cond, thenBody, elseBody)
  - ForStmt(init, cond, body)

- **Diagnostics produced by parser/LSP (current):**
  - Lexical errors (invalid token, unterminated string)
  - Syntax errors (unexpected token, missing `end`)

- **LSP features roadmap:**
  - Phase 1: publishDiagnostics on didOpen/didChange
  - Phase 2: hover and simple symbol index (functions, variables)
  - Phase 3: completions and simple refactorings

- **Limitations:**
  - This parser is intentionally small and focuses on common patterns for quick editor feedback.
