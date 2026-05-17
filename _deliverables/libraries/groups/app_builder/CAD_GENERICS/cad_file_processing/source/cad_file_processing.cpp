#include "cad_file_processing.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace app_builder {
namespace cad_generics {
namespace {

constexpr const char* kCurrentHkCadSchemaVersion = "1.1";

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string normalize_extension(const std::string& extension_or_path) {
    std::filesystem::path p(extension_or_path);
    std::string ext = p.has_extension() ? p.extension().string() : extension_or_path;

    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }

    return to_lower(ext);
}

CadFileType extension_to_type(const std::string& ext) {
    if (ext == "dwg") return CadFileType::Dwg;
    if (ext == "step" || ext == "stp") return CadFileType::Step;
    if (ext == "stl") return CadFileType::Stl;
    if (ext == "sldprt") return CadFileType::Sldprt;
    if (ext == "sldasm") return CadFileType::Sldasm;
    if (ext == "slddrw") return CadFileType::Slddrw;
    if (ext == "rvt") return CadFileType::Rvt;
    if (ext == "iges" || ext == "igs") return CadFileType::Iges;
    if (ext == "sat") return CadFileType::Sat;
    if (ext == "catpart") return CadFileType::CatPart;
    if (ext == "catproduct") return CadFileType::CatProduct;
    if (ext == "ipt") return CadFileType::Ipt;
    if (ext == "aim") return CadFileType::Aim;
    if (ext == "3dm") return CadFileType::Rhino3dm;
    if (ext == "jt") return CadFileType::Jt;
    return CadFileType::Unknown;
}

std::string hex_encode(const std::vector<unsigned char>& bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char b : bytes) {
        out << std::setw(2) << static_cast<int>(b);
    }
    return out.str();
}

bool hex_decode(const std::string& hex, std::vector<unsigned char>& out_bytes) {
    if (hex.size() % 2 != 0U) {
        return false;
    }

    out_bytes.clear();
    out_bytes.reserve(hex.size() / 2U);

    for (std::size_t i = 0; i < hex.size(); i += 2U) {
        const std::string byte_str = hex.substr(i, 2U);
        unsigned int value = 0U;
        std::istringstream in(byte_str);
        in >> std::hex >> value;
        if (in.fail()) {
            return false;
        }
        out_bytes.push_back(static_cast<unsigned char>(value & 0xFFU));
    }

    return true;
}

bool read_file_bytes(const std::string& path, std::vector<unsigned char>& out_bytes) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    in.seekg(0, std::ios::end);
    const std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    if (size < 0) {
        return false;
    }

    out_bytes.resize(static_cast<std::size_t>(size));
    if (size > 0) {
        in.read(reinterpret_cast<char*>(out_bytes.data()), size);
    }

    return in.good() || in.eof();
}

bool set_error(std::string* error_message, const std::string& message) {
    if (error_message != nullptr) {
        *error_message = message;
    }
    return false;
}

bool migrate_0_9_to_1_0(HkCadDocument& document, std::string* error_message) {
    (void)error_message;
    if (document.schema_version != "0.9") {
        return true;
    }
    if (document.metadata.find("source_type") == document.metadata.end()) {
        document.metadata["source_type"] = cad_file_type_to_string(document.source_type);
    }
    if (document.metadata.find("migration") == document.metadata.end()) {
        document.metadata["migration"] = "0.9->1.0";
    }
    document.schema_version = "1.0";
    return true;
}

bool migrate_1_0_to_1_1(HkCadDocument& document, std::string* error_message) {
    (void)error_message;
    if (document.schema_version != "1.0") {
        return true;
    }
    document.metadata["payload_size_bytes"] = std::to_string(document.payload.size());
    if (document.metadata.find("migration") == document.metadata.end()) {
        document.metadata["migration"] = "1.0->1.1";
    } else {
        document.metadata["migration"] += ";1.0->1.1";
    }
    document.schema_version = "1.1";
    return true;
}

HkCadMigrationRegistry build_default_migration_registry() {
    HkCadMigrationRegistry registry;
    registry.register_migration("0.9", "1.0", migrate_0_9_to_1_0);
    registry.register_migration("1.0", "1.1", migrate_1_0_to_1_1);
    return registry;
}

} // namespace

std::string current_hk_cad_schema_version() {
    return kCurrentHkCadSchemaVersion;
}

