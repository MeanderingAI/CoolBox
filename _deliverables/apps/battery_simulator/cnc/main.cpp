#include <battery.h>
#include <cli_tools.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using battery::Cell;
using battery::Chemistry;

struct ScriptLine {
    std::string command;
    std::map<std::string, std::string> args;
    int line_number = 0;
};

struct PackConfig {
    std::string label = "pack";
    int series = 1;
    int parallel = 1;
    Chemistry chemistry = Chemistry::LITHIUM_ION;
    double capacity_ah = battery::ChemistryDefaults::for_chemistry(Chemistry::LITHIUM_ION).typical_capacity;
    double internal_resistance_ohm =
        battery::ChemistryDefaults::for_chemistry(Chemistry::LITHIUM_ION).typical_internal_resistance;
    double initial_soc = 1.0;
    double ambient_c = 25.0;
    double initial_soh = 1.0;
    std::vector<std::pair<Chemistry, double>> mixture;
};

struct SimCell {
    Cell cell;
    double soh = 1.0;
    double temperature_c = 25.0;

    explicit SimCell(const Cell& c, double initial_soh, double initial_temperature_c)
        : cell(c), soh(initial_soh), temperature_c(initial_temperature_c) {
        cell.set_temperature(initial_temperature_c);
    }
};

struct SimPack {
    PackConfig config;
    std::vector<SimCell> cells;
    double ambient_c = 25.0;
    double elapsed_s = 0.0;
    double delivered_wh = 0.0;
    double absorbed_wh = 0.0;
    double throughput_ah = 0.0;

    int total_cells() const { return config.series * config.parallel; }
    int index(int s, int p) const { return s * config.parallel + p; }

    double average_soc() const {
        double total = 0.0;
        for (const auto& c : cells) total += c.cell.soc();
        return cells.empty() ? 0.0 : total / static_cast<double>(cells.size());
    }

    double min_soc() const {
        double value = 1.0;
        for (const auto& c : cells) value = std::min(value, c.cell.soc());
        return cells.empty() ? 0.0 : value;
    }

    double average_soh() const {
        double total = 0.0;
        for (const auto& c : cells) total += c.soh;
        return cells.empty() ? 0.0 : total / static_cast<double>(cells.size());
    }

    double min_soh() const {
        double value = 1.0;
        for (const auto& c : cells) value = std::min(value, c.soh);
        return cells.empty() ? 0.0 : value;
    }

    double average_temperature_c() const {
        double total = 0.0;
        for (const auto& c : cells) total += c.temperature_c;
        return cells.empty() ? ambient_c : total / static_cast<double>(cells.size());
    }

    double max_temperature_c() const {
        double value = ambient_c;
        for (const auto& c : cells) value = std::max(value, c.temperature_c);
        return value;
    }

    double capacity_ah() const {
        if (cells.empty()) return 0.0;
        double stage_capacity = 0.0;
        for (int p = 0; p < config.parallel; ++p) {
            const auto& c = cells[index(0, p)];
            stage_capacity += c.cell.capacity_ah() * c.soh;
        }
        return stage_capacity;
    }

    double open_circuit_voltage() const {
        double voltage = 0.0;
        for (int s = 0; s < config.series; ++s) {
            double stage_voltage = 0.0;
            for (int p = 0; p < config.parallel; ++p) {
                stage_voltage += cells[index(s, p)].cell.open_circuit_voltage();
            }
            voltage += stage_voltage / static_cast<double>(config.parallel);
        }
        return voltage;
    }

    double internal_resistance_ohm() const {
        double total = 0.0;
        for (int s = 0; s < config.series; ++s) {
            double conductance = 0.0;
            for (int p = 0; p < config.parallel; ++p) {
                const double r = cells[index(s, p)].cell.internal_resistance();
                if (r > 0.0) conductance += 1.0 / r;
            }
            if (conductance > 0.0) total += 1.0 / conductance;
        }
        return total;
    }

    double terminal_voltage(double current_a) const {
        return std::max(0.0, open_circuit_voltage() - current_a * internal_resistance_ohm());
    }

