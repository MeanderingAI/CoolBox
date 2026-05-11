// LSA bindings — wraps Eigen types for JS.
#include <emscripten/bind.h>
#include "latent_sentiment_analysis.h"

using namespace emscripten;

class LSAWrapper {
    LatentSentimentAnalysis lsa_;
public:
    LSAWrapper(int num_features) : lsa_(num_features) {}

    void train(const std::vector<std::vector<double>>& dtm_vec) {
        if (dtm_vec.empty()) return;
        Eigen::MatrixXd dtm(dtm_vec.size(), dtm_vec[0].size());
        for (size_t i = 0; i < dtm_vec.size(); ++i)
            for (size_t j = 0; j < dtm_vec[i].size(); ++j)
                dtm(i, j) = dtm_vec[i][j];
        lsa_.train(dtm);
    }

    double predict_score(int doc_index, int term_index) {
        return lsa_.predict_score(doc_index, term_index);
    }
};

EMSCRIPTEN_BINDINGS(latent_sentiment_analysis_module) {
    register_vector<double>("VectorDouble_LSA");
    register_vector<std::vector<double>>("VectorVectorDouble_LSA");

    class_<LSAWrapper>("LatentSentimentAnalysis")
        .constructor<int>()
        .function("train", &LSAWrapper::train)
        .function("predict_score", &LSAWrapper::predict_score)
    ;
}
