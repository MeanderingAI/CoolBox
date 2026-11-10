#include "decoder_language_model.h"
#include "tensor_backend.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ml::deep_learning;
using namespace ml::deep_learning::llm;

namespace {

struct Arguments {
    std::string workload = "tiny";
    std::string backend = "cpu";
    std::string precision = "auto";
    std::size_t warmup_steps = 2;
    std::size_t steps = 10;
    std::size_t batch_size = 1;
    std::size_t sequence_length = 0;
    std::uint64_t seed = 1337;
    std::optional<double> target_loss;
    std::filesystem::path json_output = "llm_benchmark.json";
    std::filesystem::path csv_output = "llm_benchmark.csv";
};

struct Workload {
    std::string name;
    DecoderLanguageModelConfig config;
};

struct Result {
    std::string workload;
    std::string backend;
    std::string device;
    std::string precision;
    std::string timestamp_utc;
    std::size_t warmup_steps = 0;
    std::size_t measured_steps = 0;
    std::size_t batch_size = 0;
    std::size_t sequence_length = 0;
    std::size_t parameter_count = 0;
    std::size_t predicted_tokens = 0;
    double elapsed_seconds = 0.0;
    double sequences_per_second = 0.0;
    double tokens_per_second = 0.0;
    double mean_step_ms = 0.0;
    double p50_step_ms = 0.0;
    double p90_step_ms = 0.0;
    double p99_step_ms = 0.0;
    double initial_loss = 0.0;
    double final_loss = 0.0;
    double minimum_loss = 0.0;
    std::optional<double> target_loss;
    std::optional<std::size_t> target_step;
    std::optional<double> time_to_target_seconds;
};

std::size_t parse_size(
    const std::string& value,
    const std::string& option,
    bool allow_zero = false) {
    std::size_t consumed = 0;
    const unsigned long long parsed = std::stoull(value, &consumed);
    if (consumed != value.size() ||
        (!allow_zero && parsed == 0)) {
        throw std::invalid_argument(
            option + (allow_zero
                ? " must be a nonnegative integer"
                : " must be a positive integer"));
    }
    return static_cast<std::size_t>(parsed);
}

Arguments parse_arguments(int argc, char** argv) {
    Arguments arguments;
    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        auto value = [&]() -> std::string {
            if (++index >= argc) {
                throw std::invalid_argument(
                    "Missing value for " + option);
            }
            return argv[index];
        };
        if (option == "--workload") {
            arguments.workload = value();
        } else if (option == "--backend") {
            arguments.backend = value();
        } else if (option == "--precision") {
            arguments.precision = value();
        } else if (option == "--warmup-steps") {
            arguments.warmup_steps =
                parse_size(value(), option, true);
        } else if (option == "--steps") {
            arguments.steps = parse_size(value(), option);
        } else if (option == "--batch-size") {
            arguments.batch_size = parse_size(value(), option);
        } else if (option == "--sequence-length") {
            arguments.sequence_length =
                parse_size(value(), option);
        } else if (option == "--seed") {
            arguments.seed = parse_size(value(), option, true);
        } else if (option == "--target-loss") {
            arguments.target_loss = std::stod(value());
        } else if (option == "--json-output") {
            arguments.json_output = value();
        } else if (option == "--csv-output") {
            arguments.csv_output = value();
        } else if (option == "--help") {
            std::cout
                << "CoolBox MLPerf-style LLM benchmark (non-compliant)\n"
                << "  --workload tiny|small|base\n"
                << "  --backend cpu|vulkan\n"
                << "  --precision auto|fp64|fp32|fp16|bf16\n"
                << "  --warmup-steps N --steps N\n"
                << "  --batch-size N\n"
                << "  --sequence-length N --seed N\n"
                << "  --target-loss VALUE\n"
                << "  --json-output PATH --csv-output PATH\n";
            std::exit(0);
        } else {
            throw std::invalid_argument(
                "Unknown option: " + option);
        }
    }
    return arguments;
}

