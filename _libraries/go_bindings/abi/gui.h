#ifndef COOLBOX_GO_BINDINGS_ABI_GUI_H
#define COOLBOX_GO_BINDINGS_ABI_GUI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxComponent CoolBoxComponent;
typedef struct CoolBoxToolbar CoolBoxToolbar;
typedef struct CoolBoxDockPanel CoolBoxDockPanel;
typedef struct CoolBoxLayerList CoolBoxLayerList;
typedef struct CoolBoxPropertyInspector CoolBoxPropertyInspector;
typedef struct CoolBoxFileTree CoolBoxFileTree;
typedef struct CoolBoxRadioSelector CoolBoxRadioSelector;
typedef struct CoolBoxCheckboxGroup CoolBoxCheckboxGroup;

CoolBoxComponent* coolbox_component_create(int component_type);
void coolbox_component_free(CoolBoxComponent* c);

CoolBoxToolbar* coolbox_toolbar_create(const char** actions, int n);
void coolbox_toolbar_free(CoolBoxToolbar* t);

CoolBoxDockPanel* coolbox_dockpanel_create(const char* title, int floating);
void coolbox_dockpanel_free(CoolBoxDockPanel* d);

CoolBoxLayerList* coolbox_layerlist_create(const char** layers, int n, int selected);
void coolbox_layerlist_free(CoolBoxLayerList* l);

CoolBoxPropertyInspector* coolbox_propertyinspector_create(const char** keys, const char** values, int n);
void coolbox_propertyinspector_free(CoolBoxPropertyInspector* p);

CoolBoxFileTree* coolbox_filetree_create(const char* root_name);
void coolbox_filetree_free(CoolBoxFileTree* f);

CoolBoxRadioSelector* coolbox_radioselector_create(const char** options, int n, int selected);
void coolbox_radioselector_free(CoolBoxRadioSelector* r);

CoolBoxCheckboxGroup* coolbox_checkboxgroup_create(const char** options, const int* checked, int n);
void coolbox_checkboxgroup_free(CoolBoxCheckboxGroup* c);

#ifdef __cplusplus
}
#endif

#endif
