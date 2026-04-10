#ifndef COOLBOX__LIBRARIES_GO_BINDINGS_BRIDGE_FORWARD_H
#define COOLBOX__LIBRARIES_GO_BINDINGS_BRIDGE_FORWARD_H

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for opaque types used by the bridge API.
// These mirror the opaque C-visible types expected by bridge.h

typedef struct CoolBoxCanvas CoolBoxCanvas;
typedef struct CoolBoxGraph CoolBoxGraph;
typedef struct CoolBoxTable CoolBoxTable;
typedef struct CoolBoxColor CoolBoxColor;
typedef struct CoolBoxFontFace CoolBoxFontFace;
typedef struct CoolBoxTextRenderer CoolBoxTextRenderer;
typedef struct CoolBoxComponent CoolBoxComponent;
typedef struct CoolBoxToolbar CoolBoxToolbar;
typedef struct CoolBoxDockPanel CoolBoxDockPanel;
typedef struct CoolBoxLayerList CoolBoxLayerList;
typedef struct CoolBoxPropertyInspector CoolBoxPropertyInspector;
typedef struct CoolBoxFileTree CoolBoxFileTree;
typedef struct CoolBoxRadioSelector CoolBoxRadioSelector;
typedef struct CoolBoxCheckboxGroup CoolBoxCheckboxGroup;

#ifdef __cplusplus
}
#endif
#endif  // COOLBOX__LIBRARIES_GO_BINDINGS_BRIDGE_FORWARD_H
