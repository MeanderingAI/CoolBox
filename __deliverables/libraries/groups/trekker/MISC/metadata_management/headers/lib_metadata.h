/*
 * Library Metadata System
 * 
 * Embeds documentation and metadata directly into shared libraries
 * so the service_manager can extract and display it.
 */

#ifndef LIB_METADATA_H
#define LIB_METADATA_H

#include <string>

// Platform-specific section attribute for embedding metadata
#if defined(_MSC_VER)
  // MSVC: use __declspec(allocate) with a pragma section, or simply skip section embedding
  #define LIB_META_SECTION
  #define LIB_META_EXPORT __declspec(dllexport)
#elif defined(__APPLE__)
  #define LIB_META_SECTION __attribute__((used, section("__DATA,__lib_meta")))
  #define LIB_META_EXPORT __attribute__((visibility("default")))
#else
  #define LIB_META_SECTION __attribute__((used, section(".lib_meta")))
  #define LIB_META_EXPORT __attribute__((visibility("default")))
#endif

// Macro to embed library metadata and Doxygen doc block with unique function names
#define LIBRARY_METADATA_DOXYGEN(libid, name, version, description, author) \
    extern "C" { \
        LIB_META_SECTION \
        static const char _lib_##libid##_name[] = name; \
        LIB_META_SECTION \
        static const char _lib_##libid##_version[] = version; \
        LIB_META_SECTION \
        static const char _lib_##libid##_description[] = description; \
        LIB_META_SECTION \
        static const char _lib_##libid##_author[] = author; \
        LIB_META_EXPORT \
        const char* get_##libid##_library_name() { return name; } \
        LIB_META_EXPORT \
        const char* get_##libid##_library_version() { return version; } \
        LIB_META_EXPORT \
        const char* get_##libid##_library_description() { return description; } \
        LIB_META_EXPORT \
        const char* get_##libid##_library_author() { return author; } \
    }

// Simpler version - just description, with Doxygen
#define LIBRARY_DOC_DOXYGEN(description) \
    extern "C" { \
        LIB_META_EXPORT \
        const char* get_library_doc() { return description; } \
    }

// Function info metadata with Doxygen
#define FUNCTION_DOC_DOXYGEN(func_name, doc) \
    extern "C" { \
        LIB_META_EXPORT \
        const char* func_name##_doc() { return doc; } \
    }

#define LIBRARY_METADATA(libid, name, version, description, author) LIBRARY_METADATA_DOXYGEN(libid, name, version, description, author)
#define LIBRARY_DOC(description) LIBRARY_DOC_DOXYGEN(description)
#define FUNCTION_DOC(func_name, doc) FUNCTION_DOC_DOXYGEN(func_name, doc)

#endif // LIB_METADATA_H
