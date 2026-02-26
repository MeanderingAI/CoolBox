/**
 * @file metadata_management.cpp
 * @brief Implementation of the metadata management library.
 *
 * This translation unit registers the library's own metadata using the
 * LIBRARY_METADATA macro, ensuring that the metadata_management library
 * itself is discoverable by the service_manager.
 */

#include "lib_metadata.h"

LIBRARY_METADATA(metadata_management,
                 "metadata_management",
                 "1.0.0",
                 "Embeds documentation and metadata into shared libraries for service_manager discovery",
                 "CoolBox")
