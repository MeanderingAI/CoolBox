package coolbox

/*
#include <stdlib.h>
char* coolbox_nlp_lowercase(const char* text);
char* coolbox_nlp_remove_punctuation(const char* text);
char* coolbox_nlp_remove_numbers(const char* text);
char* coolbox_nlp_normalize_whitespace(const char* text);
void coolbox_nlp_free_string(char* value);
void* coolbox_nlp_tokenize(const char* text, int mode);
size_t coolbox_nlp_token_count(const void* handle);
char* coolbox_nlp_token_at(const void* handle, size_t index);
void coolbox_nlp_tokens_free(void* handle);
*/
import "C"

import (
	"errors"
	"unsafe"
)

// TextLowercase lowercases text using the native NLP text processor.
func TextLowercase(text string) (string, error) {
	return nlpStringTransform(text, 0)
}

// TextRemovePunctuation removes punctuation using the native NLP processor.
func TextRemovePunctuation(text string) (string, error) {
	return nlpStringTransform(text, 1)
}

// TextRemoveNumbers removes numeric characters using the native NLP processor.
func TextRemoveNumbers(text string) (string, error) {
	return nlpStringTransform(text, 2)
}

// TextNormalizeWhitespace collapses whitespace runs and trims leading/trailing whitespace.
func TextNormalizeWhitespace(text string) (string, error) {
	return nlpStringTransform(text, 3)
}

// TextTokenize splits text at spaces, matching TextProcessor.tokenize's default delimiter.
func TextTokenize(text string) ([]string, error) {
	return nlpTokenize(text, 0)
}

// TextWordTokenize tokenizes text into words.
func TextWordTokenize(text string) ([]string, error) {
	return nlpTokenize(text, 1)
}

// TextSentenceTokenize splits text into sentences.
func TextSentenceTokenize(text string) ([]string, error) {
	return nlpTokenize(text, 2)
}

// TextProcess runs the Python TextProcessor.process default preprocessing pipeline.
func TextProcess(text string) ([]string, error) {
	return nlpTokenize(text, 3)
}

func nlpStringTransform(text string, mode int) (string, error) {
	input, err := cStringInput(text)
	if err != nil {
		return "", err
	}
	defer C.free(unsafe.Pointer(input))
	var output *C.char
	switch mode {
	case 0:
		output = C.coolbox_nlp_lowercase(input)
	case 1:
		output = C.coolbox_nlp_remove_punctuation(input)
	case 2:
		output = C.coolbox_nlp_remove_numbers(input)
	case 3:
		output = C.coolbox_nlp_normalize_whitespace(input)
	default:
		return "", errors.New("coolbox: invalid native NLP transformation")
	}
	if output == nil {
		return "", errors.New("coolbox: native NLP text transformation failed")
	}
	defer C.coolbox_nlp_free_string(output)
	return C.GoString(output), nil
}

func nlpTokenize(text string, mode C.int) ([]string, error) {
	input, err := cStringInput(text)
	if err != nil {
		return nil, err
	}
	defer C.free(unsafe.Pointer(input))
	handle := C.coolbox_nlp_tokenize(input, mode)
	if handle == nil {
		return nil, errors.New("coolbox: native NLP tokenization failed")
	}
	defer C.coolbox_nlp_tokens_free(handle)

	count := uint64(C.coolbox_nlp_token_count(handle))
	tokens := make([]string, 0, count)
	for index := uint64(0); index < count; index++ {
		token := C.coolbox_nlp_token_at(handle, C.size_t(index))
		if token == nil {
			return nil, errors.New("coolbox: native NLP token retrieval failed")
		}
		tokens = append(tokens, C.GoString(token))
		C.coolbox_nlp_free_string(token)
	}
	return tokens, nil
}
