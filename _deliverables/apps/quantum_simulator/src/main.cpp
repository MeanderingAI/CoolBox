#include "demo_runner.hpp"
#include "quantum_lab_window.hpp"

#include <cli_tools.hpp>

#include <iostream>
#include <random>
#include <string>

namespace {

void print_demo_result(const quantum_simulator_app::DemoResult& result) {
    std::cout << "=== " << result.title << " ===\n";
    for (const auto& line : result.log_lines) std::cout << "  " << line << '\n';
    if (!result.headline.empty()) std::cout << "  >> " << result.headline << '\n';
    if (!result.probabilities.empty()) {
        std::cout << "  Outcome probabilities:\n";
        for (std::size_t i = 0; i < result.probabilities.size(); ++i) {
            if (result.probabilities[i] < 1e-6) continue;
            std::cout << "    |";
            for (int b = result.num_qubits_for_display - 1; b >= 0; --b) {
                std::cout << ((i >> b) & 1);
            }
            std::cout << "> : " << (result.probabilities[i] * 100.0) << "%\n";
        }
    }
    std::cout << '\n';
}

quantum_simulator_app::DemoId demo_from_name(const std::string& name) {
    using quantum_simulator_app::DemoId;
    if (name == "bell") return DemoId::Bell;
    if (name == "ghz") return DemoId::Ghz;
    if (name == "dj-constant") return DemoId::DeutschJozsaConstant;
    if (name == "dj-balanced") return DemoId::DeutschJozsaBalanced;
    if (name == "grover") return DemoId::Grover;
    if (name == "shor15") return DemoId::ShorFactor15;
    if (name == "shor21") return DemoId::ShorFactor21;
    if (name == "shor33") return DemoId::ShorFactor33;
    if (name == "shor35") return DemoId::ShorFactor35;
    throw std::invalid_argument("unknown demo name: " + name);
}

} // namespace

int main(int argc, const char* const argv[]) {
    os_generics::cli::CommandLineParser parser;
    parser.set_program_name("quantum_simulator");
    parser.set_description(
        "CoolBox Quantum Simulator -- an exact, noiseless state-vector quantum "
        "computer simulator with classic demo algorithms (Bell/GHZ states, "
        "Deutsch-Jozsa, Grover's search, Shor's factoring).");
    parser.add_option({"cli", '\0', false, false, "",
                       "Run in headless/console mode instead of launching the GUI."});
    parser.add_option({"demo", 'd', true, false, "NAME",
                       "Which demo to run in --cli mode: bell, ghz, dj-constant, dj-balanced, "
                       "grover, shor15, shor21, shor33, shor35. Runs all of them if omitted."});
    parser.add_option({"seed", '\0', true, false, "N", "Random seed (default: a random seed)."});
    parser.add_option({"help", 'h', false, false, "", "Show help."});

    const auto args = os_generics::cli::argv_to_vector(argc, argv);
    const auto parsed = parser.parse(args);

    if (parsed.has_option("help")) {
        std::cout << parser.render_help() << '\n';
        return 0;
    }
    if (!parsed.ok()) {
        for (const auto& error : parsed.errors) std::cerr << error << '\n';
        std::cerr << parser.render_help() << '\n';
        return 2;
    }

    std::uint64_t seed;
    if (parsed.has_option("seed")) {
        seed = std::stoull(parsed.option_value("seed"));
    } else {
        std::random_device rd;
        seed = (static_cast<std::uint64_t>(rd()) << 32) | rd();
    }
    std::mt19937_64 rng(seed);

    if (!parsed.has_option("cli")) {
        quantum_simulator_app::QuantumLabWindow window;
        return window.run() ? 0 : 1;
    }

    try {
        if (parsed.has_option("demo")) {
            const auto id = demo_from_name(parsed.option_value("demo"));
            print_demo_result(quantum_simulator_app::run_demo(id, rng));
        } else {
            for (const auto& descriptor : quantum_simulator_app::demo_list()) {
                print_demo_result(quantum_simulator_app::run_demo(descriptor.id, rng));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "quantum_simulator: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
