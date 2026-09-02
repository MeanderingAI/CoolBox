#include "interpreter_php.h"
#include "lexer_php.h"
#include "parser_php.h"

#include <cassert>
#include <iostream>

namespace {

std::string run_php(const std::string& source) {
    plphp::LexerPHP lexer(source);
    plphp::ParserPHP parser(lexer.tokenize());
    auto program = parser.parse_program();
    plphp::InterpreterPHP interpreter;
    return interpreter.run(program);
}

void test_html_passthrough() {
    const std::string out = run_php("<html><body>Hello</body></html>");
    assert(out == "<html><body>Hello</body></html>");
}

void test_echo_and_variables() {
    const std::string out = run_php("<?php $name = \"World\"; echo \"Hello, \" . $name . \"!\"; ?>");
    assert(out == "Hello, World!");
}

void test_short_echo_tag() {
    const std::string out = run_php("<p><?= 1 + 2 ?></p>");
    assert(out == "<p>3</p>");
}

void test_if_else() {
    const std::string out = run_php("<?php $x = 5; if ($x > 10) { echo \"big\"; } elseif ($x > 3) { echo \"mid\"; } else { echo \"small\"; } ?>");
    assert(out == "mid");
}

void test_for_loop() {
    const std::string out = run_php("<?php for ($i = 0; $i < 3; $i++) { echo $i; } ?>");
    assert(out == "012");
}

void test_foreach_array() {
    const std::string out = run_php(
        "<?php $items = [\"a\", \"b\", \"c\"]; foreach ($items as $k => $v) { echo $k . \":\" . $v . \",\"; } ?>");
    assert(out == "0:a,1:b,2:c,");
}

void test_function_call() {
    const std::string out = run_php(
        "<?php function add($a, $b) { return $a + $b; } echo add(2, 3); ?>");
    assert(out == "5");
}

void test_builtin_functions() {
    const std::string out = run_php("<?php echo strtoupper(\"abc\") . \"-\" . strlen(\"abcd\"); ?>");
    assert(out == "ABC-4");
}

void test_array_push_and_count() {
    const std::string out = run_php(
        "<?php $arr = []; array_push($arr, 1); array_push($arr, 2); echo count($arr); ?>");
    assert(out == "2");
}

void test_interpolation() {
    const std::string out = run_php("<?php $x = 7; echo \"value=$x\"; ?>");
    assert(out == "value=7");
}

} // namespace

int main() {
    test_html_passthrough();
    test_echo_and_variables();
    test_short_echo_tag();
    test_if_else();
    test_for_loop();
    test_foreach_array();
    test_function_call();
    test_builtin_functions();
    test_array_push_and_count();
    test_interpolation();

    std::cout << "All PHP interpreter tests passed." << std::endl;
    return 0;
}