    bool depleted() const {
        for (const auto& c : cells) {
            if (c.cell.is_depleted() || c.cell.soc() <= 0.0001) return true;
        }
        return false;
    }
};

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(begin, end - begin);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string normalize_key(std::string key) {
    key = lower(trim(key));
    std::replace(key.begin(), key.end(), '-', '_');
    return key;
}

std::vector<std::string> split_words(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) words.push_back(word);
    return words;
}

double parse_double(const std::string& value, const std::string& key) {
    try {
        std::size_t used = 0;
        const double parsed = std::stod(value, &used);
        if (used != value.size()) throw std::invalid_argument("trailing characters");
        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid numeric value for '" + key + "': " + value);
    }
}

int parse_int(const std::string& value, const std::string& key) {
    const double parsed = parse_double(value, key);
    const int rounded = static_cast<int>(std::lround(parsed));
    if (std::fabs(parsed - rounded) > 1e-9) {
        throw std::runtime_error("Expected integer value for '" + key + "': " + value);
    }
    return rounded;
}

double clamp(double value, double lo, double hi) {
    return std::max(lo, std::min(hi, value));
}

double parse_fraction(const std::string& value, const std::string& key) {
    double parsed = parse_double(value, key);
    if (parsed > 1.0) parsed /= 100.0;
    return clamp(parsed, 0.0, 1.0);
}

Chemistry parse_chemistry(const std::string& value) {
    const std::string key = lower(value);
    if (key == "li-ion" || key == "li_ion" || key == "liion" ||
        key == "lithium_ion" || key == "lithium-ion") {
        return Chemistry::LITHIUM_ION;
    }
    if (key == "lipo" || key == "li-po" || key == "li_po" ||
        key == "lithium_polymer" || key == "lithium-polymer") {
        return Chemistry::LITHIUM_POLYMER;
    }
    if (key == "lifepo4" || key == "lfp" || key == "lithium_iron_phosphate") {
        return Chemistry::LITHIUM_IRON_PHOSPHATE;
    }
    if (key == "nimh" || key == "nickel_metal_hydride") {
        return Chemistry::NICKEL_METAL_HYDRIDE;
    }
    if (key == "lead-acid" || key == "lead_acid" || key == "pba") {
        return Chemistry::LEAD_ACID;
    }
    if (key == "alkaline") {
        return Chemistry::ALKALINE;
    }
    throw std::runtime_error("Unknown chemistry: " + value);
}

std::string chemistry_label(Chemistry chemistry) {
    return battery::chemistry_name(chemistry);
}

std::map<std::string, std::string> parse_args(const std::vector<std::string>& words, std::size_t begin) {
    std::map<std::string, std::string> args;
    for (std::size_t i = begin; i < words.size(); ++i) {
        const std::string& word = words[i];
        const std::size_t eq = word.find('=');
        if (eq == std::string::npos || eq == 0) {
            throw std::runtime_error("Expected key=value token, got '" + word + "'");
        }
        args[normalize_key(word.substr(0, eq))] = word.substr(eq + 1);
    }
    return args;
}

bool has_arg(const ScriptLine& line, const std::string& key) {
    return line.args.find(normalize_key(key)) != line.args.end();
}

std::string arg_string(const ScriptLine& line, const std::string& key, const std::string& fallback = "") {
    const auto it = line.args.find(normalize_key(key));
    return it == line.args.end() ? fallback : it->second;
}

double arg_double(const ScriptLine& line, const std::string& key, double fallback) {
    const auto it = line.args.find(normalize_key(key));
    return it == line.args.end() ? fallback : parse_double(it->second, key);
}

int arg_int(const ScriptLine& line, const std::string& key, int fallback) {
    const auto it = line.args.find(normalize_key(key));
    return it == line.args.end() ? fallback : parse_int(it->second, key);
}

