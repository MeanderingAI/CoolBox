#ifndef COOLBOX_APP_BUILDER_CAD_GENERICS_CAD_FILE_PROCESSING_HPP
#define COOLBOX_APP_BUILDER_CAD_GENERICS_CAD_FILE_PROCESSING_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace app_builder {
namespace cad_generics {

enum class CadFileType {
    Unknown = 0,
    Dwg,
    Step,
    Stl,
    Sldprt,
    Sldasm,
    Slddrw,
    Rvt,
    Iges,
    Sat,
    CatPart,
    CatProduct,
    Ipt,
    Aim,
    Rhino3dm,
    Jt
};

struct CadFileInfo {
    std::string path;
    std::string extension;
    CadFileType type = CadFileType::Unknown;
    bool exists = false;
    std::size_t file_size_bytes = 0U;
};

struct ProcessResult {
    bool success = false;
    CadFileInfo file;
    std::string message;
};

struct ProcessingOptions {
    bool strict_extension_check = true;
    bool require_file_to_exist = true;
};

// Internal normalized document that can be stored as .hk_cad regardless of
// the original CAD source format.
struct HkCadDocument {
    std::string schema_version = "1.0";
    CadFileType source_type = CadFileType::Unknown;
    std::string source_path;
    std::unordered_map<std::string, std::string> metadata;
    std::vector<unsigned char> payload;
};

using CadProcessor = std::function<ProcessResult(const CadFileInfo&)>;
using HkCadMigrator = std::function<bool(HkCadDocument& document, std::string* error_message)>;

CadFileType detect_cad_file_type(const std::string& path);
std::string cad_file_type_to_string(CadFileType type);
bool is_supported_cad_extension(const std::string& extension);

CadFileInfo inspect_cad_file(const std::string& path);
ProcessResult process_cad_file(const std::string& path,
                               const ProcessingOptions& options = ProcessingOptions());

// Build a normalized in-memory representation from any supported CAD input.
ProcessResult create_hk_cad_document(const std::string& path,
                                     HkCadDocument& out_document,
                                     const ProcessingOptions& options = ProcessingOptions());

// Persist/load the normalized format to/from disk.
bool save_hk_cad_file(const HkCadDocument& document,
                      const std::string& output_path,
                      std::string* error_message = nullptr);
bool load_hk_cad_file(const std::string& input_path,
                      HkCadDocument& out_document,
                      std::string* error_message = nullptr);

// Schema migration helpers for backward compatibility.
std::string current_hk_cad_schema_version();
bool migrate_hk_cad_document_to_latest(HkCadDocument& document,
                                       std::string* error_message = nullptr);

// Convenience pipeline: validate CAD input and export .hk_cad artifact.
ProcessResult process_and_export_hk_cad(const std::string& input_path,
                                        const std::string& output_hk_cad_path,
                                        const ProcessingOptions& options = ProcessingOptions());

class CadProcessingRegistry {
public:
    void register_processor(CadFileType type, CadProcessor processor);
    bool has_processor(CadFileType type) const;
    ProcessResult process_with_registered_handler(const std::string& path,
                                                  const ProcessingOptions& options = ProcessingOptions()) const;

private:
    std::unordered_map<CadFileType, CadProcessor> processors_;
};

class HkCadMigrationRegistry {
public:
    void register_migration(const std::string& from_schema,
                            const std::string& to_schema,
                            HkCadMigrator migrator);
    bool migrate_to_latest(HkCadDocument& document,
                           std::string* error_message = nullptr) const;

private:
    struct MigrationEdge {
        std::string from;
        std::string to;
        HkCadMigrator migrator;
    };
    std::vector<MigrationEdge> migrations_;
};

} // namespace cad_generics
} // namespace app_builder

#endif
