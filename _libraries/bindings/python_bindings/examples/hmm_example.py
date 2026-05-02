#!/usr/bin/env python3
"""
Example usage of the Hidden Markov Model Python bindings
"""

import numpy as np
import ml_core

def test_hmm():
    print("Testing Hidden Markov Model...")
    
    # Create HMM with 2 hidden states and 3 observable symbols
    hmm = ml_core.hmm.HMM(states=2, observations=3)
    
    # Set initial probabilities
    initial_probs = np.array([0.6, 0.4])
    hmm.set_initial_probabilities(initial_probs)
    
    # Set transition matrix
    transition_matrix = np.array([
        [0.7, 0.3],
        [0.4, 0.6]
    ])
    hmm.set_transition_matrix(transition_matrix)
    
    # Set emission matrix
    emission_matrix = np.array([
        [0.5, 0.4, 0.1],  # State 0 emission probabilities
        [0.1, 0.3, 0.6]   # State 1 emission probabilities  
    ])
    hmm.set_emission_matrix(emission_matrix)
    
    # Test observation sequence
    observations = [0, 1, 2, 1, 0]
    
    # Calculate log likelihood
    log_likelihood = hmm.log_likelihood(observations)
    print(f"Log likelihood of observation sequence {observations}: {log_likelihood}")
    
    # Find most likely hidden state sequence
    most_likely_states = hmm.get_most_likely_states(observations)
    print(f"Most likely hidden state sequence: {most_likely_states}")
    
    # Train HMM on multiple sequences
    print("\nTraining HMM...")
    training_sequences = [
        [0, 1, 2, 1, 0],
        [2, 1, 0, 0, 1],
        [1, 1, 2, 0, 2],
        [0, 2, 1, 1, 0]
    ]
    
    hmm.train(training_sequences, max_iterations=50, tolerance=1e-4)
    
    print("Training completed!")
    print("Updated transition matrix:")
    print(hmm.get_transition_matrix())
    print("Updated emission matrix:")
    print(hmm.get_emission_matrix())

if __name__ == "__main__":
    test_hmm()