#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../../go_bindings/matrix_bridge.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace py = pybind11;
using coolbox::matrix::CpuOptimization;
using coolbox::matrix::DenseMatrix;
using coolbox::matrix::DenseVector;
using coolbox::matrix::MatrixBackend;

void bind_matrix(py::module_& parent) {
    py::module_ matrix = parent.def_submodule("matrix", "Dense matrix and vector operations");

    py::enum_<CpuOptimization>(matrix, "CpuOptimization")
        .value("AUTO", CpuOptimization::Auto)
        .value("SCALAR", CpuOptimization::Scalar)
        .value("AVX2_FMA", CpuOptimization::Avx2Fma)
        .value("NEON", CpuOptimization::Neon);

    py::enum_<MatrixBackend>(matrix, "MatrixBackend")
        .value("CPU", MatrixBackend::Cpu)
        .value("EIGEN", MatrixBackend::Eigen)
        .value("AUTO", MatrixBackend::Auto)
        .value("OPENCL", MatrixBackend::OpenCl)
        .value("CUDA", MatrixBackend::Cuda)
        .value("METAL", MatrixBackend::Metal);

    matrix.def("cpu_optimization_name", &coolbox::matrix::cpu_optimization_name);
    matrix.def("cpu_optimization_available", &coolbox::matrix::cpu_optimization_available);
    matrix.def("set_backend", &coolbox::matrix::set_matrix_backend, py::arg("backend"));
    matrix.def("get_backend", &coolbox::matrix::matrix_backend);
    matrix.def("get_active_backend", &coolbox::matrix::active_matrix_backend);
    matrix.def("backend_name", &coolbox::matrix::matrix_backend_name, py::arg("backend"));
    matrix.def("backend_available", &coolbox::matrix::matrix_backend_available,
               py::arg("backend"));

    py::class_<DenseVector>(matrix, "DenseVector")
        .def(py::init([](size_t size) { return DenseVector(size); }), py::arg("size"))
        .def(py::init([](const std::vector<double>& values) {
            return DenseVector(values.data(), values.size());
        }), py::arg("values"))
        .def_property_readonly("size", &DenseVector::size)
        .def_property_readonly("data", [](const DenseVector& vector) {
            std::vector<double> values;
            if (vector.size() != 0) {
                values.assign(vector.data(), vector.data() + vector.size());
            }
            return values;
        })
        .def("at", &DenseVector::at, py::arg("index"))
        .def("set", &DenseVector::set, py::arg("index"), py::arg("value"))
        .def("norm", &DenseVector::norm)
        .def("dot", &DenseVector::dot, py::arg("other"),
             py::arg("optimization") = CpuOptimization::Auto)
        .def("add", &DenseVector::add, py::arg("other"),
             py::arg("optimization") = CpuOptimization::Auto)
        .def("transpose", [](const DenseVector& vector) {
            DenseMatrix result(1, vector.size());
            for (size_t index = 0; index < vector.size(); ++index) {
                result.set(0, index, vector.at(index));
            }
            return result;
        })
        .def("__len__", &DenseVector::size);

    py::class_<DenseMatrix>(matrix, "DenseMatrix")
        .def(py::init<size_t, size_t>(), py::arg("rows"), py::arg("columns"))
        .def(py::init([](size_t rows, size_t columns, const std::vector<double>& values) {
            return DenseMatrix(rows, columns, values.data(), values.size());
        }), py::arg("rows"), py::arg("columns"), py::arg("values"))
        .def_static("from_rows", [](const std::vector<std::vector<double>>& rows) {
            if (rows.empty() || rows.front().empty()) {
                throw std::invalid_argument("matrix rows must be non-empty");
            }
            const size_t columns = rows.front().size();
            std::vector<double> flat;
            flat.reserve(rows.size() * columns);
            for (const auto& row : rows) {
                if (row.size() != columns) {
                    throw std::invalid_argument("matrix rows must have equal lengths");
                }
                flat.insert(flat.end(), row.begin(), row.end());
            }
            return DenseMatrix(rows.size(), columns, flat.data(), flat.size());
        }, py::arg("rows"))
        .def_static("identity", &DenseMatrix::identity, py::arg("size"))
        .def_static("zeros", [](size_t rows, size_t columns) {
            return DenseMatrix(rows, columns);
        }, py::arg("rows"), py::arg("columns"))
        .def_static("ones", [](size_t rows, size_t columns) {
            DenseMatrix result(rows, columns);
            for (size_t row = 0; row < rows; ++row) {
                for (size_t column = 0; column < columns; ++column) {
                    result.set(row, column, 1.0);
                }
            }
            return result;
        }, py::arg("rows"), py::arg("columns"))
        .def_property_readonly("rows", &DenseMatrix::rows)
        .def_property_readonly("columns", &DenseMatrix::columns)
        .def_property_readonly("shape", [](const DenseMatrix& value) {
            return py::make_tuple(value.rows(), value.columns());
        })
        .def_property_readonly("data", [](const DenseMatrix& value) {
            const size_t count = value.rows() * value.columns();
            return std::vector<double>(value.data(), value.data() + count);
        })
        .def("at", &DenseMatrix::at, py::arg("row"), py::arg("column"))
        .def("set", &DenseMatrix::set, py::arg("row"), py::arg("column"), py::arg("value"))
        .def("add", &DenseMatrix::add, py::arg("other"),
             py::arg("optimization") = CpuOptimization::Auto)
        .def("subtract", &DenseMatrix::subtract, py::arg("other"))
        .def("multiply", &DenseMatrix::multiply, py::arg("other"),
             py::arg("optimization") = CpuOptimization::Auto)
        .def("transpose", &DenseMatrix::transpose)
        .def("rowwise_add", &DenseMatrix::rowwise_add,
             py::arg("vector"), py::arg("optimization") = CpuOptimization::Auto)
        .def("norm", &DenseMatrix::norm)
        .def("__add__", [](const DenseMatrix& lhs, const DenseMatrix& rhs) {
            return lhs.add(rhs, CpuOptimization::Auto);
        }, py::is_operator())
        .def("__sub__", &DenseMatrix::subtract, py::is_operator())
        .def("__matmul__", [](const DenseMatrix& lhs, const DenseMatrix& rhs) {
            return lhs.multiply(rhs, CpuOptimization::Auto);
        }, py::is_operator())
        .def("__repr__", [](const DenseMatrix& value) {
            return "DenseMatrix(shape=(" + std::to_string(value.rows()) + ", " +
                std::to_string(value.columns()) + "))";
        });
}
