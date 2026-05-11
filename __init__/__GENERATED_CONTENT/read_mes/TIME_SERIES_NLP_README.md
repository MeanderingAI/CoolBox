# Time Series and NLP Module Documentation

## Overview

Two new modules have been added to the ToolBox library:
- **Time Series Analysis**: Comprehensive time series processing, forecasting, and analysis
- **Natural Language Processing (NLP)**: Text processing, tokenization, embeddings, and representations

## Time Series Module

### Features

#### Core Classes
- **TimeSeries**: Univariate time series with preprocessing and transformations
- **MultivariatTimeSeries**: Multiple feature time series for complex analysis

#### Preprocessing
- Normalization (Z-score, min-max scaling)
- Differencing and log transforms
- Moving average and exponential smoothing
- Resampling and interpolation

#### Analysis
- Statistical measures (mean, std, median, min, max)
- Autocorrelation analysis
- Seasonal decomposition
- Outlier detection (Z-score, IQR methods)

#### Forecasting Models
- **MovingAverageForecaster**: Simple moving average predictions
- **ExponentialSmoothingForecaster**: Level, trend, and seasonal smoothing
- **AutoRegressiveModel**: AR(p) model with least squares estimation

#### ML Integration
- Sliding window creation for sequence models
- Supervised learning dataset generation
- Batch processing support

### Example Usage

```python
import ml_core

# Create time series
ts = ml_core.time_series.TimeSeries(values)

# Preprocess
ts_norm = ts.normalize()
ts_ma = ts.moving_average(5)

# Create windows for ML
windows = ts.create_windows(10, 1)
X, y = ts.create_supervised_windows(3, 1, 1)

# Forecasting
forecaster = ml_core.time_series.AutoRegressiveModel(3)
forecaster.fit(ts)
predictions = forecaster.forecast(10)

# Analysis
acf = ts.autocorrelation(10)
decomp = ml_core.time_series.seasonal_decompose(ts, 12)
```

## NLP Module

### Features

#### Text Processing
- **TextProcessor**: Comprehensive text preprocessing pipeline
  - Lowercase conversion, punctuation/number removal
  - Tokenization (word, sentence, n-grams)
  - Stemming (basic Porter-like algorithm)
  - Stop word removal

#### Vocabulary Management
- **Vocabulary**: Token-to-index mapping with special tokens
  - Frequency-based filtering
  - Size constraints
  - Special tokens: PAD, UNK, BOS, EOS

#### Text Representations
- **BagOfWords**: Term frequency vectors
- **TFIDF**: TF-IDF weighted representations
- **SequenceEncoder**: Padded/truncated sequences for neural networks
- **CharacterEncoder**: Character-level encoding

#### Word Embeddings
- **WordEmbedding**: Word vector representations
  - Random/Xavier initialization
  - Similarity computation
  - Most similar word queries
- **OneHotEncoder**: One-hot vector encoding
- **Positional Encoding**: Transformer-style positional embeddings

#### Utilities
- Cosine similarity
- Jaccard similarity
- Levenshtein edit distance
- Embedding pooling (average, max)

### Example Usage

```python
import ml_core

# Text preprocessing
processor = ml_core.nlp.TextProcessor()
text = "The QUICK brown foxes are JUMPING!"
tokens = processor.process(text, lowercase=True, remove_stops=True, apply_stemming=True)

# Vocabulary building
vocab = ml_core.nlp.Vocabulary(min_freq=1)
vocab.build(documents)
indices = vocab.encode(tokens)

# TF-IDF representation
tfidf = ml_core.nlp.TFIDF()
tfidf.fit(documents)
vectors = tfidf.transform(tokens)

# Sequence encoding
encoder = ml_core.nlp.SequenceEncoder(vocab, max_length=50, padding=True)
padded_sequences = encoder.encode_batch(documents)

# Word embeddings
embeddings = ml_core.nlp.WordEmbedding(embedding_dim=100)
embeddings.xavier_init(vocabulary)
vec = embeddings.get_embedding("word")
similar = embeddings.most_similar("word", top_k=5)

# Similarity metrics
sim = ml_core.nlp.cosine_similarity(vec1, vec2)
dist = ml_core.nlp.levenshtein_distance("kitten", "sitting")
```

## Examples

Run the comprehensive examples:

```bash
cd python_bindings
PYTHONPATH=. python3 ../examples/time_series_example.py
PYTHONPATH=. python3 ../examples/nlp_example.py
```

The examples demonstrate:
- All time series preprocessing techniques
- Forecasting with multiple models
- Seasonal decomposition and outlier detection
- Complete text processing pipelines
- Vocabulary building and encoding
- Word embeddings and similarity metrics
- Positional encoding for transformers

## Integration with Existing Modules

Both modules integrate seamlessly with the existing deep learning and computer vision modules:
- Time series windows can be fed directly to RNN/LSTM layers
- Text sequences can be processed by embedding layers
- Combined with CV for multimodal learning (image + text/time)

## Building

The modules are automatically built with the main library:

```bash
# C++ libraries
cd build
cmake ..
make time_series nlp

# Python bindings
cd python_bindings
python3 setup.py build_ext --inplace
```

## API Documentation

### Time Series Classes

- `TimeSeries(values, timestamps=[])`
- `MultivariatTimeSeries(data, feature_names=[], timestamps=[])`
- `MovingAverageForecaster(window_size)`
- `ExponentialSmoothingForecaster(alpha, beta=0, gamma=0)`
- `AutoRegressiveModel(order)`

### NLP Classes

- `TextProcessor()`
- `Vocabulary(min_freq=1, max_size=0)`
- `BagOfWords(vocab=None)`
- `TFIDF(vocab=None)`
- `SequenceEncoder(vocab, max_length=0, padding=True, truncation=True)`
- `CharacterEncoder()`
- `WordEmbedding(embedding_dim)`
- `OneHotEncoder()`

See the example files for detailed usage patterns.
