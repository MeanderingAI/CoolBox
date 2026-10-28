package io.coolbox.glm;
import org.junit.jupiter.api.Test;import static org.junit.jupiter.api.Assertions.*;
class GlmParityTest {@Test void supportsFitMethodValueObject(){LinearRegressionFitMethod method=new LinearRegressionFitMethod(20,.01,FitMethod.CLOSED_FORM);assertEquals(20,method.getNumIterations());try(LinearRegression model=new LinearRegression(method).fit(new double[][]{{0},{1},{2}},new double[]{1,3,5})){assertEquals(7,model.predict(new double[]{3}),1e-6);assertEquals(2,model.coefficients()[0],1e-6);}}}
