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
