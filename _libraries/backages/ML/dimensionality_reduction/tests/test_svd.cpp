#include "tyst_framework.hpp"

#include "svd.h"

using dimensionality_reduction::SVD;

TYST_TEST(SvdTest, ReconstructsRankOneMatricesAccurately) {
	Eigen::MatrixXd inputs(3, 2);
	inputs << 1.0, 2.0,
			  2.0, 4.0,
			  3.0, 6.0;

	SVD svd(false);
	svd.compute(inputs);

	const Eigen::MatrixXd reconstructed = svd.reconstruct();
	const Eigen::VectorXd singular_values = svd.get_singular_values();
	const Eigen::VectorXd explained_ratio = svd.explained_variance_ratio();

	TYST_EXPECT_LT((reconstructed - inputs).norm(), 1e-8);
	TYST_EXPECT_EQ(svd.rank(), 1);
	TYST_EXPECT_EQ(singular_values.size(), 2);
	TYST_EXPECT_GT(singular_values(0), singular_values(1));
	TYST_EXPECT_NEAR(explained_ratio(0), 1.0, 1e-12);
}