CadFileType detect_cad_file_type(const std::string& path) {
    return extension_to_type(normalize_extension(path));
}

std::string cad_file_type_to_string(CadFileType type) {
    switch (type) {
        case CadFileType::Dwg: return "dwg";
        case CadFileType::Step: return "step";
        case CadFileType::Stl: return "stl";
        case CadFileType::Sldprt: return "sldprt";
        case CadFileType::Sldasm: return "sldasm";
        case CadFileType::Slddrw: return "slddrw";
        case CadFileType::Rvt: return "rvt";
        case CadFileType::Iges: return "iges";
        case CadFileType::Sat: return "sat";
        case CadFileType::CatPart: return "catpart";
        case CadFileType::CatProduct: return "catproduct";
        case CadFileType::Ipt: return "ipt";
        case CadFileType::Aim: return "aim";
        case CadFileType::Rhino3dm: return "3dm";
        case CadFileType::Jt: return "jt";
        case CadFileType::Unknown:
        default:
            return "unknown";
    }
}

bool is_supported_cad_extension(const std::string& extension) {
    return detect_cad_file_type(extension) != CadFileType::Unknown;
}

CadFileInfo inspect_cad_file(const std::string& path) {
    CadFileInfo info;
    info.path = path;

    const std::filesystem::path fs_path(path);
    info.extension = normalize_extension(path);
    info.type = extension_to_type(info.extension);
    info.exists = std::filesystem::exists(fs_path);

    if (info.exists && std::filesystem::is_regular_file(fs_path)) {
        info.file_size_bytes = static_cast<std::size_t>(std::filesystem::file_size(fs_path));
    }

    return info;
}

ProcessResult process_cad_file(const std::string& path, const ProcessingOptions& options) {
    ProcessResult result;
    result.file = inspect_cad_file(path);

    if (options.strict_extension_check && result.file.type == CadFileType::Unknown) {
        result.success = false;
        result.message = "Unsupported CAD extension: " + result.file.extension;
        return result;
    }

    if (options.require_file_to_exist && !result.file.exists) {
        result.success = false;
        result.message = "File does not exist: " + result.file.path;
        return result;
    }

    result.success = true;
    result.message = "CAD file accepted for processing as type: " + cad_file_type_to_string(result.file.type);
    return result;
}

ProcessResult create_hk_cad_document(
    const std::string& path,
    HkCadDocument& out_document,
    const ProcessingOptions& options) {

    ProcessResult result = process_cad_file(path, options);
    if (!result.success) {
        return result;
    }

    out_document = HkCadDocument{};
    out_document.source_type = result.file.type;
    out_document.source_path = result.file.path;
    out_document.metadata["source_extension"] = result.file.extension;
    out_document.metadata["source_type"] = cad_file_type_to_string(result.file.type);
    out_document.metadata["exists_at_ingest"] = result.file.exists ? "true" : "false";

    if (result.file.exists) {
        std::vector<unsigned char> bytes;
        if (!read_file_bytes(path, bytes)) {
            result.success = false;
            result.message = "Failed to read CAD file bytes: " + path;
            return result;
        }
        out_document.payload = std::move(bytes);
    }

    result.message = "CAD file normalized to hk_cad document";
    return result;
}

bool save_hk_cad_file(
    const HkCadDocument& document,
    const std::string& output_path,
    std::string* error_message) {

    HkCadDocument to_save = document;
    if (to_save.schema_version.empty()) {
        to_save.schema_version = current_hk_cad_schema_version();
    }

    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return set_error(error_message, "Unable to open output file: " + output_path);
    }

    out << "HKCAD1\n";
    out << "schema=" << to_save.schema_version << "\n";
    out << "source_type=" << cad_file_type_to_string(to_save.source_type) << "\n";
    out << "source_path=" << to_save.source_path << "\n";
    out << "metadata_count=" << to_save.metadata.size() << "\n";
    for (const auto& entry : to_save.metadata) {
        out << "meta=" << entry.first << "=" << entry.second << "\n";
    }
    out << "payload_hex=" << hex_encode(to_save.payload) << "\n";

    if (!out) {
        return set_error(error_message, "Failed while writing hk_cad file: " + output_path);
    }

    return true;
}

