import ml_toolbox.ml_core as ml_core

print("Extension import OK")

try:
    fit_method = ml_core.glm.LinearRegressionFitMethod(
        100, 0.01, ml_core.glm.LinearRegressionType.GRADIENT_DESCENT
    )
    lr = ml_core.glm.LinearRegression(fit_method)
    print("LinearRegression instance created")
    # Minimal input: 2 samples, 2 features
    X = [[1.0, 2.0], [3.0, 4.0]]
    y = [5.0, 6.0]
    lr.fit(X, y)
    print("fit completed without crash")
except Exception as e:
    print(f"Exception: {e}")