std::vector<ScriptLine> load_script(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Unable to open script: " + path);
    }

    std::vector<ScriptLine> lines;
    std::string raw;
    int line_number = 0;
    while (std::getline(file, raw)) {
        ++line_number;
        const std::size_t comment = raw.find('#');
        if (comment != std::string::npos) raw = raw.substr(0, comment);
        raw = trim(raw);
        if (raw.empty()) continue;

        auto words = split_words(raw);
        if (words.empty()) continue;

        ScriptLine line;
        line.command = normalize_key(words[0]);
        line.line_number = line_number;
        try {
            line.args = parse_args(words, 1);
        } catch (const std::exception& e) {
            throw std::runtime_error("Line " + std::to_string(line_number) + ": " + e.what());
        }
        lines.push_back(line);
    }

    return lines;
}

PackConfig parse_pack_config(const std::vector<ScriptLine>& lines) {
    PackConfig config;
    bool saw_pack = false;
    bool saw_mixture = false;

    for (const auto& line : lines) {
        if (line.command == "pack") {
            saw_pack = true;
            config.label = arg_string(line, "label", config.label);
            config.series = arg_int(line, "series", config.series);
            config.parallel = arg_int(line, "parallel", config.parallel);
            if (has_arg(line, "cells")) {
                const int cells = arg_int(line, "cells", config.series * config.parallel);
                if (!has_arg(line, "series") && has_arg(line, "parallel")) {
                    if (cells % config.parallel != 0) {
                        throw std::runtime_error("Line " + std::to_string(line.line_number) +
                                                 ": cells must divide evenly by parallel");
                    }
                    config.series = cells / config.parallel;
                } else if (has_arg(line, "series") && !has_arg(line, "parallel")) {
                    if (cells % config.series != 0) {
                        throw std::runtime_error("Line " + std::to_string(line.line_number) +
                                                 ": cells must divide evenly by series");
                    }
                    config.parallel = cells / config.series;
                } else if (!has_arg(line, "series") && !has_arg(line, "parallel")) {
                    config.series = cells;
                    config.parallel = 1;
                }
            }
            if (has_arg(line, "chemistry")) {
                config.chemistry = parse_chemistry(arg_string(line, "chemistry"));
            }
            auto defaults = battery::ChemistryDefaults::for_chemistry(config.chemistry);
            config.capacity_ah = arg_double(line, "capacity_ah", arg_double(line, "capacity", config.capacity_ah));
            if (config.capacity_ah <= 0.0) config.capacity_ah = defaults.typical_capacity;
            config.internal_resistance_ohm = arg_double(
                line, "internal_resistance_ohm",
                arg_double(line, "resistance_ohm", arg_double(line, "r_ohm", config.internal_resistance_ohm)));
            if (config.internal_resistance_ohm <= 0.0) {
                config.internal_resistance_ohm = defaults.typical_internal_resistance;
            }
            config.initial_soc = parse_fraction(arg_string(line, "soc", "100"), "soc");
            config.ambient_c = arg_double(line, "ambient_c", arg_double(line, "temperature_c", config.ambient_c));
            config.initial_soh = parse_fraction(arg_string(line, "soh", "100"), "soh");
        } else if (line.command == "mixture") {
            saw_mixture = true;
            config.mixture.clear();
            for (const auto& item : line.args) {
                config.mixture.push_back({parse_chemistry(item.first), parse_fraction(item.second, item.first)});
            }
        }
    }

    if (!saw_pack) {
        throw std::runtime_error("Script must include a pack line");
    }
    if (config.series < 1 || config.parallel < 1) {
        throw std::runtime_error("Pack series and parallel values must be >= 1");
    }
    if (config.series * config.parallel < 1) {
        throw std::runtime_error("Pack must contain at least one cell");
    }
    if (!saw_mixture || config.mixture.empty()) {
        config.mixture = {{config.chemistry, 1.0}};
    }

    double total_weight = 0.0;
    for (const auto& item : config.mixture) total_weight += item.second;
    if (total_weight <= 0.0) {
        throw std::runtime_error("Mixture weights must sum to more than zero");
    }
    for (auto& item : config.mixture) item.second /= total_weight;

    return config;
}

