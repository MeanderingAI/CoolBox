#include "cad_file_processing.hpp"

#include "tyst_framework.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

using app_builder::cad_generics::CadFileType;
using app_builder::cad_generics::CadProcessingRegistry;
using app_builder::cad_generics::HkCadDocument;
using app_builder::cad_generics::ProcessingOptions;
using app_builder::cad_generics::current_hk_cad_schema_version;
using app_builder::cad_generics::create_hk_cad_document;
using app_builder::cad_generics::detect_cad_file_type;
using app_builder::cad_generics::inspect_cad_file;
using app_builder::cad_generics::is_supported_cad_extension;
using app_builder::cad_generics::load_hk_cad_file;
using app_builder::cad_generics::migrate_hk_cad_document_to_latest;
using app_builder::cad_generics::process_cad_file;
using app_builder::cad_generics::process_and_export_hk_cad;
using app_builder::cad_generics::save_hk_cad_file;

TYST_TEST(CadFileProcessingTest, DetectsRequestedExtensions) {
    TYST_ASSERT_EQ(detect_cad_file_type("sample.dwg"), CadFileType::Dwg);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.step"), CadFileType::Step);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.stl"), CadFileType::Stl);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.sldprt"), CadFileType::Sldprt);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.sldasm"), CadFileType::Sldasm);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.slddrw"), CadFileType::Slddrw);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.rvt"), CadFileType::Rvt);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.iges"), CadFileType::Iges);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.sat"), CadFileType::Sat);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.CATPart"), CadFileType::CatPart);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.CATProduct"), CadFileType::CatProduct);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.ipt"), CadFileType::Ipt);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.aim"), CadFileType::Aim);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.3dm"), CadFileType::Rhino3dm);
    TYST_ASSERT_EQ(detect_cad_file_type("sample.jt"), CadFileType::Jt);
}

TYST_TEST(CadFileProcessingTest, ReportsUnsupportedExtension) {
    TYST_ASSERT_EQ(is_supported_cad_extension("txt"), false);
    TYST_ASSERT_EQ(is_supported_cad_extension("sldprt"), true);
}

TYST_TEST(CadFileProcessingTest, ProcessesExistingFile) {
    const std::filesystem::path temp = std::filesystem::temp_directory_path() / "cad_test_file.step";

    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out << "ISO-10303-21";
    }

    const auto info = inspect_cad_file(temp.string());
    TYST_ASSERT_EQ(info.type, CadFileType::Step);
    TYST_ASSERT_EQ(info.exists, true);
    TYST_ASSERT_EQ(info.file_size_bytes > 0U, true);

    ProcessingOptions options;
    options.require_file_to_exist = true;
    const auto result = process_cad_file(temp.string(), options);
    TYST_ASSERT_EQ(result.success, true);

    std::filesystem::remove(temp);
}

TYST_TEST(CadFileProcessingTest, UsesCustomProcessorWhenRegistered) {
    CadProcessingRegistry registry;
    registry.register_processor(CadFileType::Stl, [](const app_builder::cad_generics::CadFileInfo& file) {
        app_builder::cad_generics::ProcessResult res;
        res.success = true;
        res.file = file;
        res.message = "custom STL processor";
        return res;
    });

    ProcessingOptions options;
    options.require_file_to_exist = false;

    const auto result = registry.process_with_registered_handler("mesh.stl", options);
    TYST_ASSERT_EQ(result.success, true);
    TYST_ASSERT_EQ(result.message, std::string("custom STL processor"));
}

