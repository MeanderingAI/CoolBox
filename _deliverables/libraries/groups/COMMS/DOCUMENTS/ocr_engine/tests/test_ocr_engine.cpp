#include "ocr_engine.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void test_unavailable_engine_throws() {
    docs::UnavailableOcrEngine engine;
    bool threw = false;
    try {
        engine.recognize_text(std::vector<unsigned char>{1, 2, 3});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_default_engine_is_unavailable() {
    auto engine = docs::default_ocr_engine();
    assert(engine != nullptr);
    bool threw = false;
    try {
        engine->recognize_text({});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

} // namespace

int main() {
    test_unavailable_engine_throws();
    test_default_engine_is_unavailable();

    std::cout << "All ocr_engine tests passed." << std::endl;
    return 0;
}
