#include <string.h>

// Fractal JNI
JNIEXPORT jlong JNICALL Java_coolbox_graphics_Fractal_create(JNIEnv* env, jclass cls, jint width, jint height, jint type) {
    return (jlong)coolbox_fractal_create(width, height, type);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_setParams(JNIEnv* env, jclass cls, jlong handle, jdouble param1, jdouble param2) {
    coolbox_fractal_set_params((CoolBoxFractal*)handle, param1, param2);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_setMaxIter(JNIEnv* env, jclass cls, jlong handle, jint maxIter) {
    coolbox_fractal_set_max_iter((CoolBoxFractal*)handle, maxIter);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_setBounds(JNIEnv* env, jclass cls, jlong handle, jdouble xMin, jdouble xMax, jdouble yMin, jdouble yMax) {
    coolbox_fractal_set_bounds((CoolBoxFractal*)handle, xMin, xMax, yMin, yMax);
}
JNIEXPORT jlong JNICALL Java_coolbox_graphics_Fractal_render(JNIEnv* env, jclass cls, jlong handle) {
    return (jlong)coolbox_fractal_render((CoolBoxFractal*)handle);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_free(JNIEnv* env, jclass cls, jlong handle) {
    coolbox_fractal_free((CoolBoxFractal*)handle);
}

// FunctionPlot JNI
JNIEXPORT jlong JNICALL Java_coolbox_graphics_FunctionPlot_create(JNIEnv* env, jclass cls, jint width, jint height) {
    return (jlong)coolbox_function_plot_create(width, height);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setEquation(JNIEnv* env, jclass cls, jlong handle, jstring expr) {
    const char* cexpr = (*env)->GetStringUTFChars(env, expr, 0);
    coolbox_function_plot_set_equation((CoolBoxFunctionPlot*)handle, cexpr);
    (*env)->ReleaseStringUTFChars(env, expr, cexpr);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setRange(JNIEnv* env, jclass cls, jlong handle, jdouble xMin, jdouble xMax) {
    coolbox_function_plot_set_range((CoolBoxFunctionPlot*)handle, xMin, xMax);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setSamples(JNIEnv* env, jclass cls, jlong handle, jint n) {
    coolbox_function_plot_set_samples((CoolBoxFunctionPlot*)handle, n);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setColor(JNIEnv* env, jclass cls, jlong handle, jint r, jint g, jint b, jint a) {
    CoolBoxColor* c = coolbox_color_create(r, g, b, a);
    coolbox_function_plot_set_color((CoolBoxFunctionPlot*)handle, c);
    coolbox_color_free(c);
}
JNIEXPORT jlong JNICALL Java_coolbox_graphics_FunctionPlot_render(JNIEnv* env, jclass cls, jlong handle) {
    return (jlong)coolbox_function_plot_render((CoolBoxFunctionPlot*)handle);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_free(JNIEnv* env, jclass cls, jlong handle) {
    coolbox_function_plot_free((CoolBoxFunctionPlot*)handle);
}

// ParametricPlot JNI
JNIEXPORT jlong JNICALL Java_coolbox_graphics_ParametricPlot_create(JNIEnv* env, jclass cls, jint width, jint height) {
    return (jlong)coolbox_parametric_plot_create(width, height);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setEquations(JNIEnv* env, jclass cls, jlong handle, jstring xExpr, jstring yExpr) {
    const char* cx = (*env)->GetStringUTFChars(env, xExpr, 0);
    const char* cy = (*env)->GetStringUTFChars(env, yExpr, 0);
    coolbox_parametric_plot_set_equations((CoolBoxParametricPlot*)handle, cx, cy);
    (*env)->ReleaseStringUTFChars(env, xExpr, cx);
    (*env)->ReleaseStringUTFChars(env, yExpr, cy);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setTRange(JNIEnv* env, jclass cls, jlong handle, jdouble tMin, jdouble tMax) {
    coolbox_parametric_plot_set_t_range((CoolBoxParametricPlot*)handle, tMin, tMax);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setSamples(JNIEnv* env, jclass cls, jlong handle, jint n) {
    coolbox_parametric_plot_set_samples((CoolBoxParametricPlot*)handle, n);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setColor(JNIEnv* env, jclass cls, jlong handle, jint r, jint g, jint b, jint a) {
    CoolBoxColor* c = coolbox_color_create(r, g, b, a);
    coolbox_parametric_plot_set_color((CoolBoxParametricPlot*)handle, c);
    coolbox_color_free(c);
}
JNIEXPORT jlong JNICALL Java_coolbox_graphics_ParametricPlot_render(JNIEnv* env, jclass cls, jlong handle) {
    return (jlong)coolbox_parametric_plot_render((CoolBoxParametricPlot*)handle);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_free(JNIEnv* env, jclass cls, jlong handle) {
    coolbox_parametric_plot_free((CoolBoxParametricPlot*)handle);
}

// PolarPlot JNI
JNIEXPORT jlong JNICALL Java_coolbox_graphics_PolarPlot_create(JNIEnv* env, jclass cls, jint width, jint height) {
    return (jlong)coolbox_polar_plot_create(width, height);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setEquation(JNIEnv* env, jclass cls, jlong handle, jstring expr) {
    const char* cexpr = (*env)->GetStringUTFChars(env, expr, 0);
    coolbox_polar_plot_set_equation((CoolBoxPolarPlot*)handle, cexpr);
    (*env)->ReleaseStringUTFChars(env, expr, cexpr);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setThetaRange(JNIEnv* env, jclass cls, jlong handle, jdouble thetaMin, jdouble thetaMax) {
    coolbox_polar_plot_set_theta_range((CoolBoxPolarPlot*)handle, thetaMin, thetaMax);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setSamples(JNIEnv* env, jclass cls, jlong handle, jint n) {
    coolbox_polar_plot_set_samples((CoolBoxPolarPlot*)handle, n);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setColor(JNIEnv* env, jclass cls, jlong handle, jint r, jint g, jint b, jint a) {
    CoolBoxColor* c = coolbox_color_create(r, g, b, a);
    coolbox_polar_plot_set_color((CoolBoxPolarPlot*)handle, c);
    coolbox_color_free(c);
}
JNIEXPORT jlong JNICALL Java_coolbox_graphics_PolarPlot_render(JNIEnv* env, jclass cls, jlong handle) {
    return (jlong)coolbox_polar_plot_render((CoolBoxPolarPlot*)handle);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_free(JNIEnv* env, jclass cls, jlong handle) {
    coolbox_polar_plot_free((CoolBoxPolarPlot*)handle);
}

// HistogramPlot JNI
JNIEXPORT jlong JNICALL Java_coolbox_graphics_HistogramPlot_create(JNIEnv* env, jclass cls, jint width, jint height) {
    return (jlong)coolbox_histogram_plot_create(width, height);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_setData(JNIEnv* env, jclass cls, jlong handle, jdoubleArray values) {
    jsize n = (*env)->GetArrayLength(env, values);
    jdouble* arr = (*env)->GetDoubleArrayElements(env, values, 0);
    coolbox_histogram_plot_set_data((CoolBoxHistogramPlot*)handle, arr, n);
    (*env)->ReleaseDoubleArrayElements(env, values, arr, 0);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_setBins(JNIEnv* env, jclass cls, jlong handle, jint n) {
    coolbox_histogram_plot_set_bins((CoolBoxHistogramPlot*)handle, n);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_setColor(JNIEnv* env, jclass cls, jlong handle, jint r, jint g, jint b, jint a) {
    CoolBoxColor* c = coolbox_color_create(r, g, b, a);
    coolbox_histogram_plot_set_color((CoolBoxHistogramPlot*)handle, c);
    coolbox_color_free(c);
}
JNIEXPORT jlong JNICALL Java_coolbox_graphics_HistogramPlot_render(JNIEnv* env, jclass cls, jlong handle) {
    return (jlong)coolbox_histogram_plot_render((CoolBoxHistogramPlot*)handle);
}
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_free(JNIEnv* env, jclass cls, jlong handle) {
    coolbox_histogram_plot_free((CoolBoxHistogramPlot*)handle);
}
#include "GuiComponentsBridge.h"
#include "../../go_bindings/bridge.h"
#include <stdlib.h>

JNIEXPORT jlong JNICALL Java_coolbox_gui_Toolbar_create(JNIEnv* env, jclass cls, jobjectArray actions) {
    jsize n = (*env)->GetArrayLength(env, actions);
    const char** cActions = (const char**)malloc(n * sizeof(char*));
    for (jsize i = 0; i < n; ++i) {
        jstring str = (jstring)(*env)->GetObjectArrayElement(env, actions, i);
        const char* cstr = (*env)->GetStringUTFChars(env, str, 0);
        cActions[i] = strdup(cstr);
        (*env)->ReleaseStringUTFChars(env, str, cstr);
    }
    jlong handle = (jlong)coolbox_toolbar_create(cActions, n);
    for (jsize i = 0; i < n; ++i) free((void*)cActions[i]);
    free(cActions);
    return handle;
}
JNIEXPORT void JNICALL Java_coolbox_gui_Toolbar_free(JNIEnv* env, jclass cls, jlong handle) {
    coolbox_toolbar_free((CoolBoxToolbar*)handle);
}
// Repeat similar JNI wrappers for DockPanel, LayerList, PropertyInspector, FileTree, RadioSelector, CheckboxGroup...
