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
        import sys
        sys.path.insert(0, ".")
        import ml_toolbox.ml_core as ml_core
        print("✓ ml_toolbox.ml_core imported successfully")

        # Test submodules (if available)
        for submodule in [
            "decision_tree",
            "svm",
            "bayesian_network",
            "hmm",
            "glm",
            "multi_arm_bandit",
            "graphics",
            "misc.wave_generator"
        ]:
            try:
                __import__(f"ml_toolbox.ml_core.{submodule}")
                print(f"✓ {submodule} module available")
            except ImportError:
                print(f"✗ {submodule} module not available")
        return True
    except ImportError as e:
        print(f"✗ Import failed: {e}")
        return False

def test_decision_tree():
    """Test decision tree functionality"""
    print("\nTesting Decision Tree...")
    try:
        import sys
        sys.path.insert(0, ".")
        import ml_toolbox.ml_core as ml_core
        
        # Simple XOR problem
        X = [[0, 0], [0, 1], [1, 0], [1, 1]]
        y = [0, 1, 1, 0]
        
        dt = ml_core.decision_tree.DecisionTree(ml_core.decision_tree.SplitCriterion.GINI)
        dt.fit(X, y, 3)
        
        # Test predictions
        for sample, expected in zip(X, y):
            pred = dt.predict(sample)
            print(f"  Input: {sample} -> Prediction: {pred}")
            assert pred == expected

        regressor = ml_core.decision_tree.DecisionTreeRegressor()
        regressor.fit([[0.1], [0.3], [0.7], [0.9]], [-2.5, -2.5, 4.5, 4.5], 1)
        assert regressor.predict([0.2]) == -2.5
        assert regressor.predict([0.8]) == 4.5
        regressor.fit([[0.0], [1.0]], [1.5, 4.5], 0)
        assert regressor.predict([0.5]) == 3.0

        forest = ml_core.decision_tree.RandomForestRegressor(30, 2, seed=123)
        forest.fit([[0.0], [0.1], [1.0], [1.1]], [-2.5, -2.5, 4.5, 4.5])
        assert forest.predict([0.0]) < forest.predict([1.0])

        params = ml_core.decision_tree.BoostTreeParameters()
        params.num_estimators = 60
        params.learning_rate = 0.3
        params.max_depth = 2
        boosted = ml_core.decision_tree.BoostTree(params)
        boosted.fit([[0.0], [1.0]], [-2.5, 4.5])
        assert abs(boosted.predict([0.0]) + 2.5) < 0.01
        assert len(boosted.predict_batch([[0.0], [1.0]])) == 2

        classifier = ml_core.decision_tree.BoostTreeClassifier(params)
        classifier.fit([[-1.0], [0.0], [1.0]], [-7, 42, 99])
        assert classifier.classes() == [-7, 42, 99]
        for sample, label in zip([[-1.0], [0.0], [1.0]], [-7, 42, 99]):
            assert classifier.predict(sample) == label
            probabilities = classifier.predict_proba(sample)
            assert len(probabilities) == 3
            assert all(0.0 <= value <= 1.0 for value in probabilities)
            assert abs(sum(probabilities) - 1.0) < 1e-12

        print("✓ Decision Tree test passed")
        return True
        
    except Exception as e:
        print(f"✗ Decision Tree test failed: {e}")
        return False

def test_svm():
    """Test SVM functionality"""
    print("\nTesting SVM...")
    try:
        import sys
        sys.path.insert(0, ".")
        import ml_toolbox.ml_core as ml_core
        
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
        import sys
        sys.path.insert(0, ".")
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
        import sys
        sys.path.insert(0, ".")
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
        import sys
        sys.path.insert(0, ".")
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
        import sys
        sys.path.insert(0, ".")
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

def test_graphics():
    """Test graphics bindings"""
    print("\nTesting Graphics...")
    try:
        import sys
        sys.path.insert(0, ".")
        import ml_core

        graph = ml_core.graphics.Graph(320, 240, ml_core.graphics.GraphType.LINE)
        graph.set_title("Demo")
        graph.set_x_label("x")
        graph.set_y_label("y")
        graph.add_series(
            ml_core.graphics.DataSeries(
                "sample",
                [0.0, 1.0, 2.0, 3.0],
                [1.0, 3.0, 2.0, 4.0],
                ml_core.graphics.BLUE,
            )
        )

        canvas = graph.render()
        pixels = canvas.to_numpy()
        print(f"  Rendered canvas shape: {pixels.shape}")

        plot_2d = ml_core.graphics.plot_2d(
            [0.0, 1.0, 2.0],
            [2.0, 1.0, 3.0],
            width=48,
            height=32,
        )
        assert plot_2d.width() == 48
        assert plot_2d.height() == 32
        assert plot_2d.to_numpy().shape == (32, 48, 4)

        plot_3d = ml_core.graphics.plot_3d(
            [0.0, 1.0, 2.0],
            [2.0, 1.0, 3.0],
            [1.0, 2.0, 0.0],
            width=48,
            height=32,
        )
        assert plot_3d.width() == 48
        assert plot_3d.height() == 32
        assert plot_3d.to_numpy().shape == (32, 48, 4)

        table = ml_core.graphics.Table()
        table.set_headers(["Name", "Value"])
        table.add_row(["alpha", "1"])
        table.add_row(["beta", "2"])
        table_canvas = table.render()
        print(f"  Table canvas size: {table_canvas.width()}x{table_canvas.height()}")

        print("✓ Graphics test passed")
        return True

    except Exception as e:
        print(f"✗ Graphics test failed: {e}")
        return False