Chemistry chemistry_for_cell(const PackConfig& config, int cell_index) {
    const int total = config.series * config.parallel;
    const double position = (static_cast<double>(cell_index) + 0.5) / static_cast<double>(total);
    double cumulative = 0.0;
    for (const auto& item : config.mixture) {
        cumulative += item.second;
        if (position <= cumulative) return item.first;
    }
    return config.mixture.back().first;
}

SimPack create_pack(const PackConfig& config) {
    SimPack pack;
    pack.config = config;
    pack.ambient_c = config.ambient_c;
    pack.cells.reserve(static_cast<std::size_t>(config.series * config.parallel));

    for (int i = 0; i < config.series * config.parallel; ++i) {
        const Chemistry chemistry = chemistry_for_cell(config, i);
        const auto defaults = battery::ChemistryDefaults::for_chemistry(chemistry);
        const double capacity = config.capacity_ah > 0.0 ? config.capacity_ah : defaults.typical_capacity;
        const double resistance =
            config.internal_resistance_ohm > 0.0 ? config.internal_resistance_ohm : defaults.typical_internal_resistance;
        Cell cell("cell_" + std::to_string(i + 1), chemistry, capacity, resistance, config.initial_soc);
        pack.cells.emplace_back(cell, config.initial_soh, config.ambient_c);
    }

    return pack;
}

void print_script_help(std::ostream& out) {
    out << "Script format:\n"
        << "  pack label=demo cells=8 series=4 parallel=2 chemistry=li-ion capacity_ah=3.0 soc=100 soh=100 ambient_c=25\n"
        << "  mixture li-ion=75 lifepo4=25\n"
        << "  discharge current_a=12 duration_s=900 step_s=60\n"
        << "  rest duration_s=300 step_s=60\n"
        << "  charge current_a=4 duration_s=600 step_s=60\n\n"
        << "Commands:\n"
        << "  pack       Defines cell count, series/parallel layout, capacity, chemistry, SoC, SoH, ambient temp.\n"
        << "  mixture    Optional chemistry mix. Values can be fractions or percentages.\n"
        << "  discharge  Runs a positive pack current draw.\n"
        << "  charge     Charges the pack.\n"
        << "  rest       Advances time with thermal cooling and no current.\n"
        << "  run        Generic action: mode=discharge|charge|rest current_a=... duration_s=...\n";
}

void print_header(std::ostream& out, const SimPack& pack, bool csv) {
    if (csv) {
        out << "time_s,action,current_a,terminal_v,open_circuit_v,soc_pct,min_soc_pct,soh_pct,min_soh_pct,"
            << "temp_c,max_temp_c,delivered_wh,absorbed_wh,duration_s\n";
        return;
    }

    out << "Battery simulator\n";
    out << "Pack: " << pack.config.label << " " << pack.config.series << "S" << pack.config.parallel << "P"
        << " cells=" << pack.total_cells() << " capacity=" << pack.capacity_ah() << "Ah"
        << " mixture=";
    for (std::size_t i = 0; i < pack.config.mixture.size(); ++i) {
        if (i) out << ",";
        out << chemistry_label(pack.config.mixture[i].first) << ":"
            << std::fixed << std::setprecision(0) << (pack.config.mixture[i].second * 100.0) << "%";
    }
    out << "\n\n";
    out << std::fixed << std::setprecision(2);
    out << "time_s  action      amps     term_v   ocv      soc%    min_soc% soh%    temp_c  max_c   Wh_out  Wh_in\n";
}

