#include "coolbox_jbridge.h"
#include "hidden_markov_model/hidden_markov_model.h"
#include <Eigen/Dense>

namespace {
Eigen::MatrixXd matrix_of(const double* p,size_t rows,size_t cols) { auto v=coolbox_jbridge::rows_of(p,rows,cols); Eigen::MatrixXd m(rows,cols); for(size_t r=0;r<rows;++r) for(size_t c=0;c<cols;++c) m(r,c)=v[r][c]; return m; }
std::vector<int> observations_of(const HMM& hmm,const int* p,size_t n) { if(!p||n==0) throw std::invalid_argument("observation sequence must not be empty"); const auto emissions=hmm.get_emission_matrix(); std::vector<int> values(p,p+n); for(int value:values) if(value<0||value>=emissions.cols()) throw std::out_of_range("observation index is out of range"); return values; }
double* matrix_out(const Eigen::MatrixXd& m,size_t* rows,size_t* cols) { std::vector<double> v; v.reserve(static_cast<size_t>(m.rows()*m.cols())); for(Eigen::Index r=0;r<m.rows();++r) for(Eigen::Index c=0;c<m.cols();++c) v.push_back(m(r,c)); if(rows)*rows=static_cast<size_t>(m.rows()); if(cols)*cols=static_cast<size_t>(m.cols()); return coolbox_jbridge::malloc_copy(v,nullptr); }
}
extern "C" {
void* coolbox_hmm_new(int states,int observations,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ if(states<=0||observations<=0) throw std::invalid_argument("state and observation counts must be positive"); return new HMM(states,observations); }); }
void coolbox_hmm_free(void* h) { delete static_cast<HMM*>(h); }
int coolbox_hmm_set_initial(void* h,const double* p,size_t n,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ auto& hmm=coolbox_jbridge::checked<HMM>(h,"HMM"); if(n!=hmm.get_initial_probabilities().size()) throw std::invalid_argument("initial probability count does not match state count"); hmm.set_initial_probabilities(coolbox_jbridge::vec(p,n)); }); }
int coolbox_hmm_set_transition(void* h,const double* p,size_t r,size_t col,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ auto& hmm=coolbox_jbridge::checked<HMM>(h,"HMM"); const size_t states=hmm.get_initial_probabilities().size(); if(r!=states||col!=states) throw std::invalid_argument("transition matrix must be square with one row per state"); hmm.set_transition_matrix(matrix_of(p,r,col)); }); }
int coolbox_hmm_set_emission(void* h,const double* p,size_t r,size_t col,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ auto& hmm=coolbox_jbridge::checked<HMM>(h,"HMM"); const size_t states=hmm.get_initial_probabilities().size(); if(r!=states||col==0) throw std::invalid_argument("emission matrix must have one row per state and at least one observation"); hmm.set_emission_matrix(matrix_of(p,r,col)); }); }
double* coolbox_hmm_get_initial(void* h,size_t* n,char* e,size_t c) { try { return coolbox_jbridge::malloc_copy(coolbox_jbridge::checked<HMM>(h,"HMM").get_initial_probabilities(),n); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
double* coolbox_hmm_get_transition(void* h,size_t* r,size_t* col,char* e,size_t c) { try { return matrix_out(coolbox_jbridge::checked<HMM>(h,"HMM").get_transition_matrix(),r,col); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
double* coolbox_hmm_get_emission(void* h,size_t* r,size_t* col,char* e,size_t c) { try { return matrix_out(coolbox_jbridge::checked<HMM>(h,"HMM").get_emission_matrix(),r,col); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
double coolbox_hmm_log_likelihood(void* h,const int* obs,size_t n,char* e,size_t c) { try { auto& hmm=coolbox_jbridge::checked<HMM>(h,"HMM"); return hmm.log_likelihood(observations_of(hmm,obs,n)); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return 0; } }
int* coolbox_hmm_viterbi(void* h,const int* obs,size_t n,size_t* count,char* e,size_t c) {
    try {
        auto& hmm=coolbox_jbridge::checked<HMM>(h,"HMM");
        if(!obs||n==0) throw std::invalid_argument("observation sequence must not be empty");
        const auto initial=hmm.get_initial_probabilities();
        const Eigen::MatrixXd transition=hmm.get_transition_matrix(), emission=hmm.get_emission_matrix();
        const size_t states=initial.size();
        std::vector<double> score(states*n);
        std::vector<int> predecessor(states*n);
        for(size_t s=0;s<states;++s) {
            if(obs[0]<0||obs[0]>=emission.cols()) throw std::out_of_range("observation index is out of range");
            score[s*n]=std::log(initial[s])+std::log(emission(s,obs[0]));
        }
        for(size_t t=1;t<n;++t) {
            if(obs[t]<0||obs[t]>=emission.cols()) throw std::out_of_range("observation index is out of range");
            for(size_t s=0;s<states;++s) {
                double best=-std::numeric_limits<double>::infinity(); int best_prev=0;
                for(size_t prev=0;prev<states;++prev) {
                    const double value=score[prev*n+t-1]+std::log(transition(prev,s));
                    if(value>best){best=value;best_prev=static_cast<int>(prev);}
                }
                score[s*n+t]=best+std::log(emission(s,obs[t])); predecessor[s*n+t]=best_prev;
            }
        }
        int best_state=0;
        for(size_t s=1;s<states;++s) if(score[s*n+n-1]>score[static_cast<size_t>(best_state)*n+n-1]) best_state=static_cast<int>(s);
        std::vector<int> path(n); path[n-1]=best_state;
        for(size_t t=n-1;t>0;--t) path[t-1]=predecessor[static_cast<size_t>(path[t])*n+t];
        return coolbox_jbridge::malloc_copy(path,count);
    } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; }
}
int coolbox_hmm_train(void* h,const int* obs,const size_t* offsets,size_t sequences,size_t total,int iterations,double tolerance,double smoothing,unsigned int seed,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ if(!offsets||(total&&!obs)) throw std::invalid_argument("sequence data and offsets are required"); std::vector<std::vector<int>> data; for(size_t i=0;i<sequences;++i){ if(offsets[i]>offsets[i+1]||offsets[i+1]>total) throw std::invalid_argument("invalid sequence offsets"); data.emplace_back(obs+offsets[i],obs+offsets[i+1]); } auto& hmm=coolbox_jbridge::checked<HMM>(h,"HMM"); for(const auto& sequence:data) if(!sequence.empty()) observations_of(hmm,sequence.data(),sequence.size()); hmm.train(data,iterations,tolerance,smoothing,seed); }); }
}

