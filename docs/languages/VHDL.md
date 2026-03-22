**VHDL Language Specification (subset implemented)**

- **Purpose:** Formal description of the supported subset used by the repository parsers and LSP server.
- **Targets:** diagnostics via LSP; roadmap for hover, go-to-definition, completions.

- **Supported Lexical Elements:**
  - Identifiers: [A-Za-z_][A-Za-z0-9_]*
  - Numbers: integers (decimal) and real literals (simple form)
  - Strings: delimited by double quotes "..."
  - Comments: `--` until end-of-line
  - Delimiters: `;`, `:`, `,`, `(`, `)`, `=>`

- **Supported Grammar (high level):**
  - Design unit: `entity` / `architecture` pairs
  - Entity: `entity <ident> is` ... `end entity;`
  - Architecture: `architecture <ident> of <entity> is` declarations `begin` statements `end architecture;`
  - Simple signal and port declarations (identifier, optional type, optional default)
  - Process blocks: `process` ... `end process;`

- **AST Nodes (exposed by parser):**
  - TranslationUnit
  - EntityDecl(name, ports)
  - ArchitectureDecl(name, entity, declarations, statements)
  - SignalDecl(name, type, default)
  - ProcessDecl(sensitivityList, statements)

- **Diagnostics produced by parser/LSP (current):**
  - Lexical errors: invalid token, unterminated string
  - Syntax errors: unexpected token, missing `;` or `end` markers
  - Semantic (planned): unknown identifier, duplicate port name

- **LSP features roadmap:**
  - Phase 1 (now): publishDiagnostics on didOpen/didChange
  - Phase 2: hover (type/name summaries), simple go-to-definition for entity/architecture
  - Phase 3: completions for keywords and port names, workspace symbols

- **Limitations:**
  - This implementation is intentionally lightweight and only targets the subset above for quick diagnostics in editors.
