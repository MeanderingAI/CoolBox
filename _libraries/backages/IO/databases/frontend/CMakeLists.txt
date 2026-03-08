
# SQL Frontend — lexical analysis, syntax analysis, semantic analysis

# 1. Lexical analysis (tokenizer)
add_subdirectory(lexical_analysis/parser)

# 2. Syntax analysis (AST generator + grammar validation)
add_subdirectory(syntax_analysis/ast)
add_subdirectory(syntax_analysis/grammar_validation)

# 3. Semantic analysis (meaning analysis)
add_subdirectory(semantic_analysis/meaning_analysis)
