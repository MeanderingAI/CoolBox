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

TYST_TEST(LatentSentimentAnalysisTest, ProducesDeterministicResultsForSameInput) {
	Eigen::MatrixXd document_term_matrix(2, 3);
	document_term_matrix << 3.0, 0.0, 1.0,
							0.0, 4.0, 2.0;

	LatentSentimentAnalysis first_model(2, 0.02, 0.01, 250);
	LatentSentimentAnalysis second_model(2, 0.02, 0.01, 250);

	first_model.train(document_term_matrix);
	second_model.train(document_term_matrix);

	TYST_EXPECT_NEAR(
		(first_model.get_document_features() - second_model.get_document_features()).norm(),
		0.0,
		1e-12
	);
	TYST_EXPECT_NEAR(
		(first_model.get_term_features() - second_model.get_term_features()).norm(),
		0.0,
		1e-12
	);
	TYST_EXPECT_NEAR(first_model.predict_score(0, 0), second_model.predict_score(0, 0), 1e-12);
	TYST_EXPECT_NEAR(first_model.predict_score(1, 2), second_model.predict_score(1, 2), 1e-12);
}

TYST_TEST(LatentSentimentAnalysisTest, KeepsZeroMatrixPredictionsFiniteAndNearZero) {
	Eigen::MatrixXd document_term_matrix = Eigen::MatrixXd::Zero(2, 3);

	LatentSentimentAnalysis model(2, 0.02, 0.05, 250);
	model.train(document_term_matrix);

	const auto& document_features = model.get_document_features();
	const auto& term_features = model.get_term_features();

	TYST_EXPECT_EQ(document_features.rows(), 2);
	TYST_EXPECT_EQ(document_features.cols(), 2);
	TYST_EXPECT_EQ(term_features.rows(), 3);
	TYST_EXPECT_EQ(term_features.cols(), 2);
	TYST_EXPECT_TRUE(document_features.array().isFinite().all());
	TYST_EXPECT_TRUE(term_features.array().isFinite().all());

	for (Eigen::Index doc_index = 0; doc_index < document_term_matrix.rows(); ++doc_index) {
		for (Eigen::Index term_index = 0; term_index < document_term_matrix.cols(); ++term_index) {
			TYST_EXPECT_LT(std::abs(model.predict_score(doc_index, term_index)), 0.01);
		}
	}

	TYST_EXPECT_EQ(model.predict_score(-1, 0), 0.0);
	TYST_EXPECT_EQ(model.predict_score(0, -1), 0.0);
}

TYST_TEST(LatentSentimentAnalysisTest, LearnsDocumentSpecificTermPreferencesOnDiagonalMatrix) {
	Eigen::MatrixXd document_term_matrix = Eigen::MatrixXd::Zero(3, 3);
	document_term_matrix(0, 0) = 5.0;
	document_term_matrix(1, 1) = 5.0;
	document_term_matrix(2, 2) = 5.0;

	LatentSentimentAnalysis model(3, 0.02, 0.01, 400);
	model.train(document_term_matrix);

	for (Eigen::Index index = 0; index < 3; ++index) {
		const double aligned_score = model.predict_score(index, index);
		TYST_EXPECT_GT(aligned_score, 0.0);
		for (Eigen::Index other_index = 0; other_index < 3; ++other_index) {
			if (other_index == index) {
				continue;
			}
			TYST_EXPECT_GT(aligned_score, model.predict_score(index, other_index));
		}
	}
}