void print_status(std::ostream& out, const SimPack& pack, const std::string& action, double current_a, bool csv) {
    if (csv) {
        out << std::fixed << std::setprecision(4)
            << pack.elapsed_s << ','
            << action << ','
            << current_a << ','
            << pack.terminal_voltage(current_a) << ','
            << pack.open_circuit_voltage() << ','
            << pack.average_soc() * 100.0 << ','
            << pack.min_soc() * 100.0 << ','
            << pack.average_soh() * 100.0 << ','
            << pack.min_soh() * 100.0 << ','
            << pack.average_temperature_c() << ','
            << pack.max_temperature_c() << ','
            << pack.delivered_wh << ','
            << pack.absorbed_wh << ','
            << pack.elapsed_s << '\n';
        return;
    }

    out << std::fixed << std::setprecision(2)
        << std::setw(6) << pack.elapsed_s << "  "
        << std::left << std::setw(10) << action << std::right
        << std::setw(7) << current_a
        << std::setw(9) << pack.terminal_voltage(current_a)
        << std::setw(9) << pack.open_circuit_voltage()
        << std::setw(8) << pack.average_soc() * 100.0
        << std::setw(10) << pack.min_soc() * 100.0
        << std::setw(7) << pack.average_soh() * 100.0
        << std::setw(8) << pack.average_temperature_c()
        << std::setw(7) << pack.max_temperature_c()
        << std::setw(8) << pack.delivered_wh
        << std::setw(7) << pack.absorbed_wh
        << '\n';
}

void apply_thermal_and_health(SimPack& pack, double pack_current_a, double seconds) {
    const double current_per_cell = pack.config.parallel > 0
        ? std::fabs(pack_current_a) / static_cast<double>(pack.config.parallel)
        : std::fabs(pack_current_a);
    const double hours = seconds / 3600.0;

    for (auto& sim_cell : pack.cells) {
        const double heat_c = current_per_cell * current_per_cell *
            sim_cell.cell.internal_resistance() * seconds * 0.004;
        const double cooling_c = (sim_cell.temperature_c - pack.ambient_c) *
            (1.0 - std::exp(-seconds / 900.0));
        sim_cell.temperature_c += heat_c - cooling_c;
        sim_cell.temperature_c = std::max(pack.ambient_c - 5.0, sim_cell.temperature_c);
        sim_cell.cell.set_temperature(sim_cell.temperature_c);

        const double c_rate = sim_cell.cell.capacity_ah() > 0.0
            ? current_per_cell / sim_cell.cell.capacity_ah()
            : 0.0;
        const double temp_penalty = std::max(0.0, sim_cell.temperature_c - 35.0) * 0.000002 * hours;
        const double cycle_penalty = c_rate * hours * 0.00003;
        sim_cell.soh = clamp(sim_cell.soh - temp_penalty - cycle_penalty, 0.50, 1.0);
    }
}

void advance_pack(SimPack& pack, const std::string& action, double current_a, double duration_s, double step_s,
                  bool stop_on_empty, bool csv) {
    if (duration_s < 0.0) throw std::runtime_error("duration_s must be non-negative");
    if (step_s <= 0.0) throw std::runtime_error("step_s must be positive");

    double remaining = duration_s;
    while (remaining > 1e-9) {
        const double dt = std::min(step_s, remaining);
        if (action == "discharge") {
            const double current_per_cell = current_a / static_cast<double>(pack.config.parallel);
            for (auto& sim_cell : pack.cells) {
                pack.delivered_wh += sim_cell.cell.discharge(current_per_cell, dt);
            }
            pack.throughput_ah += current_a * dt / 3600.0;
        } else if (action == "charge") {
            const double current_per_cell = current_a / static_cast<double>(pack.config.parallel);
            for (auto& sim_cell : pack.cells) {
                pack.absorbed_wh += sim_cell.cell.charge(current_per_cell, dt);
            }
            pack.throughput_ah += current_a * dt / 3600.0;
        }

        apply_thermal_and_health(pack, action == "charge" ? -current_a : current_a, dt);
        pack.elapsed_s += dt;
        print_status(std::cout, pack, action, action == "charge" ? -current_a : current_a, csv);
        remaining -= dt;

        if (stop_on_empty && action == "discharge" && pack.depleted()) {
            if (!csv) std::cout << "Pack depleted; stopping discharge early.\n";
            break;
        }
    }
}

