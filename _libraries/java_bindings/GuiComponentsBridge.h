#ifndef GUI_COMPONENTS_BRIDGE_H
#define GUI_COMPONENTS_BRIDGE_H

#include <jni.h>
#include <stdint.h>
// Fractal and Plotting Primitives
JNIEXPORT jlong JNICALL Java_coolbox_graphics_Fractal_create(JNIEnv*, jclass, jint width, jint height, jint type);
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_setParams(JNIEnv*, jclass, jlong handle, jdouble param1, jdouble param2);
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_setMaxIter(JNIEnv*, jclass, jlong handle, jint maxIter);
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_setBounds(JNIEnv*, jclass, jlong handle, jdouble xMin, jdouble xMax, jdouble yMin, jdouble yMax);
JNIEXPORT jlong JNICALL Java_coolbox_graphics_Fractal_render(JNIEnv*, jclass, jlong handle);
JNIEXPORT void JNICALL Java_coolbox_graphics_Fractal_free(JNIEnv*, jclass, jlong handle);

JNIEXPORT jlong JNICALL Java_coolbox_graphics_FunctionPlot_create(JNIEnv*, jclass, jint width, jint height);
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setEquation(JNIEnv*, jclass, jlong handle, jstring expr);
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setRange(JNIEnv*, jclass, jlong handle, jdouble xMin, jdouble xMax);
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setSamples(JNIEnv*, jclass, jlong handle, jint n);
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_setColor(JNIEnv*, jclass, jlong handle, jint r, jint g, jint b, jint a);
JNIEXPORT jlong JNICALL Java_coolbox_graphics_FunctionPlot_render(JNIEnv*, jclass, jlong handle);
JNIEXPORT void JNICALL Java_coolbox_graphics_FunctionPlot_free(JNIEnv*, jclass, jlong handle);

JNIEXPORT jlong JNICALL Java_coolbox_graphics_ParametricPlot_create(JNIEnv*, jclass, jint width, jint height);
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setEquations(JNIEnv*, jclass, jlong handle, jstring xExpr, jstring yExpr);
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setTRange(JNIEnv*, jclass, jlong handle, jdouble tMin, jdouble tMax);
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setSamples(JNIEnv*, jclass, jlong handle, jint n);
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_setColor(JNIEnv*, jclass, jlong handle, jint r, jint g, jint b, jint a);
JNIEXPORT jlong JNICALL Java_coolbox_graphics_ParametricPlot_render(JNIEnv*, jclass, jlong handle);
JNIEXPORT void JNICALL Java_coolbox_graphics_ParametricPlot_free(JNIEnv*, jclass, jlong handle);

JNIEXPORT jlong JNICALL Java_coolbox_graphics_PolarPlot_create(JNIEnv*, jclass, jint width, jint height);
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setEquation(JNIEnv*, jclass, jlong handle, jstring expr);
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setThetaRange(JNIEnv*, jclass, jlong handle, jdouble thetaMin, jdouble thetaMax);
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setSamples(JNIEnv*, jclass, jlong handle, jint n);
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_setColor(JNIEnv*, jclass, jlong handle, jint r, jint g, jint b, jint a);
JNIEXPORT jlong JNICALL Java_coolbox_graphics_PolarPlot_render(JNIEnv*, jclass, jlong handle);
JNIEXPORT void JNICALL Java_coolbox_graphics_PolarPlot_free(JNIEnv*, jclass, jlong handle);

JNIEXPORT jlong JNICALL Java_coolbox_graphics_HistogramPlot_create(JNIEnv*, jclass, jint width, jint height);
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_setData(JNIEnv*, jclass, jlong handle, jdoubleArray values);
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_setBins(JNIEnv*, jclass, jlong handle, jint n);
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_setColor(JNIEnv*, jclass, jlong handle, jint r, jint g, jint b, jint a);
JNIEXPORT jlong JNICALL Java_coolbox_graphics_HistogramPlot_render(JNIEnv*, jclass, jlong handle);
JNIEXPORT void JNICALL Java_coolbox_graphics_HistogramPlot_free(JNIEnv*, jclass, jlong handle);

// Toolbar
JNIEXPORT jlong JNICALL Java_coolbox_gui_Toolbar_create(JNIEnv*, jclass, jobjectArray actions);
JNIEXPORT void JNICALL Java_coolbox_gui_Toolbar_free(JNIEnv*, jclass, jlong handle);

// DockPanel
JNIEXPORT jlong JNICALL Java_coolbox_gui_DockPanel_create(JNIEnv*, jclass, jstring title, jboolean floating);
JNIEXPORT void JNICALL Java_coolbox_gui_DockPanel_free(JNIEnv*, jclass, jlong handle);

// LayerList
JNIEXPORT jlong JNICALL Java_coolbox_gui_LayerList_create(JNIEnv*, jclass, jobjectArray layers, jint selected);
JNIEXPORT void JNICALL Java_coolbox_gui_LayerList_free(JNIEnv*, jclass, jlong handle);

// PropertyInspector
JNIEXPORT jlong JNICALL Java_coolbox_gui_PropertyInspector_create(JNIEnv*, jclass, jobjectArray keys, jobjectArray values);
JNIEXPORT void JNICALL Java_coolbox_gui_PropertyInspector_free(JNIEnv*, jclass, jlong handle);

// FileTree
JNIEXPORT jlong JNICALL Java_coolbox_gui_FileTree_create(JNIEnv*, jclass, jstring rootName);
JNIEXPORT void JNICALL Java_coolbox_gui_FileTree_free(JNIEnv*, jclass, jlong handle);

// RadioSelector
JNIEXPORT jlong JNICALL Java_coolbox_gui_RadioSelector_create(JNIEnv*, jclass, jobjectArray options, jint selected);
JNIEXPORT void JNICALL Java_coolbox_gui_RadioSelector_free(JNIEnv*, jclass, jlong handle);

// CheckboxGroup
JNIEXPORT jlong JNICALL Java_coolbox_gui_CheckboxGroup_create(JNIEnv*, jclass, jobjectArray options, jbooleanArray checked);
JNIEXPORT void JNICALL Java_coolbox_gui_CheckboxGroup_free(JNIEnv*, jclass, jlong handle);

#ifdef __cplusplus
}
#endif

#endif // GUI_COMPONENTS_BRIDGE_H
