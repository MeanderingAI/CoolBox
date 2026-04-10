/**
 * @file test_metadata_management.cpp
 * @brief Tests for the metadata management macros.
 */

#include "tyst_framework.hpp"
#include "lib_metadata.h"

// Register test-specific metadata using the macros
LIBRARY_METADATA(test_lib,
                 "test_library",
                 "0.1.0",
                 "A test library for metadata verification",
                 "TestAuthor")

LIBRARY_DOC("Additional documentation string for the test library")

FUNCTION_DOC(sample_function, "A sample function used for testing metadata")

TEST(MetadataManagementTest, LibraryNameIsCorrect) {
    const char* name = get_test_lib_library_name();
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "test_library");
}

TEST(MetadataManagementTest, LibraryVersionIsCorrect) {
    const char* version = get_test_lib_library_version();
    ASSERT_NE(version, nullptr);
    EXPECT_STREQ(version, "0.1.0");
}

TEST(MetadataManagementTest, LibraryDescriptionIsCorrect) {
    const char* desc = get_test_lib_library_description();
    ASSERT_NE(desc, nullptr);
    EXPECT_STREQ(desc, "A test library for metadata verification");
}

TEST(MetadataManagementTest, LibraryAuthorIsCorrect) {
    const char* author = get_test_lib_library_author();
    ASSERT_NE(author, nullptr);
    EXPECT_STREQ(author, "TestAuthor");
}

TEST(MetadataManagementTest, LibraryDocIsCorrect) {
    const char* doc = get_library_doc();
    ASSERT_NE(doc, nullptr);
    EXPECT_STREQ(doc, "Additional documentation string for the test library");
}

TEST(MetadataManagementTest, FunctionDocIsCorrect) {
    const char* doc = sample_function_doc();
    ASSERT_NE(doc, nullptr);
    EXPECT_STREQ(doc, "A sample function used for testing metadata");
}