TYST_TEST(CadFileProcessingTest, HkCadRoundTripSaveLoad) {
    HkCadDocument document;
    document.source_type = CadFileType::Step;
    document.source_path = "source.step";
    document.metadata["unit"] = "mm";
    document.payload = {0x01U, 0x7FU, 0xFFU};

    const std::filesystem::path temp = std::filesystem::temp_directory_path() / "roundtrip.hk_cad";
    std::string error;
    const bool save_ok = save_hk_cad_file(document, temp.string(), &error);
    TYST_ASSERT_EQ(save_ok, true);

    HkCadDocument loaded;
    const bool load_ok = load_hk_cad_file(temp.string(), loaded, &error);
    TYST_ASSERT_EQ(load_ok, true);
    TYST_ASSERT_EQ(loaded.source_type, CadFileType::Step);
    TYST_ASSERT_EQ(loaded.source_path, std::string("source.step"));
    TYST_ASSERT_EQ(loaded.metadata["unit"], std::string("mm"));
    TYST_ASSERT_EQ(loaded.payload.size(), static_cast<std::size_t>(3U));
    TYST_ASSERT_EQ(loaded.payload[0], static_cast<unsigned char>(0x01U));
    TYST_ASSERT_EQ(loaded.payload[1], static_cast<unsigned char>(0x7FU));
    TYST_ASSERT_EQ(loaded.payload[2], static_cast<unsigned char>(0xFFU));

    std::filesystem::remove(temp);
}

TYST_TEST(CadFileProcessingTest, CreatesAndExportsHkCadFromCadInput) {
    const std::filesystem::path cad_temp = std::filesystem::temp_directory_path() / "export_source.stl";
    const std::filesystem::path hk_temp = std::filesystem::temp_directory_path() / "exported.hk_cad";

    {
        std::ofstream out(cad_temp, std::ios::binary | std::ios::trunc);
        out << "solid sample\nendsolid sample\n";
    }

    ProcessingOptions options;
    options.require_file_to_exist = true;

    HkCadDocument document;
    const auto create_result = create_hk_cad_document(cad_temp.string(), document, options);
    TYST_ASSERT_EQ(create_result.success, true);
    TYST_ASSERT_EQ(document.source_type, CadFileType::Stl);
    TYST_ASSERT_EQ(document.payload.empty(), false);

    const auto export_result = process_and_export_hk_cad(cad_temp.string(), hk_temp.string(), options);
    TYST_ASSERT_EQ(export_result.success, true);

    HkCadDocument loaded;
    std::string error;
    const bool load_ok = load_hk_cad_file(hk_temp.string(), loaded, &error);
    TYST_ASSERT_EQ(load_ok, true);
    TYST_ASSERT_EQ(loaded.source_type, CadFileType::Stl);
    TYST_ASSERT_EQ(loaded.payload.empty(), false);

    std::filesystem::remove(cad_temp);
    std::filesystem::remove(hk_temp);
}

TYST_TEST(CadFileProcessingTest, MigratesLegacyDocumentToLatestSchema) {
    HkCadDocument legacy;
    legacy.schema_version = "0.9";
    legacy.source_type = CadFileType::Stl;
    legacy.source_path = "legacy.stl";
    legacy.payload = {0xAAU, 0xBBU};

    std::string error;
    const bool migrated = migrate_hk_cad_document_to_latest(legacy, &error);
    TYST_ASSERT_EQ(migrated, true);
    TYST_ASSERT_EQ(legacy.schema_version, current_hk_cad_schema_version());
    TYST_ASSERT_EQ(legacy.metadata.find("payload_size_bytes") != legacy.metadata.end(), true);
}

TYST_TEST(CadFileProcessingTest, LoadsLegacyHeaderAndAutoMigrates) {
    const std::filesystem::path temp = std::filesystem::temp_directory_path() / "legacy.hk_cad";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out << "HKCAD0\n";
        out << "source_type=stl\n";
        out << "source_path=legacy.stl\n";
        out << "payload_hex=aabb\n";
    }

    HkCadDocument loaded;
    std::string error;
    const bool loaded_ok = load_hk_cad_file(temp.string(), loaded, &error);
    TYST_ASSERT_EQ(loaded_ok, true);
    TYST_ASSERT_EQ(loaded.schema_version, current_hk_cad_schema_version());
    TYST_ASSERT_EQ(loaded.source_type, CadFileType::Stl);
    TYST_ASSERT_EQ(loaded.payload.size(), static_cast<std::size_t>(2U));

    std::filesystem::remove(temp);
}

} // namespace