void execute_script(const std::vector<ScriptLine>& lines, SimPack& pack, bool csv) {
    print_header(std::cout, pack, csv);
    print_status(std::cout, pack, "initial", 0.0, csv);

    for (const auto& line : lines) {
        if (line.command == "pack" || line.command == "mixture" || line.command == "output") continue;

        std::string action = line.command;
        if (action == "run") {
            action = normalize_key(arg_string(line, "mode", "discharge"));
        }
        if (action == "load") action = "discharge";

        if (action != "discharge" && action != "charge" && action != "rest") {
            throw std::runtime_error("Line " + std::to_string(line.line_number) + ": unknown command '" +
                                     line.command + "'");
        }

        const double duration_s = arg_double(line, "duration_s",
            arg_double(line, "seconds", arg_double(line, "duration", 0.0)));
        const double step_s = arg_double(line, "step_s", arg_double(line, "interval_s", std::min(60.0, duration_s)));
        const double current_a = action == "rest" ? 0.0 : arg_double(line, "current_a", arg_double(line, "amps", 0.0));
        if (action != "rest" && current_a <= 0.0) {
            throw std::runtime_error("Line " + std::to_string(line.line_number) +
                                     ": current_a must be positive for " + action);
        }

        advance_pack(pack, action, current_a, duration_s, step_s, true, csv);
    }
}

double rand_uniform(std::mt19937& rng, double lo, double hi) {
    std::uniform_real_distribution<double> dist(lo, hi);
    return dist(rng);
}

int rand_int(std::mt19937& rng, int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

Chemistry rand_chemistry(std::mt19937& rng) {
    static const Chemistry choices[] = {
        Chemistry::LITHIUM_ION,
        Chemistry::LITHIUM_POLYMER,
        Chemistry::LITHIUM_IRON_PHOSPHATE,
        Chemistry::NICKEL_METAL_HYDRIDE,
    };
    return choices[rand_int(rng, 0, static_cast<int>(sizeof(choices) / sizeof(choices[0])) - 1)];
}

ScriptLine make_script_line(const std::string& command, int line_number,
                            const std::map<std::string, std::string>& args) {
    ScriptLine line;
    line.command = normalize_key(command);
    line.line_number = line_number;
    line.args = args;
    return line;
}

void print_starting_conditions(std::ostream& out, const PackConfig& config) {
    out << "Random test starting conditions\n";
    out << std::fixed << std::setprecision(2);
    out << "  label: " << config.label << '\n';
    out << "  layout: " << config.series << "S" << config.parallel << "P"
        << " (" << config.series * config.parallel << " cells)\n";
    out << "  chemistry: " << chemistry_label(config.chemistry) << '\n';
    out << "  capacity_ah: " << config.capacity_ah << '\n';
    out << "  internal_resistance_ohm: " << config.internal_resistance_ohm << '\n';
    out << "  soc: " << config.initial_soc * 100.0 << "%\n";
    out << "  soh: " << config.initial_soh * 100.0 << "%\n";
    out << "  ambient_c: " << config.ambient_c << " C\n";
    out << "  mixture:";
    for (const auto& item : config.mixture) {
        out << ' ' << chemistry_label(item.first) << '=' << item.second * 100.0 << '%';
    }
    out << "\n\n";
}

struct RandomTestPlan {
    PackConfig config;
    std::vector<ScriptLine> lines;
};

RandomTestPlan generate_random_test(std::mt19937& rng) {
    RandomTestPlan plan;
    PackConfig& config = plan.config;

    config.label = "random_test";
    config.series = rand_int(rng, 1, 6);
    config.parallel = rand_int(rng, 1, 4);
    config.chemistry = rand_chemistry(rng);

    const auto defaults = battery::ChemistryDefaults::for_chemistry(config.chemistry);
    config.capacity_ah = defaults.typical_capacity * rand_uniform(rng, 0.6, 1.8);
    config.internal_resistance_ohm = defaults.typical_internal_resistance * rand_uniform(rng, 0.7, 1.5);
    config.initial_soc = rand_uniform(rng, 0.20, 1.0);
    config.initial_soh = rand_uniform(rng, 0.75, 1.0);
    config.ambient_c = rand_uniform(rng, -5.0, 40.0);

    if (rand_uniform(rng, 0.0, 1.0) < 0.7 || config.chemistry == Chemistry::NICKEL_METAL_HYDRIDE) {
        config.mixture = {{config.chemistry, 1.0}};
    } else {
        Chemistry secondary = rand_chemistry(rng);
        while (secondary == config.chemistry) secondary = rand_chemistry(rng);
        const double primary_weight = rand_uniform(rng, 0.55, 0.85);
        config.mixture = {{config.chemistry, primary_weight}, {secondary, 1.0 - primary_weight}};
    }

    const double pack_capacity_ah = config.capacity_ah * static_cast<double>(config.parallel);
    const double discharge_c_rate = rand_uniform(rng, 0.5, 1.5);
    const double charge_c_rate = rand_uniform(rng, 0.25, 0.6);
    const double discharge_current_a = pack_capacity_ah * discharge_c_rate;
    const double charge_current_a = pack_capacity_ah * charge_c_rate;
    const double discharge_duration_s = rand_uniform(rng, 180.0, 480.0);
    const double rest_duration_s = rand_uniform(rng, 60.0, 180.0);
    const double charge_duration_s = rand_uniform(rng, 120.0, 360.0);

    plan.lines = {
        make_script_line("discharge", 1,
            {{"current_a", std::to_string(discharge_current_a)},
             {"duration_s", std::to_string(discharge_duration_s)},
             {"step_s", "60"}}),
        make_script_line("rest", 2,
            {{"duration_s", std::to_string(rest_duration_s)},
             {"step_s", "60"}}),
        make_script_line("charge", 3,
            {{"current_a", std::to_string(charge_current_a)},
             {"duration_s", std::to_string(charge_duration_s)},
             {"step_s", "60"}}),
    };

    return plan;
}

void run_random_test(bool csv) {
    std::random_device seed_source;
    std::mt19937 rng(seed_source());

    const RandomTestPlan plan = generate_random_test(rng);
    print_starting_conditions(std::cout, plan.config);

    auto pack = create_pack(plan.config);
    execute_script(plan.lines, pack, csv);
}

} // namespace