def test_wave_generator():
    """Test wave generator bindings"""
    print("\nTesting Wave Generator...")
    try:
        import sys
        sys.path.insert(0, ".")
        import ml_core

        config = ml_core.misc.wave_generator.WaveConfig()
        config.amplitude = 2.0
        config.frequency_hz = 5.0

        sample = ml_core.misc.wave_generator.sample_at(
            0.05,
            ml_core.misc.wave_generator.WavePattern.SINE,
            config,
        )
        samples = ml_core.misc.wave_generator.generate_samples_for_duration(
            0.5,
            20.0,
            ml_core.misc.wave_generator.WavePattern.SAWTOOTH,
            config,
        )

        print(f"  Sample at t=0.05s: {sample}")
        print(f"  Generated {len(samples)} samples")

        print("✓ Wave Generator test passed")
        return True

    except Exception as e:
        print(f"✗ Wave Generator test failed: {e}")
        return False

def test_opt_math_finance():
    """Test optimization, heat solvers, and finance bindings."""
    print("\nTesting OPT, MATH, and FINANCE...")
    try:
        import ml_core

        price = ml_core.finance.evaluate(
            ml_core.finance.Formula.BLACK_SCHOLES_CALL_PRICE,
            [100.0, 100.0, 0.05, 0.2, 1.0],
        )
        assert abs(price - 10.4505835722) < 1e-6

        path = ml_core.finance.brownian_motion_path([0.25, 0.25], [0.0, 2.0])
        assert path == [0.0, 0.0, 1.0]

        solution = ml_core.math.solve_heat_equation(
            [0.0, 1.0, 0.0], (3, 1, 1), 1, 1.0, 1.0, 0.1, 1
        )
        assert 0.0 < solution[1] < 1.0

        optimizer = ml_core.opt.Optimizer(
            ml_core.opt.OptimizerType.HILL_CLIMBING,
            [1, 40, 20, 1, 0.25, -10, 10, 2026],
        )
        candidate = optimizer.optimize(lambda point: point[0] ** 2, [4.0])
        assert len(candidate) == 1
        assert optimizer.best_score <= 16.0

        average = ml_core.opt.naive_average_parameters([[1.0, 3.0], [3.0, 5.0]])
        assert average == [2.0, 4.0]

        config = ml_core.opt.OwaConfig()
        config.merge_iterations = 10
        cv = ml_core.opt.fast_owa_cross_validation_linear_regression(
            [[0.0, 1.0], [2.0, 1.0], [4.0, 3.0]],
            [[[1.0, 2.0]], [[2.0, 1.0]], [[1.0, 1.0]]],
            [[2.0], [1.0], [3.0]],
            config,
        )
        assert len(cv.fold_losses) == 3
        assert np.isfinite(cv.mean_loss)

        tensor = ml_core.deep_learning.Tensor
        local_layers = [
            [tensor([2], [1.0, 2.0]), tensor([1], [3.0])],
            [tensor([2], [3.0, 1.0]), tensor([1], [5.0])],
        ]
        weighted = ml_core.opt.weighted_tensor_average(
            [local_layers[0][0], local_layers[1][0]], [0.5, 0.5]
        )
        assert weighted.shape() == [2]
        assert weighted.data() == [2.0, 1.5]

        nowa = ml_core.opt.nonlinear_optimal_weighted_average_layers(
            local_layers,
            lambda layers: sum(sum(layer.data()) for layer in layers),
            config,
        )
        assert len(nowa.merged_layers) == 2
        assert len(nowa.layer_weights) == 2
        assert np.isfinite(nowa.validation_loss)

        def fail_validation(_layers):
            raise ValueError("validation callback failed")

        try:
            ml_core.opt.nonlinear_optimal_weighted_average_layers(
                local_layers, fail_validation, config
            )
            raise AssertionError("NOWA callback exception should propagate")
        except ValueError as error:
            assert str(error) == "validation callback failed"

        observed_paths = []

        def terminal_payoff(path):
            observed_paths.append(path)
            return path[-1]

        custom_estimate = ml_core.finance.monte_carlo_estimate_custom_payoff(
            100.0, 0.05, 0.2, [1.0], [[0.0]], terminal_payoff
        )
        assert custom_estimate.sample_count == 1
        assert np.isfinite(custom_estimate.price)
        assert len(observed_paths) == 1
        assert len(observed_paths[0]) == 2
        assert observed_paths[0][0] == 100.0

        def fail_payoff(_path):
            raise ValueError("payoff callback failed")

        try:
            ml_core.finance.monte_carlo_estimate_custom_payoff(
                100.0, 0.05, 0.2, [1.0], [[0.0]], fail_payoff
            )
            raise AssertionError("payoff callback exception should propagate")
        except ValueError as error:
            assert str(error) == "payoff callback failed"

        custom_greeks = ml_core.finance.monte_carlo_greeks_custom_payoff_fd(
            100.0, 100.0, 0.05, 0.2, [1.0], [[0.0]],
            lambda path: path[-1], time_bump_years=0.01,
        )
        assert np.isfinite(custom_greeks.delta.price)
        try:
            ml_core.finance.monte_carlo_greeks_custom_payoff_fd(
                100.0, 100.0, 0.05, 0.2, [1.0], [[0.0]], fail_payoff
            )
            raise AssertionError("Greek payoff callback exception should propagate")
        except ValueError as error:
            assert str(error) == "payoff callback failed"

        print("✓ OPT, MATH, and FINANCE tests passed")
        return True
    except Exception as e:
        print(f"✗ OPT, MATH, and FINANCE test failed: {e}")
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
        test_bandit,
        test_graphics,
        test_wave_generator,
        test_opt_math_finance,
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