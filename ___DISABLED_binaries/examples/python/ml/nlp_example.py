#!/usr/bin/env python3
"""
Natural Language Processing (NLP) Examples

Demonstrates text processing, tokenization, embeddings, and text representations.
"""

import sys
sys.path.insert(0, './build')
import ml_core

def example_text_preprocessing():
    """Basic text preprocessing"""
    print("=" * 60)
    print("Example 1: Text Preprocessing")
    print("=" * 60)
    
    text = "  Hello, World! This is a SAMPLE text with Numbers 123.  "
    
    processor = ml_core.nlp.TextProcessor()
    
    print(f"Original: '{text}'")
    print(f"Lowercase: '{processor.to_lowercase(text)}'")
    print(f"Remove punct: '{processor.remove_punctuation(text)}'")
    print(f"Remove numbers: '{processor.remove_numbers(text)}'")
    print(f"Strip: '{processor.strip(text)}'")
    print(f"Remove extra whitespace: '{processor.remove_extra_whitespace(text)}'")
    print()

def example_tokenization():
    """Tokenization examples"""
    print("=" * 60)
    print("Example 2: Tokenization")
    print("=" * 60)
    
    text = "The quick brown fox jumps over the lazy dog. What a beautiful day!"
    
    processor = ml_core.nlp.TextProcessor()
    
    # Word tokenization
    words = processor.word_tokenize(text)
    print(f"Words: {words}")
    
    # Sentence tokenization
    sentences = processor.sentence_tokenize(text)
    print(f"Sentences: {sentences}")
    
    # N-grams
    bigrams = processor.generate_ngrams(words, 2)
    print(f"Bigrams: {bigrams[:3]}...")
    
    trigrams = processor.generate_ngrams(words, 3)
    print(f"Trigrams: {trigrams[:3]}...")
    print()

def example_text_pipeline():
    """Full text processing pipeline"""
    print("=" * 60)
    print("Example 3: Text Processing Pipeline")
    print("=" * 60)
    
    text = "The QUICK brown foxes are JUMPING over the lazy dogs!"
    
    processor = ml_core.nlp.TextProcessor()
    
    # Full pipeline
    tokens = processor.process(
        text,
        lowercase=True,
        remove_punct=True,
        remove_nums=False,
        remove_stops=True,
        apply_stemming=True
    )
    
    print(f"Original: {text}")
    print(f"Processed tokens: {tokens}")
    print()

def example_vocabulary():
    """Vocabulary building and encoding"""
    print("=" * 60)
    print("Example 4: Vocabulary Management")
    print("=" * 60)
    
    # Sample documents
    documents = [
        ["the", "cat", "sat", "on", "the", "mat"],
        ["the", "dog", "ran", "in", "the", "park"],
        ["cat", "and", "dog", "are", "friends"]
    ]
    
    # Build vocabulary
    vocab = ml_core.nlp.Vocabulary(min_freq=1, max_size=0)
    vocab.build(documents)
    
    print(f"Vocabulary size: {vocab.size()}")
    print(f"'cat' -> {vocab.token_to_index('cat')}")
    print(f"'dog' -> {vocab.token_to_index('dog')}")
    print(f"'unknown' -> {vocab.token_to_index('unknown')} (UNK)")
    
    # Encode/decode
    tokens = ["the", "cat", "sat"]
    indices = vocab.encode(tokens)
    decoded = vocab.decode(indices)
    print(f"\nTokens: {tokens}")
    print(f"Encoded: {indices}")
    print(f"Decoded: {decoded}")
    
    # Check frequencies
    print(f"\n'the' frequency: {vocab.frequency('the')}")
    print(f"'cat' frequency: {vocab.frequency('cat')}")
    print()

def example_bag_of_words():
    """Bag of Words representation"""
    print("=" * 60)
    print("Example 5: Bag of Words")
    print("=" * 60)
    
    documents = [
        ["machine", "learning", "is", "fun"],
        ["deep", "learning", "is", "powerful"],
        ["machine", "learning", "and", "deep", "learning"]
    ]
    
    # Create and fit BoW
    bow = ml_core.nlp.BagOfWords()
    bow.fit(documents)
    
    print(f"Vocabulary size: {bow.vocabulary().size()}")
    
    # Transform documents
    for i, doc in enumerate(documents):
        vec = bow.transform(doc)
        non_zero = [(idx, val) for idx, val in enumerate(vec) if val > 0]
        print(f"Doc {i}: {doc}")
        print(f"  BoW (non-zero): {non_zero[:5]}...")
    print()

def example_tfidf():
    """TF-IDF representation"""
    print("=" * 60)
    print("Example 6: TF-IDF")
    print("=" * 60)
    
    documents = [
        ["machine", "learning", "is", "fun"],
        ["deep", "learning", "is", "powerful"],
        ["machine", "learning", "and", "deep", "learning"]
    ]
    
    # Create and fit TF-IDF
    tfidf = ml_core.nlp.TFIDF()
    tfidf.fit(documents)
    
    print(f"Vocabulary size: {tfidf.vocabulary().size()}")
    
    # Transform documents
    for i, doc in enumerate(documents):
        vec = tfidf.transform(doc)
        non_zero = [(idx, f"{val:.3f}") for idx, val in enumerate(vec) if val > 0]
        print(f"Doc {i}: {doc}")
        print(f"  TF-IDF (non-zero): {non_zero[:5]}...")
    print()