int main(int argc, const char* const argv[]) {
    os_generics::cli::CommandLineParser parser;
    parser.set_program_name("battery_simulator");
    parser.set_description("Runs a scripted battery pack simulation with temperature, SoH, SoC, and duration output.");
    parser.add_option({"script", 's', true, false, "PATH", "Path to a battery simulation script."});
    parser.add_option({"random-test", 'r', false, false, "", "Run with random starting conditions and a built-in test profile."});
    parser.add_option({"csv", '\0', false, false, "", "Emit CSV instead of a fixed-width table."});
    parser.add_option({"help", 'h', false, false, "", "Show help."});
    parser.add_option({"script-help", '\0', false, false, "", "Show the line-oriented script format."});

    auto args = os_generics::cli::argv_to_vector(argc, argv);
    for (auto& arg : args) {
        if (arg == "-rt") arg = "--random-test";
    }

    const auto parsed = parser.parse(args);
    if (parsed.has_option("help")) {
        std::cout << parser.render_help() << "\n\n";
        print_script_help(std::cout);
        return 0;
    }
    if (parsed.has_option("script-help")) {
        print_script_help(std::cout);
        return 0;
    }
    if (!parsed.ok()) {
        for (const auto& error : parsed.errors) std::cerr << error << '\n';
        std::cerr << parser.render_help() << '\n';
        return 2;
    }

    if (parsed.has_option("random-test")) {
        try {
            run_random_test(parsed.has_option("csv"));
        } catch (const std::exception& e) {
            std::cerr << "battery_simulator: " << e.what() << '\n';
            return 1;
        }
        return 0;
    }

    std::string script_path = parsed.option_value("script");
    if (script_path.empty() && !parsed.positionals.empty()) {
        script_path = parsed.positionals.front();
    }
    if (script_path.empty()) {
        std::cerr << "No script supplied. Use --script PATH or pass PATH as the first argument.\n\n";
        std::cerr << parser.render_help() << '\n';
        return 2;
    }

    try {
        const auto lines = load_script(script_path);
        const auto config = parse_pack_config(lines);
        auto pack = create_pack(config);
        execute_script(lines, pack, parsed.has_option("csv"));
    } catch (const std::exception& e) {
        std::cerr << "battery_simulator: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
