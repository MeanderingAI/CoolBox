#!/usr/bin/env python3
"""
Comprehensive test script for all ML Core Python bindings
"""

import sys
import numpy as np

def test_imports():
    """Test if all modules can be imported"""
    print("Testing imports...")
    try:
        import ml_core
        print("✓ ml_core imported successfully")
        
        # Test submodules
        import ml_core.decision_tree
        print("✓ decision_tree module available")
        
        import ml_core.svm
        print("✓ svm module available")
        
        import ml_core.bayesian_network
        print("✓ bayesian_network module available")
        
        import ml_core.hmm
        print("✓ hmm module available")
        
        import ml_core.glm
        print("✓ glm module available")
        
        import ml_core.multi_arm_bandit
        print("✓ multi_arm_bandit module available")
        
        return True
        
    except ImportError as e:
        print(f"✗ Import failed: {e}")
        return False

def test_decision_tree():
    """Test decision tree functionality"""
    print("\nTesting Decision Tree...")
    try:
        import ml_core
        
        # Simple XOR problem
        X = [[0, 0], [0, 1], [1, 0], [1, 1]]
        y = [0, 1, 1, 0]
        
        dt = ml_core.decision_tree.DecisionTree(ml_core.decision_tree.SplitCriterion.GINI)
        dt.fit(X, y, 3)
        
        # Test predictions
        for sample in X:
            pred = dt.predict(sample)
            print(f"  Input: {sample} -> Prediction: {pred}")
            
        print("✓ Decision Tree test passed")
        return True
        
    except Exception as e:
        print(f"✗ Decision Tree test failed: {e}")
        return False

def test_svm():
    """Test SVM functionality"""
    print("\nTesting SVM...")
    try:
        import ml_core
        
        # Simple linearly separable data
        X = np.array([[1.0, 1.0], [2.0, 2.0], [5.0, 5.0], [6.0, 6.0]])
        y = np.array([-1.0, -1.0, 1.0, 1.0])
        
        # Test linear kernel
        linear_kernel = ml_core.svm.LinearKernel()
        svm = ml_core.svm.SVM(linear_kernel)
        svm.fit(X, y)
        
        test_point = np.array([3.0, 3.0])
        prediction = svm.predict(test_point)
        print(f"  Linear SVM prediction for {test_point}: {prediction}")
        
        # Test RBF kernel
        rbf_kernel = ml_core.svm.RBFKernel(1.0)
        svm_rbf = ml_core.svm.SVM(rbf_kernel)
        svm_rbf.fit(X, y)
        
        prediction_rbf = svm_rbf.predict(test_point)
        print(f"  RBF SVM prediction for {test_point}: {prediction_rbf}")
        
        print("✓ SVM test passed")
        return True
        
    except Exception as e:
        print(f"✗ SVM test failed: {e}")
        return False

def test_hmm():
    """Test HMM functionality"""
    print("\nTesting HMM...")
    try:
        import ml_core
        
        # Create 2-state, 2-observation HMM
        hmm = ml_core.hmm.HMM(2, 2)
        
        # Set parameters
        initial = np.array([0.8, 0.2])
        transition = np.array([[0.9, 0.1], [0.2, 0.8]])
        emission = np.array([[0.9, 0.1], [0.2, 0.8]])
        
        hmm.set_initial_probabilities(initial)
        hmm.set_transition_matrix(transition)
        hmm.set_emission_matrix(emission)
        
        # Test with observation sequence
        obs = [0, 1, 0, 1]
        likelihood = hmm.log_likelihood(obs)
        states = hmm.get_most_likely_states(obs)
        
        print(f"  Observation sequence: {obs}")
        print(f"  Log likelihood: {likelihood}")
        print(f"  Most likely states: {states}")
        
        print("✓ HMM test passed")
        return True
        
    except Exception as e:
        print(f"✗ HMM test failed: {e}")
        return False