bool load_hk_cad_file(
    const std::string& input_path,
    HkCadDocument& out_document,
    std::string* error_message) {

    std::ifstream in(input_path, std::ios::binary);
    if (!in) {
        return set_error(error_message, "Unable to open hk_cad file: " + input_path);
    }

    out_document = HkCadDocument{};

    std::string line;
    if (!std::getline(in, line) || (line != "HKCAD1" && line != "HKCAD0")) {
        return set_error(error_message, "Invalid hk_cad header");
    }

    if (line == "HKCAD0") {
        out_document.schema_version = "0.9";
    }

    while (std::getline(in, line)) {
        const std::size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, equals_pos);
        const std::string value = line.substr(equals_pos + 1U);

        if (key == "schema") {
            out_document.schema_version = value;
        } else if (key == "source_type") {
            out_document.source_type = detect_cad_file_type("dummy." + value);
        } else if (key == "source_path") {
            out_document.source_path = value;
        } else if (key == "meta") {
            const std::size_t key_split = value.find('=');
            if (key_split != std::string::npos) {
                out_document.metadata[value.substr(0, key_split)] = value.substr(key_split + 1U);
            }
        } else if (key == "payload_hex") {
            if (!hex_decode(value, out_document.payload)) {
                return set_error(error_message, "Invalid payload_hex in hk_cad file");
            }
        }
    }

    return migrate_hk_cad_document_to_latest(out_document, error_message);
}

bool migrate_hk_cad_document_to_latest(
    HkCadDocument& document,
    std::string* error_message) {

    HkCadMigrationRegistry registry = build_default_migration_registry();
    return registry.migrate_to_latest(document, error_message);
}

ProcessResult process_and_export_hk_cad(
    const std::string& input_path,
    const std::string& output_hk_cad_path,
    const ProcessingOptions& options) {

    ProcessResult result;
    HkCadDocument document;
    result = create_hk_cad_document(input_path, document, options);
    if (!result.success) {
        return result;
    }

    std::string error;
    if (!save_hk_cad_file(document, output_hk_cad_path, &error)) {
        result.success = false;
        result.message = error;
        return result;
    }

    result.message = "Exported hk_cad file: " + output_hk_cad_path;
    return result;
}

void HkCadMigrationRegistry::register_migration(
    const std::string& from_schema,
    const std::string& to_schema,
    HkCadMigrator migrator) {

    if (from_schema.empty() || to_schema.empty() || !migrator) {
        throw std::invalid_argument("Invalid hk_cad migration registration");
    }
    migrations_.push_back(MigrationEdge{from_schema, to_schema, std::move(migrator)});
}

bool HkCadMigrationRegistry::migrate_to_latest(
    HkCadDocument& document,
    std::string* error_message) const {

    if (document.schema_version.empty()) {
        document.schema_version = "0.9";
    }

    const std::string target = current_hk_cad_schema_version();
    if (document.schema_version == target) {
        return true;
    }

    std::size_t guard = 0U;
    constexpr std::size_t kMaxSteps = 16U;
    while (document.schema_version != target && guard < kMaxSteps) {
        bool progressed = false;
        for (const auto& edge : migrations_) {
            if (edge.from == document.schema_version) {
                if (!edge.migrator(document, error_message)) {
                    return false;
                }
                if (document.schema_version != edge.to) {
                    document.schema_version = edge.to;
                }
                progressed = true;
                break;
            }
        }

        if (!progressed) {
            return set_error(error_message,
                             "No hk_cad migration path from schema " + document.schema_version +
                             " to " + target);
        }
        ++guard;
    }

    if (document.schema_version != target) {
        return set_error(error_message, "hk_cad migration exceeded step limit");
    }

    return true;
}

void CadProcessingRegistry::register_processor(CadFileType type, CadProcessor processor) {
    if (type == CadFileType::Unknown) {
        throw std::invalid_argument("Cannot register processor for unknown CAD type");
    }
    processors_[type] = std::move(processor);
}

bool CadProcessingRegistry::has_processor(CadFileType type) const {
    return processors_.find(type) != processors_.end();
}

ProcessResult CadProcessingRegistry::process_with_registered_handler(
    const std::string& path,
    const ProcessingOptions& options) const {

    ProcessResult fallback = process_cad_file(path, options);
    if (!fallback.success) {
        return fallback;
    }

    const auto it = processors_.find(fallback.file.type);
    if (it == processors_.end()) {
        return fallback;
    }

    return it->second(fallback.file);
}

} // namespace cad_generics
} // namespace app_builder
