#include "tyst_framework.hpp"

#include "latent_sentiment_analysis.h"

TYST_TEST(LatentSentimentAnalysisTest, LearnsStrongerScoresForObservedTerms) {
	Eigen::MatrixXd document_term_matrix(3, 3);
	document_term_matrix << 5.0, 4.0, 0.0,
							0.0, 1.0, 4.0,
							4.0, 0.0, 0.0;

	LatentSentimentAnalysis model(2, 0.02, 0.01, 300);
	model.train(document_term_matrix);

	const auto& document_features = model.get_document_features();
	const auto& term_features = model.get_term_features();
	const double seen_score = model.predict_score(0, 0);
	const double unseen_score = model.predict_score(0, 2);

	TYST_EXPECT_EQ(document_features.rows(), 3);
	TYST_EXPECT_EQ(document_features.cols(), 2);
	TYST_EXPECT_EQ(term_features.rows(), 3);
	TYST_EXPECT_EQ(term_features.cols(), 2);
	TYST_EXPECT_GT(seen_score, unseen_score);
	TYST_EXPECT_GT(seen_score, 0.0);
	TYST_EXPECT_EQ(model.predict_score(9, 9), 0.0);
}