def test_linear_regression():
    """Test Linear Regression functionality"""
    print("\nTesting Linear Regression...")
    try:
        import ml_core
        
        # Simple linear data: y = 2*x + 1
        X = [[1.0], [2.0], [3.0], [4.0]]
        y = [3.0, 5.0, 7.0, 9.0]
        
        # Test gradient descent
        gd_method = ml_core.glm.LinearRegressionFitMethod(
            1000, 0.01, ml_core.glm.LinearRegressionType.GRADIENT_DESCENT
        )
        lr = ml_core.glm.LinearRegression(gd_method)
        lr.fit(X, y)
        
        test_input = [5.0]
        prediction = lr.predict(test_input)
        weights, bias = lr.get_coefficients()
        
        print(f"  Input: {test_input} -> Prediction: {prediction}")
        print(f"  Learned weights: {weights}, bias: {bias}")
        
        print("✓ Linear Regression test passed")
        return True
        
    except Exception as e:
        print(f"✗ Linear Regression test failed: {e}")
        return False

def test_bayesian_network():
    """Test Bayesian Network functionality"""
    print("\nTesting Bayesian Network...")
    try:
        import ml_core
        
        bn = ml_core.bayesian_network.BayesianNetwork()
        
        # Add nodes
        a = bn.add_node("A", ["true", "false"])
        b = bn.add_node("B", ["true", "false"])
        
        # Add edge A -> B
        bn.add_edge(a, b)
        
        # Set CPTs
        a_cpt = np.array([[0.6, 0.4]])  # P(A)
        bn.set_cpt(a, a_cpt)
        
        b_cpt = np.array([[0.8, 0.2], [0.3, 0.7]])  # P(B|A)
        bn.set_cpt(b, b_cpt)
        
        # Test joint probability
        assignment = {a: 0, b: 0}  # A=true, B=true
        joint_prob = bn.calculate_joint_probability(assignment)
        
        # Test inference
        evidence = {a: 0}  # A=true
        conditional_prob = bn.infer(b, 0, evidence)  # P(B=true | A=true)
        
        print(f"  P(A=true, B=true) = {joint_prob}")
        print(f"  P(B=true | A=true) = {conditional_prob}")
        
        print("✓ Bayesian Network test passed")
        return True
        
    except Exception as e:
        print(f"✗ Bayesian Network test failed: {e}")
        return False

def test_bandit():
    """Test Multi-arm Bandit functionality"""
    print("\nTesting Multi-arm Bandit...")
    try:
        import ml_core
        
        # Create bandit arm with 0.7 true reward probability
        arm = ml_core.multi_arm_bandit.BanditArm(0.7)
        
        # Pull arm multiple times
        rewards = []
        for _ in range(10):
            reward = arm.pull()
            arm.update(reward)
            rewards.append(reward)
        
        estimated_prob = arm.get_estimated_prob()
        pull_count = arm.get_pull_count()
        true_prob = arm.get_true_prob()
        
        print(f"  Rewards: {rewards}")
        print(f"  True probability: {true_prob}")
        print(f"  Estimated probability: {estimated_prob}")
        print(f"  Pull count: {pull_count}")
        
        print("✓ Multi-arm Bandit test passed")
        return True
        
    except Exception as e:
        print(f"✗ Multi-arm Bandit test failed: {e}")
        return False

def main():
    """Run all tests"""
    print("Running ML Core Python Bindings Test Suite")
    print("=" * 50)
    
    tests = [
        test_imports,
        test_decision_tree,
        test_svm,
        test_hmm,
        test_linear_regression,
        test_bayesian_network,
        test_bandit
    ]
    
    passed = 0
    total = len(tests)
    
    for test in tests:
        try:
            if test():
                passed += 1
        except Exception as e:
            print(f"✗ Test {test.__name__} failed with exception: {e}")
    
    print("\n" + "=" * 50)
    print(f"Test Results: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 All tests passed!")
        return 0
    else:
        print("❌ Some tests failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())