Workload make_workload(
    const std::string& name,
    std::uint64_t seed) {
    DecoderLanguageModelConfig config;
    config.vocab_size = 260;
    config.dropout_rate = 0.0;
    config.optimizer = LanguageModelOptimizer::ADAMW;
    config.learning_rate = 3e-4;
    config.weight_decay = 0.01;
    config.gradient_clip_norm = 1.0;
    config.seed = seed;
    if (name == "tiny") {
        config.d_model = 64;
        config.num_heads = 4;
        config.d_ff = 128;
        config.num_layers = 2;
        config.max_sequence_length = 32;
    } else if (name == "small") {
        config.d_model = 256;
        config.num_heads = 8;
        config.d_ff = 1024;
        config.num_layers = 4;
        config.max_sequence_length = 128;
    } else if (name == "base") {
        config.d_model = 768;
        config.num_heads = 12;
        config.d_ff = 3072;
        config.num_layers = 12;
        config.max_sequence_length = 512;
    } else {
        throw std::invalid_argument(
            "Unknown workload: " + name);
    }
    return {"coolbox_decoder_" + name, config};
}

TensorPrecision parse_precision(const std::string& name) {
    if (name == "auto") return TensorPrecision::AUTO;
    if (name == "fp64") return TensorPrecision::FP64;
    if (name == "fp32") return TensorPrecision::FP32;
    if (name == "fp16") return TensorPrecision::FP16;
    if (name == "bf16") return TensorPrecision::BF16;
    throw std::invalid_argument("Unknown precision: " + name);
}

std::string resolved_precision_name(TensorPrecision precision) {
    if (precision == TensorPrecision::FP64) return "fp64";
    if (precision == TensorPrecision::FP32) return "fp32";
    if (precision == TensorPrecision::FP16) return "fp16";
    if (precision == TensorPrecision::BF16) return "bf16";
    return "auto";
}

std::vector<std::size_t> make_sequence(
    std::size_t predicted_tokens,
    std::size_t step,
    std::uint64_t seed) {
    std::vector<std::size_t> tokens(predicted_tokens + 1);
    std::uint64_t state =
        seed + static_cast<std::uint64_t>(step) *
            0x9e3779b97f4a7c15ULL;
    for (std::size_t& token : tokens) {
        state = state * 6364136223846793005ULL + 1;
        token = 4 + static_cast<std::size_t>(state % 256);
    }
    return tokens;
}

double percentile(
    std::vector<double> values,
    double fraction) {
    std::sort(values.begin(), values.end());
    const double position =
        fraction * static_cast<double>(values.size() - 1);
    const std::size_t lower =
        static_cast<std::size_t>(std::floor(position));
    const std::size_t upper =
        static_cast<std::size_t>(std::ceil(position));
    const double weight = position - lower;
    return values[lower] * (1.0 - weight) +
        values[upper] * weight;
}

std::string utc_timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm value{};
#ifdef _WIN32
    gmtime_s(&value, &now);
#else
    gmtime_r(&now, &value);
