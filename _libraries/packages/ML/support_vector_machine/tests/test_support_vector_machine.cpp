#include "tyst_framework.hpp"

#include <cmath>

#include "linear_kernel.h"
#include "polynomial_kernel.h"
#include "rbf_kernel.h"
#include "sigmoid_kernel.h"
#include "support_vector_machine.h"

namespace {

Eigen::VectorXd make_vector(double first, double second) {
	Eigen::VectorXd vector(2);
	vector(0) = first;
	vector(1) = second;
	return vector;
}

Eigen::MatrixXd make_separable_training_inputs() {
	Eigen::MatrixXd inputs(4, 2);
	inputs.row(0) = make_vector(2.0, 2.0);
	inputs.row(1) = make_vector(2.0, 0.0);
	inputs.row(2) = make_vector(-2.0, -1.0);
	inputs.row(3) = make_vector(-1.0, -2.0);
	return inputs;
}

Eigen::VectorXd make_separable_training_labels() {
	Eigen::VectorXd labels(4);
	labels(0) = 1.0;
	labels(1) = 1.0;
	labels(2) = -1.0;
	labels(3) = -1.0;
	return labels;
}

} // namespace

TYST_TEST(SupportVectorMachineKernelTest, LinearKernelComputesDotProduct) {
	LinearKernel kernel;

	const Eigen::VectorXd left = make_vector(1.0, 3.0);
	const Eigen::VectorXd right = make_vector(2.0, 4.0);

	TYST_EXPECT_NEAR(kernel.calculate(left, right), 14.0, 1e-12);
}

TYST_TEST(SupportVectorMachineKernelTest, PolynomialKernelAppliesGammaOffsetAndDegree) {
	PolynomialKernel kernel(0.5, 1.0, 3);

	const Eigen::VectorXd left = make_vector(2.0, 0.0);
	const Eigen::VectorXd right = make_vector(1.0, 1.0);

	TYST_EXPECT_NEAR(kernel.calculate(left, right), 8.0, 1e-12);
}

TYST_TEST(SupportVectorMachineKernelTest, RbfKernelReturnsOneForIdenticalVectors) {
	RBFKernel kernel(0.75);
	const Eigen::VectorXd sample = make_vector(1.5, -0.5);

	TYST_EXPECT_NEAR(kernel.calculate(sample, sample), 1.0, 1e-12);
}

TYST_TEST(SupportVectorMachineKernelTest, SigmoidKernelMatchesHyperbolicTangentForm) {
	SigmoidKernel kernel(0.25, -0.5);

	const Eigen::VectorXd left = make_vector(2.0, 1.0);
	const Eigen::VectorXd right = make_vector(4.0, -2.0);

	TYST_EXPECT_NEAR(kernel.calculate(left, right), std::tanh(0.25 * 6.0 - 0.5), 1e-12);
}

TYST_TEST(SupportVectorMachineModelTest, LinearKernelSeparatesTrainingSamples) {
	LinearKernel kernel;
	SVM svm(kernel);

	const Eigen::MatrixXd inputs = make_separable_training_inputs();
	const Eigen::VectorXd labels = make_separable_training_labels();
	svm.fit(inputs, labels);

	for (Eigen::Index index = 0; index < inputs.rows(); ++index) {
		TYST_EXPECT_EQ(svm.predict(matrix::DenseMatrix(inputs.row(index).begin(), 1, inputs.cols())), labels(index));
	}
}

TYST_TEST(SupportVectorMachineModelTest, LinearKernelClassifiesHeldOutPointsNearEachCluster) {
	LinearKernel kernel;
	SVM svm(kernel);

	svm.fit(make_separable_training_inputs(), make_separable_training_labels());

	TYST_EXPECT_EQ(svm.predict(matrix::DenseMatrix({3.0, 1.0}, 1, 2)), 1.0);
	TYST_EXPECT_EQ(svm.predict(matrix::DenseMatrix({-2.5, -1.5}, 1, 2)), -1.0);
}

TYST_TEST(SupportVectorMachineModelTest, RbfKernelSeparatesSameSimpleDataset) {
	RBFKernel kernel(0.5);
	SVM svm(kernel);

	const Eigen::MatrixXd inputs = make_separable_training_inputs();
	const Eigen::VectorXd labels = make_separable_training_labels();
	svm.fit(inputs, labels);

	TYST_EXPECT_EQ(svm.predict(matrix::DenseMatrix({2.5, 1.5}, 1, 2)), 1.0);
	TYST_EXPECT_EQ(svm.predict(matrix::DenseMatrix({-1.5, -2.5}, 1, 2)), -1.0);
}
