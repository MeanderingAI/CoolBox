// Shared helpers for the Java-only C bridges in native/src/*_jbridge.cc.
//
// Conventions (identical to the Go bridges so Java can treat both uniformly):
//   * Every exported function is declared inside extern "C" and named coolbox_<module>_<what>.
//     The definition must start at column 0 so CMake can generate the Windows .def file.
//   * Fallible functions return int (1 = success, 0 = failure) or a handle (nullptr = failure)
//     and take trailing (char* error, size_t error_capacity) parameters.
//   * Handles are opaque void*; each type has a coolbox_<module>_<type>_free(void*).
//   * Strings returned to the caller are malloc'ed and must be released with coolbox_java_free.
//   * Variable-length numeric results use a two-call protocol (query the size, then copy into a
//     caller-provided buffer) or a malloc'ed array released with coolbox_java_free.
#ifndef COOLBOX_JBRIDGE_H
#define COOLBOX_JBRIDGE_H

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace coolbox_jbridge {

inline void write_error(char* buffer, size_t capacity, const char* message) {
    if (buffer == nullptr || capacity == 0) {
        return;
    }
    const size_t count = std::min(capacity - 1, std::strlen(message));
    std::memcpy(buffer, message, count);
    buffer[count] = '\0';
}

template <typename Function>
int invoke(char* error, size_t error_capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        write_error(error, error_capacity, exception.what());
    } catch (...) {
        write_error(error, error_capacity, "unknown native exception");
    }
    return 0;
}

// Runs function() and returns its pointer result, or nullptr with the error filled in.
template <typename Function>
void* invoke_handle(char* error, size_t error_capacity, Function&& function) {
    try {
        return static_cast<void*>(function());
    } catch (const std::exception& exception) {
        write_error(error, error_capacity, exception.what());
    } catch (...) {
        write_error(error, error_capacity, "unknown native exception");
    }
    return nullptr;
}

template <typename T>
T& checked(void* handle, const char* what = "native object") {
    if (handle == nullptr) {
        throw std::invalid_argument(std::string(what) + " is closed");
    }
    return *static_cast<T*>(handle);
}

inline char* dup_string(const std::string& value) {
    char* out = static_cast<char*>(std::malloc(value.size() + 1));
    if (out == nullptr) {
        throw std::bad_alloc();
    }
    std::memcpy(out, value.data(), value.size());
    out[value.size()] = '\0';
    return out;
}

inline std::string str(const char* value) {
    return value == nullptr ? std::string() : std::string(value);
}

inline std::vector<double> vec(const double* values, size_t count) {
    if (count == 0) {
        return {};
    }
    if (values == nullptr) {
        throw std::invalid_argument("input array must not be null");
    }
    return std::vector<double>(values, values + count);
}

// Row-major (rows x cols) buffer -> vector of rows.
inline std::vector<std::vector<double>> rows_of(const double* values, size_t rows, size_t cols) {
    if (rows != 0 && cols != 0 && values == nullptr) {
        throw std::invalid_argument("input matrix must not be null");
    }
    std::vector<std::vector<double>> out(rows, std::vector<double>(cols));
    for (size_t r = 0; r < rows; ++r) {
        std::copy_n(values + r * cols, cols, out[r].begin());
    }
    return out;
}

// Copies values into a caller buffer (when large enough) and always reports the required size.
template <typename T>
size_t copy_out(const std::vector<T>& values, T* out, size_t capacity) {
    if (out != nullptr && capacity >= values.size()) {
        std::copy(values.begin(), values.end(), out);
    }
    return values.size();
}

// Returns a malloc'ed copy of values (release with coolbox_java_free); *count receives the length.
template <typename T>
T* malloc_copy(const std::vector<T>& values, size_t* count) {
    if (count != nullptr) {
        *count = values.size();
    }
    T* out = static_cast<T*>(std::malloc(std::max<size_t>(1, values.size()) * sizeof(T)));
    if (out == nullptr) {
        throw std::bad_alloc();
    }
    std::copy(values.begin(), values.end(), out);
    return out;
}

// Flattens a vector of equally sized rows into a malloc'ed row-major buffer.
inline double* malloc_matrix(const std::vector<std::vector<double>>& rows, size_t* row_count, size_t* col_count) {
    const size_t r = rows.size();
    const size_t c = r == 0 ? 0 : rows.front().size();
    if (row_count != nullptr) {
        *row_count = r;
    }
    if (col_count != nullptr) {
        *col_count = c;
    }
    double* out = static_cast<double*>(std::malloc(std::max<size_t>(1, r * c) * sizeof(double)));
    if (out == nullptr) {
        throw std::bad_alloc();
    }
    for (size_t i = 0; i < r; ++i) {
        if (rows[i].size() != c) {
            std::free(out);
            throw std::runtime_error("ragged matrix cannot be flattened");
        }
        std::copy(rows[i].begin(), rows[i].end(), out + i * c);
    }
    return out;
}

}  // namespace coolbox_jbridge

#endif  // COOLBOX_JBRIDGE_H
