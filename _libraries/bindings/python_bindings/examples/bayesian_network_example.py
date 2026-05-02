#!/usr/bin/env python3
"""
Example usage of the Bayesian Network Python bindings
"""

import numpy as np
import ml_core

def test_bayesian_network():
    print("Testing Bayesian Network...")
    
    # Create a simple Bayesian network
    # Variables: Weather (Sunny, Rainy), Sprinkler (On, Off), Grass (Wet, Dry)
    bn = ml_core.bayesian_network.BayesianNetwork()
    
    # Add nodes
    weather_idx = bn.add_node("Weather", ["Sunny", "Rainy"])
    sprinkler_idx = bn.add_node("Sprinkler", ["On", "Off"])
    grass_idx = bn.add_node("Grass", ["Wet", "Dry"])
    
    print(f"Added nodes - Weather: {weather_idx}, Sprinkler: {sprinkler_idx}, Grass: {grass_idx}")
    
    # Add edges: Weather -> Sprinkler, Weather -> Grass, Sprinkler -> Grass
    bn.add_edge(weather_idx, sprinkler_idx)  # Weather influences Sprinkler
    bn.add_edge(weather_idx, grass_idx)      # Weather influences Grass
    bn.add_edge(sprinkler_idx, grass_idx)    # Sprinkler influences Grass
    
    # Set conditional probability tables
    
    # P(Weather) - Prior probabilities
    weather_cpt = np.array([[0.7, 0.3]])  # [P(Sunny), P(Rainy)]
    bn.set_cpt(weather_idx, weather_cpt)
    
    # P(Sprinkler | Weather)
    # Rows: Weather states (Sunny, Rainy)
    # Cols: Sprinkler states (On, Off)
    sprinkler_cpt = np.array([
        [0.1, 0.9],  # P(Sprinkler | Weather=Sunny)
        [0.01, 0.99] # P(Sprinkler | Weather=Rainy)
    ])
    bn.set_cpt(sprinkler_idx, sprinkler_cpt)
    
    # P(Grass | Weather, Sprinkler) 
    # This is more complex as it depends on two parents
    # We need to consider all combinations of parent states
    # Order: (Weather=Sunny,Sprinkler=On), (Weather=Sunny,Sprinkler=Off), 
    #        (Weather=Rainy,Sprinkler=On), (Weather=Rainy,Sprinkler=Off)
    grass_cpt = np.array([
        [0.99, 0.01],  # P(Grass | Weather=Sunny, Sprinkler=On)
        [0.2, 0.8],    # P(Grass | Weather=Sunny, Sprinkler=Off)
        [0.99, 0.01],  # P(Grass | Weather=Rainy, Sprinkler=On)  
        [0.9, 0.1]     # P(Grass | Weather=Rainy, Sprinkler=Off)
    ])
    bn.set_cpt(grass_idx, grass_cpt)
    
    # Test joint probability calculation
    # P(Weather=Sunny, Sprinkler=Off, Grass=Wet)
    assignment = {
        weather_idx: 0,    # Sunny
        sprinkler_idx: 1,  # Off
        grass_idx: 0       # Wet
    }
    
    joint_prob = bn.calculate_joint_probability(assignment)
    print(f"P(Weather=Sunny, Sprinkler=Off, Grass=Wet) = {joint_prob:.4f}")
    
    # Test inference
    # P(Grass=Wet | Weather=Sunny)
    evidence = {weather_idx: 0}  # Weather=Sunny
    
    prob_grass_wet = bn.infer(grass_idx, 0, evidence)  # Grass=Wet
    print(f"P(Grass=Wet | Weather=Sunny) = {prob_grass_wet:.4f}")
    
    # P(Weather=Rainy | Grass=Wet)
    evidence = {grass_idx: 0}  # Grass=Wet
    
    prob_weather_rainy = bn.infer(weather_idx, 1, evidence)  # Weather=Rainy
    print(f"P(Weather=Rainy | Grass=Wet) = {prob_weather_rainy:.4f}")

if __name__ == "__main__":
    test_bayesian_network()