#endif
    std::ostringstream stream;
    stream << std::put_time(&value, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

std::string json_escape(const std::string& value) {
    std::ostringstream stream;
    for (const char character : value) {
        if (character == '"' || character == '\\') {
            stream << '\\';
        }
        stream << character;
    }
    return stream.str();
}

void write_json(
    const Result& result,
    const std::filesystem::path& path) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error(
            "Unable to write JSON result: " + path.string());
    }
    output << std::setprecision(12)
           << "{\n"
           << "  \"benchmark\": \"mlperf_style_llm_training\",\n"
           << "  \"mlperf_compliant\": false,\n"
           << "  \"workload\": \"" << json_escape(result.workload) << "\",\n"
           << "  \"backend\": \"" << result.backend << "\",\n"
           << "  \"device\": \"" << json_escape(result.device) << "\",\n"
           << "  \"precision\": \"" << result.precision << "\",\n"
           << "  \"timestamp_utc\": \"" << result.timestamp_utc << "\",\n"
           << "  \"warmup_steps\": " << result.warmup_steps << ",\n"
           << "  \"measured_steps\": " << result.measured_steps << ",\n"
           << "  \"batch_size\": " << result.batch_size << ",\n"
           << "  \"sequence_length\": " << result.sequence_length << ",\n"
           << "  \"parameter_count\": " << result.parameter_count << ",\n"
           << "  \"predicted_tokens\": " << result.predicted_tokens << ",\n"
           << "  \"elapsed_seconds\": " << result.elapsed_seconds << ",\n"
           << "  \"sequences_per_second\": "
           << result.sequences_per_second << ",\n"
           << "  \"tokens_per_second\": " << result.tokens_per_second << ",\n"
           << "  \"mean_step_ms\": " << result.mean_step_ms << ",\n"
           << "  \"p50_step_ms\": " << result.p50_step_ms << ",\n"
           << "  \"p90_step_ms\": " << result.p90_step_ms << ",\n"
           << "  \"p99_step_ms\": " << result.p99_step_ms << ",\n"
           << "  \"initial_loss\": " << result.initial_loss << ",\n"
           << "  \"final_loss\": " << result.final_loss << ",\n"
           << "  \"minimum_loss\": " << result.minimum_loss << ",\n"
           << "  \"target_loss\": ";
    if (result.target_loss) output << *result.target_loss;
    else output << "null";
    output << ",\n  \"target_step\": ";
    if (result.target_step) output << *result.target_step;
    else output << "null";
    output << ",\n  \"time_to_target_seconds\": ";
    if (result.time_to_target_seconds) {
        output << *result.time_to_target_seconds;
    } else {
        output << "null";
    }
    output << "\n}\n";
}

void write_csv(
    const Result& result,
    const std::filesystem::path& path) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error(
            "Unable to write CSV result: " + path.string());
    }
    output
        << "benchmark,mlperf_compliant,workload,backend,device,precision,"
        << "timestamp_utc,warmup_steps,measured_steps,sequence_length,"
        << "batch_size,"
        << "parameter_count,predicted_tokens,elapsed_seconds,"
        << "sequences_per_second,tokens_per_second,mean_step_ms,"
        << "p50_step_ms,p90_step_ms,p99_step_ms,initial_loss,final_loss,"
        << "minimum_loss,target_loss,target_step,time_to_target_seconds\n";
    output << std::setprecision(12)
           << "mlperf_style_llm_training,false,"
           << result.workload << ','
           << result.backend << ",\""
           << result.device << "\","
           << result.precision << ','
           << result.timestamp_utc << ','
           << result.warmup_steps << ','
           << result.measured_steps << ','
           << result.sequence_length << ','
           << result.batch_size << ','
           << result.parameter_count << ','
           << result.predicted_tokens << ','
           << result.elapsed_seconds << ','
           << result.sequences_per_second << ','
           << result.tokens_per_second << ','
           << result.mean_step_ms << ','
           << result.p50_step_ms << ','
           << result.p90_step_ms << ','
           << result.p99_step_ms << ','
           << result.initial_loss << ','
           << result.final_loss << ','
           << result.minimum_loss << ',';
    if (result.target_loss) output << *result.target_loss;
    output << ',';
    if (result.target_step) output << *result.target_step;
    output << ',';
    if (result.time_to_target_seconds) {
        output << *result.time_to_target_seconds;
    }
    output << '\n';
}