def example_sequence_encoding():
    """Sequence encoding with padding"""
    print("=" * 60)
    print("Example 7: Sequence Encoding")
    print("=" * 60)
    
    # Build vocabulary
    documents = [
        ["hello", "world"],
        ["machine", "learning", "is", "great"],
        ["short"],
        ["this", "is", "a", "longer", "sentence", "example"]
    ]
    
    vocab = ml_core.nlp.Vocabulary()
    vocab.build(documents)
    
    # Create encoder with padding
    encoder = ml_core.nlp.SequenceEncoder(vocab, max_length=6, padding=True, truncation=True)
    
    # Encode sequences
    for doc in documents:
        encoded = encoder.encode(doc)
        print(f"Tokens: {doc}")
        print(f"Encoded (padded to 6): {encoded}")
    
    # Decode
    decoded = encoder.decode(encoded, skip_special=True)
    print(f"\nDecoded (last): {decoded}")
    print()

def example_character_encoding():
    """Character-level encoding"""
    print("=" * 60)
    print("Example 8: Character Encoding")
    print("=" * 60)
    
    texts = ["hello", "world", "machine learning"]
    
    # Build character vocabulary
    char_encoder = ml_core.nlp.CharacterEncoder()
    char_encoder.fit(texts)
    
    print(f"Character vocabulary size: {char_encoder.vocab_size()}")
    
    # Encode text
    text = "hello"
    encoded = char_encoder.encode(text)
    decoded = char_encoder.decode(encoded)
    
    print(f"\nText: '{text}'")
    print(f"Encoded: {encoded}")
    print(f"Decoded: '{decoded}'")
    print()

def example_word_embeddings():
    """Word embeddings"""
    print("=" * 60)
    print("Example 9: Word Embeddings")
    print("=" * 60)
    
    vocabulary = ["cat", "dog", "mouse", "animal", "pet", "house", "car"]
    
    # Create embeddings
    embeddings = ml_core.nlp.WordEmbedding(embedding_dim=50)
    embeddings.random_init(vocabulary)
    
    print(f"Vocabulary size: {embeddings.vocab_size()}")
    print(f"Embedding dimension: {embeddings.embedding_dim()}")
    
    # Get embedding
    cat_emb = embeddings.get_embedding("cat")
    print(f"\n'cat' embedding (first 5 dims): {cat_emb[:5]}")
    
    # Calculate similarity
    sim = embeddings.similarity("cat", "dog")
    print(f"\nSimilarity('cat', 'dog'): {sim:.3f}")
    
    # Find most similar
    most_similar = embeddings.most_similar("cat", top_k=3)
    print(f"\nMost similar to 'cat': {[(word, f'{score:.3f}') for word, score in most_similar]}")
    print()

def example_similarity_metrics():
    """Text similarity metrics"""
    print("=" * 60)
    print("Example 10: Similarity Metrics")
    print("=" * 60)
    
    # Cosine similarity
    vec1 = [1.0, 2.0, 3.0, 4.0]
    vec2 = [2.0, 3.0, 4.0, 5.0]
    cosine_sim = ml_core.nlp.cosine_similarity(vec1, vec2)
    print(f"Cosine similarity: {cosine_sim:.3f}")
    
    # Jaccard similarity
    set1 = {"cat", "dog", "mouse"}
    set2 = {"dog", "mouse", "bird"}
    jaccard_sim = ml_core.nlp.jaccard_similarity(set1, set2)
    print(f"Jaccard similarity: {jaccard_sim:.3f}")
    
    # Levenshtein distance
    s1 = "kitten"
    s2 = "sitting"
    lev_dist = ml_core.nlp.levenshtein_distance(s1, s2)
    print(f"Levenshtein distance('{s1}', '{s2}'): {lev_dist}")
    print()

def example_positional_encoding():
    """Positional encoding for transformers"""
    print("=" * 60)
    print("Example 11: Positional Encoding")
    print("=" * 60)
    
    max_length = 10
    embedding_dim = 8
    
    pos_enc = ml_core.nlp.create_positional_encoding(max_length, embedding_dim)
    
    print(f"Positional encoding shape: {len(pos_enc)} x {len(pos_enc[0])}")
    print(f"\nFirst position encoding:")
    print(f"  {[f'{v:.3f}' for v in pos_enc[0]]}")
    print(f"\nSecond position encoding:")
    print(f"  {[f'{v:.3f}' for v in pos_enc[1]]}")
    print()

def example_embedding_pooling():
    """Embedding pooling operations"""
    print("=" * 60)
    print("Example 12: Embedding Pooling")
    print("=" * 60)
    
    # Sequence of embeddings
    embeddings = [
        [1.0, 2.0, 3.0],
        [2.0, 3.0, 4.0],
        [3.0, 4.0, 5.0]
    ]
    
    # Average pooling
    avg = ml_core.nlp.average_embeddings(embeddings)
    print(f"Average pooling: {avg}")
    
    # Max pooling
    max_pool = ml_core.nlp.max_pooling_embeddings(embeddings)
    print(f"Max pooling: {max_pool}")
    print()

if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("NATURAL LANGUAGE PROCESSING EXAMPLES")
    print("=" * 60 + "\n")
    
    example_text_preprocessing()
    example_tokenization()
    example_text_pipeline()
    example_vocabulary()
    example_bag_of_words()
    example_tfidf()
    example_sequence_encoding()
    example_character_encoding()
    example_word_embeddings()
    example_similarity_metrics()
    example_positional_encoding()
    example_embedding_pooling()
    
    print("=" * 60)
    print("All examples completed successfully!")
    print("=" * 60)
