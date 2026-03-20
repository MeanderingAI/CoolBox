#ifndef GUI_COMPONENTS_BRIDGE_H
#define GUI_COMPONENTS_BRIDGE_H

#include <jni.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

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
