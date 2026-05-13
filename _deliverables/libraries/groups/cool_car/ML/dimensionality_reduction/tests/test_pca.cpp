#include "tyst_framework.hpp"

#include "pca.h"

using dimensionality_reduction::PCA;

TYST_TEST(PcaTest, ProjectsCollinearDataIntoOneComponent) {
	Eigen::MatrixXd inputs(4, 2);
	inputs << 1.0, 2.0,
			  2.0, 4.0,
			  3.0, 6.0,
			  4.0, 8.0;

	PCA pca(1, true, false);
	const Eigen::MatrixXd transformed = pca.fit_transform(inputs);
	const Eigen::MatrixXd reconstructed = pca.inverse_transform(transformed);
	const auto ratio_vec = pca.get_explained_variance_ratio();
	const Eigen::VectorXd explained_ratio = Eigen::Map<const Eigen::VectorXd>(
		ratio_vec.data(), static_cast<Eigen::Index>(ratio_vec.size()));

	TYST_EXPECT_EQ(transformed.rows(), inputs.rows());
	TYST_EXPECT_EQ(transformed.cols(), 1);
	TYST_EXPECT_LT((reconstructed - inputs).norm(), 1e-8);
	TYST_EXPECT_EQ(explained_ratio.size(), 1);
	TYST_EXPECT_NEAR(explained_ratio(0), 1.0, 1e-12);
}
