#include "cad_file_processing.hpp"

#include <iostream>

int main(int argc, char** argv) {
    using namespace app_builder::cad_generics;

    if (argc < 2) {
        std::cout << "Usage: cad_file_processing_demo <path-to-cad-file>\n";
        return 1;
    }

    const std::string path = argv[1];
    ProcessingOptions options;
    options.require_file_to_exist = false;

    const ProcessResult result = process_cad_file(path, options);

    std::cout << "Path: " << result.file.path << "\n";
    std::cout << "Extension: " << result.file.extension << "\n";
    std::cout << "Type: " << cad_file_type_to_string(result.file.type) << "\n";
    std::cout << "Exists: " << (result.file.exists ? "yes" : "no") << "\n";
    std::cout << "Result: " << (result.success ? "success" : "failure") << "\n";
    std::cout << "Message: " << result.message << "\n";

    return result.success ? 0 : 2;
}
