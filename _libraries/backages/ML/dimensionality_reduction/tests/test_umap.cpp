#include "tyst_framework.hpp"

#include "umap.h"

#include <cmath>

using dimensionality_reduction::UMAP;

TYST_TEST(UmapTest, ProducesFiniteEmbeddingsAndTransforms) {
	Eigen::MatrixXd inputs(5, 3);
	inputs << 0.0, 0.0, 0.0,
			  0.0, 1.0, 0.0,
			  1.0, 0.0, 0.0,
			  1.0, 1.0, 0.0,
			  0.5, 0.5, 1.0;

	UMAP umap(2, 2, 0.1, "euclidean", 0.5, 10, 42);
	const Eigen::MatrixXd embedding = umap.fit_transform(inputs);
	const Eigen::MatrixXd transformed = umap.transform(inputs.topRows(2));

	TYST_EXPECT_EQ(embedding.rows(), inputs.rows());
	TYST_EXPECT_EQ(embedding.cols(), 2);
	TYST_EXPECT_TRUE(embedding.array().isFinite().all());
	TYST_EXPECT_EQ(transformed.rows(), 2);
	TYST_EXPECT_EQ(transformed.cols(), 2);
	TYST_EXPECT_TRUE(transformed.array().isFinite().all());
}
