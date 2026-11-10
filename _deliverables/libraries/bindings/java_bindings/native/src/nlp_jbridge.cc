#include "coolbox_jbridge.h"
#include "nlp/text_processor.h"
#include "nlp/embeddings.h"
#include <memory>
#include <set>

using namespace coolbox_jbridge;
namespace n=ml::nlp;
namespace {
struct Strings { std::vector<std::string> values; };
struct Vocab { std::shared_ptr<n::Vocabulary> value; };
struct Bag { n::BagOfWords value; explicit Bag(std::shared_ptr<n::Vocabulary> v):value(std::move(v)){} };
struct Tf { n::TFIDF value; explicit Tf(std::shared_ptr<n::Vocabulary> v):value(std::move(v)){} };
std::vector<std::string> strings(const char* const* p,size_t n){std::vector<std::string> v; if(n&&!p)throw std::invalid_argument("string array is null");for(size_t i=0;i<n;++i)v.emplace_back(str(p[i]));return v;}
std::vector<std::vector<std::string>> docs(const char* const* p,const size_t* offsets,size_t count){if(count&&!offsets)throw std::invalid_argument("document offsets are null");size_t total=count?offsets[count]:0;if(total&&!p)throw std::invalid_argument("tokens are null");std::vector<std::vector<std::string>> out(count);size_t start=0;for(size_t d=0;d<count;++d){if(offsets[d]<start||offsets[d+1]<offsets[d]||offsets[d+1]>total)throw std::invalid_argument("invalid document offsets");for(size_t i=offsets[d];i<offsets[d+1];++i)out[d].emplace_back(str(p[i]));start=offsets[d+1];}return out;}
std::shared_ptr<n::Vocabulary> vocab(void* h){if(!h) return nullptr; return static_cast<Vocab*>(h)->value;}
template<class T> double* matrix_out(const std::vector<std::vector<T>>& m,size_t* rows,size_t* cols){return malloc_matrix(m,rows,cols);}
}
extern "C" {
void* coolbox_nlp_string_list_new(const std::vector<std::string>* values){return new Strings{*values};}
void coolbox_nlp_string_list_free(void* h){delete static_cast<Strings*>(h);}
size_t coolbox_nlp_string_list_size(const void* h){return checked<const Strings>(const_cast<void*>(h),"string list").values.size();}
char* coolbox_nlp_string_list_at(const void* h,size_t i,char* e,size_t n){try{return dup_string(checked<const Strings>(const_cast<void*>(h),"string list").values.at(i));}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
void* coolbox_nlp_strip(const char* text){try{return new Strings{{n::TextProcessor::strip(str(text))}};}catch(...){return nullptr;}}
void* coolbox_nlp_tokenize_full(const char* text,const char* delimiter,int mode,char* e,size_t cap){return invoke_handle(e,cap,[&]()->void*{std::vector<std::string> out;std::string s=str(text);switch(mode){case 0:out=n::TextProcessor::tokenize(s,str(delimiter));break;case 1:out=n::TextProcessor::word_tokenize(s);break;case 2:out=n::TextProcessor::sentence_tokenize(s);break;default:throw std::invalid_argument("unknown tokenization mode");}return new Strings{std::move(out)};});}
void* coolbox_nlp_ngrams(const char* const* tokens,size_t count,size_t ngram,char* e,size_t cap){return invoke_handle(e,cap,[&]()->void*{return new Strings{n::TextProcessor::generate_ngrams(strings(tokens,count),ngram)};});}
char* coolbox_nlp_stem(const char* word,char* e,size_t n){try{return dup_string(n::TextProcessor::stem(str(word)));}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
void* coolbox_nlp_stem_tokens(const char* const* tokens,size_t count,char* e,size_t cap){return invoke_handle(e,cap,[&]()->void*{return new Strings{n::TextProcessor::stem_tokens(strings(tokens,count))};});}
void* coolbox_nlp_default_stop_words(){try{auto words=n::TextProcessor::default_stop_words();return new Strings{std::vector<std::string>(words.begin(),words.end())};}catch(...){return nullptr;}}
void* coolbox_nlp_remove_stop_words(const char* const* tokens,size_t count,const char* const* stop,size_t stop_count,char* e,size_t cap){return invoke_handle(e,cap,[&]()->void*{auto s=strings(stop,stop_count);return new Strings{n::TextProcessor::remove_stop_words(strings(tokens,count),std::set<std::string>(s.begin(),s.end()))};});}
void* coolbox_nlp_process(const char* text,int lower,int punct,int nums,int stops,int stemming,char* e,size_t cap){return invoke_handle(e,cap,[&]()->void*{n::TextProcessor p;return new Strings{p.process(str(text),lower!=0,punct!=0,nums!=0,stops!=0,stemming!=0)};});}
void* coolbox_nlp_vocabulary_new(size_t min_freq,size_t max_size){try{return new Vocab{std::make_shared<n::Vocabulary>(min_freq,max_size)};}catch(...){return nullptr;}}
void coolbox_nlp_vocabulary_free(void* h){delete static_cast<Vocab*>(h);}
int coolbox_nlp_vocabulary_build(void* h,const char* const* tokens,const size_t* offsets,size_t count,char* e,size_t cap){return invoke(e,cap,[&]{static_cast<Vocab*>(h)->value->build(docs(tokens,offsets,count));});}
int coolbox_nlp_vocabulary_build_texts(void* h,const char* const* texts,size_t count,char* e,size_t cap){return invoke(e,cap,[&]{static_cast<Vocab*>(h)->value->build_from_texts(strings(texts,count));});}
int coolbox_nlp_vocabulary_add_special(void* h,const char* token,char* e,size_t cap){return invoke(e,cap,[&]{static_cast<Vocab*>(h)->value->add_special_token(str(token));});}
size_t coolbox_nlp_vocabulary_to_index(const void* h,const char* token){return checked<const Vocab>(const_cast<void*>(h),"vocabulary").value->token_to_index(str(token));}
char* coolbox_nlp_vocabulary_to_token(const void* h,size_t index,char* e,size_t cap){try{return dup_string(checked<const Vocab>(const_cast<void*>(h),"vocabulary").value->index_to_token(index));}catch(const std::exception& x){write_error(e,cap,x.what());return nullptr;}}
int coolbox_nlp_vocabulary_contains(const void* h,const char* token){return checked<const Vocab>(const_cast<void*>(h),"vocabulary").value->contains(str(token));}
size_t coolbox_nlp_vocabulary_size(const void* h){return checked<const Vocab>(const_cast<void*>(h),"vocabulary").value->size();}
size_t coolbox_nlp_vocabulary_frequency(const void* h,const char* token){return checked<const Vocab>(const_cast<void*>(h),"vocabulary").value->frequency(str(token));}
long long* coolbox_nlp_vocabulary_encode(const void* h,const char* const* tokens,size_t count,size_t* out_count,char* e,size_t cap){try{auto v=checked<const Vocab>(const_cast<void*>(h),"vocabulary").value->encode(strings(tokens,count));std::vector<long long> r(v.begin(),v.end());return malloc_copy(r,out_count);}catch(const std::exception& x){write_error(e,cap,x.what());return nullptr;}}
void* coolbox_nlp_vocabulary_decode(const void* h,const long long* indices,size_t count,char* e,size_t cap){return invoke_handle(e,cap,[&]()->void*{std::vector<size_t> v;if(count&&!indices)throw std::invalid_argument("indices are null");for(size_t i=0;i<count;++i)v.push_back(static_cast<size_t>(indices[i]));return new Strings{checked<const Vocab>(const_cast<void*>(h),"vocabulary").value->decode(v)};});}
void* coolbox_nlp_vocabulary_copy(const void* h){try{return new Vocab{checked<const Vocab>(const_cast<void*>(h),"vocabulary").value};}catch(...){return nullptr;}}
void* coolbox_nlp_bag_new(void* v){try{return new Bag(v?vocab(v):nullptr);}catch(...){return nullptr;}}
void coolbox_nlp_bag_free(void* h){delete static_cast<Bag*>(h);}
int coolbox_nlp_bag_fit(void* h,const char* const* t,const size_t* o,size_t c,char* e,size_t n){return invoke(e,n,[&]{static_cast<Bag*>(h)->value.fit(docs(t,o,c));});}
double* coolbox_nlp_bag_transform(const void* h,const char* const* t,size_t c,size_t* count,char* e,size_t n){try{return malloc_copy(static_cast<const Bag*>(h)->value.transform(strings(t,c)),count);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
double* coolbox_nlp_bag_batch(const void* h,const char* const* t,const size_t* o,size_t c,size_t* rows,size_t* cols,char* e,size_t n){try{return matrix_out(static_cast<const Bag*>(h)->value.transform_batch(docs(t,o,c)),rows,cols);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
void* coolbox_nlp_bag_vocabulary(const void* h){try{return new Vocab{static_cast<const Bag*>(h)->value.vocabulary()};}catch(...){return nullptr;}}
void* coolbox_nlp_tfidf_new(void* v){try{return new Tf(v?vocab(v):nullptr);}catch(...){return nullptr;}}
void coolbox_nlp_tfidf_free(void* h){delete static_cast<Tf*>(h);}
int coolbox_nlp_tfidf_fit(void* h,const char* const* t,const size_t* o,size_t c,char* e,size_t n){return invoke(e,n,[&]{static_cast<Tf*>(h)->value.fit(docs(t,o,c));});}
double* coolbox_nlp_tfidf_transform(const void* h,const char* const* t,size_t c,size_t* count,char* e,size_t n){try{return malloc_copy(static_cast<const Tf*>(h)->value.transform(strings(t,c)),count);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
double* coolbox_nlp_tfidf_batch(const void* h,const char* const* t,const size_t* o,size_t c,size_t* rows,size_t* cols,char* e,size_t n){try{return matrix_out(static_cast<const Tf*>(h)->value.transform_batch(docs(t,o,c)),rows,cols);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
void* coolbox_nlp_tfidf_vocabulary(const void* h){try{return new Vocab{static_cast<const Tf*>(h)->value.vocabulary()};}catch(...){return nullptr;}}
void* coolbox_nlp_sequence_new(void* v,size_t maxlen,int pad,int trunc){try{if(!v) return nullptr; return new n::SequenceEncoder(vocab(v),maxlen,pad!=0,trunc!=0);}catch(...){return nullptr;}}
void coolbox_nlp_sequence_free(void* h){delete static_cast<n::SequenceEncoder*>(h);}
long long* coolbox_nlp_sequence_encode(const void* h,const char* const* t,size_t c,size_t* count,char* e,size_t n){try{auto v=static_cast<const n::SequenceEncoder*>(h)->encode(strings(t,c));std::vector<long long> r(v.begin(),v.end());return malloc_copy(r,count);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
long long* coolbox_nlp_sequence_batch(const void* h,const char* const* t,const size_t* o,size_t c,size_t* rows,size_t* cols,char* e,size_t n){try{auto d=docs(t,o,c);auto m=static_cast<const n::SequenceEncoder*>(h)->encode_batch(d);*rows=m.size();*cols=m.empty()?0:m.front().size();std::vector<long long> r;for(auto& row:m)for(auto x:row)r.push_back(x);return malloc_copy(r,nullptr);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
void* coolbox_nlp_sequence_decode(const void* h,const long long* a,size_t c,int skip,char* e,size_t n){return invoke_handle(e,n,[&]()->void*{std::vector<size_t> v;if(c&&!a)throw std::invalid_argument("indices are null");for(size_t i=0;i<c;++i)v.push_back(static_cast<size_t>(a[i]));return new Strings{static_cast<const n::SequenceEncoder*>(h)->decode(v,skip!=0)};});}
void* coolbox_nlp_character_new(){try{return new n::CharacterEncoder();}catch(...){return nullptr;}}
void coolbox_nlp_character_free(void* h){delete static_cast<n::CharacterEncoder*>(h);}
int coolbox_nlp_character_fit(void* h,const char* const* t,size_t c,char* e,size_t n){return invoke(e,n,[&]{static_cast<n::CharacterEncoder*>(h)->fit(strings(t,c));});}
long long* coolbox_nlp_character_encode(const void* h,const char* t,size_t* c,char* e,size_t n){try{auto v=static_cast<const n::CharacterEncoder*>(h)->encode(str(t));std::vector<long long> r(v.begin(),v.end());return malloc_copy(r,c);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
char* coolbox_nlp_character_decode(const void* h,const long long* a,size_t c,char* e,size_t n){try{std::vector<size_t> v;for(size_t i=0;i<c;++i)v.push_back(a[i]);return dup_string(static_cast<const n::CharacterEncoder*>(h)->decode(v));}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
size_t coolbox_nlp_character_vocab_size(const void* h){return static_cast<const n::CharacterEncoder*>(h)->vocab_size();}
void* coolbox_nlp_embedding_new(size_t dim){try{return new n::WordEmbedding(dim);}catch(...){return nullptr;}}
void coolbox_nlp_embedding_free(void* h){delete static_cast<n::WordEmbedding*>(h);}
int coolbox_nlp_embedding_init(void* h,const char* const* words,size_t c,int xavier,char* e,size_t n){return invoke(e,n,[&]{if(xavier)static_cast<n::WordEmbedding*>(h)->xavier_init(strings(words,c));else static_cast<n::WordEmbedding*>(h)->random_init(strings(words,c));});}
double* coolbox_nlp_embedding_get(const void* h,const char* word,size_t* c,char* e,size_t n){try{return malloc_copy(static_cast<const n::WordEmbedding*>(h)->get_embedding(str(word)),c);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
int coolbox_nlp_embedding_has(const void* h,const char* word){return static_cast<const n::WordEmbedding*>(h)->has_word(str(word));}
double* coolbox_nlp_embedding_batch(const void* h,const char* const* words,size_t c,size_t* rows,size_t* cols,char* e,size_t n){try{return matrix_out(static_cast<const n::WordEmbedding*>(h)->get_embeddings(strings(words,c)),rows,cols);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
int coolbox_nlp_embedding_update(void* h,const char* word,const double* v,size_t c,char* e,size_t n){return invoke(e,n,[&]{static_cast<n::WordEmbedding*>(h)->update_embedding(str(word),vec(v,c));});}
double coolbox_nlp_embedding_similarity(const void* h,const char* a,const char* b,char* e,size_t n){double out=0;invoke(e,n,[&]{out=static_cast<const n::WordEmbedding*>(h)->similarity(str(a),str(b));});return out;}
void* coolbox_nlp_embedding_similar(const void* h,const char* word,size_t k,char* e,size_t n){return invoke_handle(e,n,[&]()->void*{auto pairs=static_cast<const n::WordEmbedding*>(h)->most_similar(str(word),k);Strings* out=new Strings();for(auto& p:pairs)out->values.push_back(p.first+"\t"+std::to_string(p.second));return out;});}
size_t coolbox_nlp_embedding_vocab_size(const void* h){return static_cast<const n::WordEmbedding*>(h)->vocab_size();}
size_t coolbox_nlp_embedding_dim(const void* h){return static_cast<const n::WordEmbedding*>(h)->embedding_dim();}
void* coolbox_nlp_onehot_new(){try{return new n::OneHotEncoder();}catch(...){return nullptr;}}
void coolbox_nlp_onehot_free(void* h){delete static_cast<n::OneHotEncoder*>(h);}
int coolbox_nlp_onehot_fit(void* h,const char* const* w,size_t c,char* e,size_t n){return invoke(e,n,[&]{static_cast<n::OneHotEncoder*>(h)->fit(strings(w,c));});}
double* coolbox_nlp_onehot_encode(const void* h,const char* w,size_t* c,char* e,size_t n){try{return malloc_copy(static_cast<const n::OneHotEncoder*>(h)->encode(str(w)),c);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
double* coolbox_nlp_onehot_batch(const void* h,const char* const* w,size_t c,size_t* r,size_t* col,char* e,size_t n){try{return matrix_out(static_cast<const n::OneHotEncoder*>(h)->encode_batch(strings(w,c)),r,col);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
size_t coolbox_nlp_onehot_vocab_size(const void* h){return static_cast<const n::OneHotEncoder*>(h)->vocab_size();}
double coolbox_nlp_cosine(const double* a,size_t ac,const double* b,size_t bc,char* e,size_t n){double v=0;invoke(e,n,[&]{v=n::cosine_similarity(vec(a,ac),vec(b,bc));});return v;}
double coolbox_nlp_jaccard(const char* const* a,size_t ac,const char* const* b,size_t bc,char* e,size_t n){double v=0;invoke(e,n,[&]{auto av=strings(a,ac),bv=strings(b,bc);v=n::jaccard_similarity(std::set<std::string>(av.begin(),av.end()),std::set<std::string>(bv.begin(),bv.end()));});return v;}
size_t coolbox_nlp_levenshtein(const char* a,const char* b){return n::levenshtein_distance(str(a),str(b));}
double* coolbox_nlp_positional(size_t length,size_t dim,size_t* rows,size_t* cols,char* e,size_t n){try{return malloc_matrix(n::create_positional_encoding(length,dim),rows,cols);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
double* coolbox_nlp_pool(const double* a,size_t rows,size_t cols,int maximum,size_t* count,char* e,size_t n){try{auto m=rows_of(a,rows,cols);auto v=maximum?n::max_pooling_embeddings(m):n::average_embeddings(m);return malloc_copy(v,count);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
}


