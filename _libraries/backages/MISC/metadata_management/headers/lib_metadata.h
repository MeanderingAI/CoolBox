/*
 * Library Metadata System
 * 
 * Embeds documentation and metadata directly into shared libraries
 * so the service_manager can extract and display it.
 */

#ifndef LIB_METADATA_H
#define LIB_METADATA_H

#include <string>



// Macro to embed library metadata and Doxygen doc block with unique function names
#define LIBRARY_METADATA_DOXYGEN(libid, name, version, description, author) \
    /** \addtogroup Libraries \{ \brief """ name """ (v" version ")\n *  \details " description "\n *  \author " author "\n */ \
    extern "C" { \
        __attribute__((used, section("__DATA,__lib_meta"))) \
        static const char _lib_##libid##_name[] = name; \
        __attribute__((used, section("__DATA,__lib_meta"))) \
        static const char _lib_##libid##_version[] = version; \
        __attribute__((used, section("__DATA,__lib_meta"))) \
        static const char _lib_##libid##_description[] = description; \
        __attribute__((used, section("__DATA,__lib_meta"))) \
        static const char _lib_##libid##_author[] = author; \
        __attribute__((visibility("default"))) \
        const char* get_##libid##_library_name() { return name; } \
        __attribute__((visibility("default"))) \
        const char* get_##libid##_library_version() { return version; } \
        __attribute__((visibility("default"))) \
        const char* get_##libid##_library_description() { return description; } \
        __attribute__((visibility("default"))) \
        const char* get_##libid##_library_author() { return author; } \
    }

// Simpler version - just description, with Doxygen
#define LIBRARY_DOC_DOXYGEN(description) \
    /** \brief " description " */ \
    extern "C" { \
        __attribute__((visibility("default"), used)) \
        const char* get_library_doc() { return description; } \
    }

// Function info metadata with Doxygen
#define FUNCTION_DOC_DOXYGEN(func_name, doc) \
    /** \brief " doc " */ \
    extern "C" { \
        __attribute__((visibility("default"), used)) \
        const char* func_name##_doc() { return doc; } \
    }

#define LIBRARY_METADATA(libid, name, version, description, author) LIBRARY_METADATA_DOXYGEN(libid, name, version, description, author)
#define LIBRARY_DOC(description) LIBRARY_DOC_DOXYGEN(description)
#define FUNCTION_DOC(func_name, doc) FUNCTION_DOC_DOXYGEN(func_name, doc)

#endif // LIB_METADATA_H
