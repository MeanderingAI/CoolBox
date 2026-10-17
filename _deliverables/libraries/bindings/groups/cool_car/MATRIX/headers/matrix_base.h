#pragma once
#include <memory>
namespace mytrix {

class MatrixBase {
public:
	virtual ~MatrixBase() = default;
	virtual int rows() const = 0;
	virtual int cols() const = 0;
	// Add more pure virtuals as needed
};

class VectorBase {
public:
	virtual ~VectorBase() = default;
	virtual int size() const = 0;
	// Add more pure virtuals as needed
};

using MatrixBasePtr = std::shared_ptr<MatrixBase>;
using VectorBasePtr = std::shared_ptr<VectorBase>;

} // namespace mytrix