Result run_benchmark(const Arguments& arguments) {
    Workload workload =
        make_workload(arguments.workload, arguments.seed);
    const std::size_t sequence_length =
        arguments.sequence_length == 0
            ? workload.config.max_sequence_length
            : arguments.sequence_length;
    if (sequence_length > workload.config.max_sequence_length) {
        throw std::invalid_argument(
            "sequence length exceeds workload maximum");
    }

    TensorBackend backend = TensorBackend::CPU;
    if (arguments.backend == "vulkan") {
        backend = TensorBackend::VULKAN;
    } else if (arguments.backend != "cpu") {
        throw std::invalid_argument(
            "Unknown backend: " + arguments.backend);
    }
    if (!tensor_backend_available(backend)) {
        throw std::runtime_error(
            tensor_backend_unavailable_reason(backend));
    }
    const TensorPrecision precision =
        parse_precision(arguments.precision);
    if (!tensor_precision_available(precision)) {
        throw std::runtime_error(
            "Requested precision is unavailable");
    }
    set_tensor_backend(backend);
    set_tensor_precision(precision);

    DecoderLanguageModel model(workload.config);
    auto make_batch = [&](std::size_t step) {
        std::vector<std::vector<std::size_t>> batch;
        batch.reserve(arguments.batch_size);
        for (std::size_t item = 0;
             item < arguments.batch_size;
             ++item) {
            batch.push_back(make_sequence(
                sequence_length,
                step * arguments.batch_size + item,
                arguments.seed));
        }
        return batch;
    };
    for (std::size_t step = 0;
         step < arguments.warmup_steps;
         ++step) {
        model.train_batch(make_batch(step));
    }

    std::vector<double> losses;
    std::vector<double> step_milliseconds;
    losses.reserve(arguments.steps);
    step_milliseconds.reserve(arguments.steps);
    const auto measured_start =
        std::chrono::steady_clock::now();
    std::optional<std::size_t> target_step;
    std::optional<double> time_to_target;
    for (std::size_t step = 0; step < arguments.steps; ++step) {
        const auto step_start =
            std::chrono::steady_clock::now();
        const double loss = model.train_batch(
            make_batch(arguments.warmup_steps + step));
        const auto step_end =
            std::chrono::steady_clock::now();
        losses.push_back(loss);
        step_milliseconds.push_back(
            std::chrono::duration<double, std::milli>(
                step_end - step_start).count());
        if (arguments.target_loss &&
            !target_step &&
            loss <= *arguments.target_loss) {
            target_step = step + 1;
            time_to_target =
                std::chrono::duration<double>(
                    step_end - measured_start).count();
        }
    }
    const double elapsed =
        std::chrono::duration<double>(
            std::chrono::steady_clock::now() -
            measured_start).count();
    const std::size_t predicted_tokens =
        arguments.steps * arguments.batch_size * sequence_length;

    Result result;
    result.workload = workload.name;
    result.backend = arguments.backend;
    result.device = tensor_backend_device_name(backend);
    result.precision = resolved_precision_name(
        active_tensor_precision());
    result.timestamp_utc = utc_timestamp();
    result.warmup_steps = arguments.warmup_steps;
    result.measured_steps = arguments.steps;
    result.batch_size = arguments.batch_size;
    result.sequence_length = sequence_length;
    result.parameter_count = model.parameter_count();
    result.predicted_tokens = predicted_tokens;
    result.elapsed_seconds = elapsed;
    result.sequences_per_second =
        static_cast<double>(
            arguments.steps * arguments.batch_size) / elapsed;
    result.tokens_per_second =
        static_cast<double>(predicted_tokens) / elapsed;
    result.mean_step_ms = std::accumulate(
        step_milliseconds.begin(),
        step_milliseconds.end(), 0.0) /
        static_cast<double>(step_milliseconds.size());
    result.p50_step_ms = percentile(step_milliseconds, 0.50);
    result.p90_step_ms = percentile(step_milliseconds, 0.90);
    result.p99_step_ms = percentile(step_milliseconds, 0.99);
    result.initial_loss = losses.front();
    result.final_loss = losses.back();
    result.minimum_loss =
        *std::min_element(losses.begin(), losses.end());
    result.target_loss = arguments.target_loss;
    result.target_step = target_step;
    result.time_to_target_seconds = time_to_target;
    return result;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Arguments arguments = parse_arguments(argc, argv);
        const Result result = run_benchmark(arguments);
        write_json(result, arguments.json_output);
        write_csv(result, arguments.csv_output);
        std::cout << std::fixed << std::setprecision(3)
                  << "workload=" << result.workload
                  << " backend=" << result.backend
                  << " precision=" << result.precision
                  << " parameters=" << result.parameter_count
                  << " tokens_per_second=" << result.tokens_per_second
                  << " mean_step_ms=" << result.mean_step_ms
                  << " final_loss=" << result.final_loss
                  << '\n'
                  << "json=" << arguments.json_output.string()
                  << " csv=" << arguments.csv_output.string()
                  << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "benchmark error: " << error.what() << '\n';
        return 1;
    }
}
