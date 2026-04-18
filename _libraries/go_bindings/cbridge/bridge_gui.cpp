#include "../abi/gui.h"

#include <string>
#include <utility>
#include <vector>

struct CoolBoxComponent {
    int component_type = 0;
};

struct CoolBoxToolbar {
    std::vector<std::string> actions;
};

struct CoolBoxDockPanel {
    std::string title;
    bool floating = false;
};

struct CoolBoxLayerList {
    std::vector<std::string> layers;
    int selected = 0;
};

struct CoolBoxPropertyInspector {
    std::vector<std::pair<std::string, std::string>> properties;
};

struct CoolBoxFileTree {
    std::string root_name;
};

struct CoolBoxRadioSelector {
    std::vector<std::string> options;
    int selected = 0;
};

struct CoolBoxCheckboxGroup {
    std::vector<std::string> options;
    std::vector<int> checked;
};

namespace {

std::vector<std::string> strings_from_c_array(const char** values, int count) {
    if (count <= 0) {
        return {};
    }
    if (values == nullptr) {
        return {};
    }

    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        result.emplace_back(values[index] != nullptr ? values[index] : "");
    }
    return result;
}

}  // namespace

extern "C" {

CoolBoxComponent* coolbox_component_create(int component_type) {
    auto* component = new CoolBoxComponent();
    component->component_type = component_type;
    return component;
}

void coolbox_component_free(CoolBoxComponent* c) {
    delete c;
}

CoolBoxToolbar* coolbox_toolbar_create(const char** actions, int n) {
    auto* toolbar = new CoolBoxToolbar();
    toolbar->actions = strings_from_c_array(actions, n);
    return toolbar;
}

void coolbox_toolbar_free(CoolBoxToolbar* t) {
    delete t;
}

CoolBoxDockPanel* coolbox_dockpanel_create(const char* title, int floating) {
    auto* dock_panel = new CoolBoxDockPanel();
    dock_panel->title = title != nullptr ? title : "";
    dock_panel->floating = floating != 0;
    return dock_panel;
}

void coolbox_dockpanel_free(CoolBoxDockPanel* d) {
    delete d;
}

CoolBoxLayerList* coolbox_layerlist_create(const char** layers, int n, int selected) {
    auto* layer_list = new CoolBoxLayerList();
    layer_list->layers = strings_from_c_array(layers, n);
    layer_list->selected = selected;
    return layer_list;
}

void coolbox_layerlist_free(CoolBoxLayerList* l) {
    delete l;
}

CoolBoxPropertyInspector* coolbox_propertyinspector_create(const char** keys, const char** values, int n) {
    auto* inspector = new CoolBoxPropertyInspector();
    if (n > 0 && keys != nullptr && values != nullptr) {
        inspector->properties.reserve(static_cast<std::size_t>(n));
        for (int index = 0; index < n; ++index) {
            inspector->properties.emplace_back(
                keys[index] != nullptr ? keys[index] : "",
                values[index] != nullptr ? values[index] : "");
        }
    }
    return inspector;
}

void coolbox_propertyinspector_free(CoolBoxPropertyInspector* p) {
    delete p;
}

CoolBoxFileTree* coolbox_filetree_create(const char* root_name) {
    auto* file_tree = new CoolBoxFileTree();
    file_tree->root_name = root_name != nullptr ? root_name : "";
    return file_tree;
}

void coolbox_filetree_free(CoolBoxFileTree* f) {
    delete f;
}

CoolBoxRadioSelector* coolbox_radioselector_create(const char** options, int n, int selected) {
    auto* radio_selector = new CoolBoxRadioSelector();
    radio_selector->options = strings_from_c_array(options, n);
    radio_selector->selected = selected;
    return radio_selector;
}

void coolbox_radioselector_free(CoolBoxRadioSelector* r) {
    delete r;
}

CoolBoxCheckboxGroup* coolbox_checkboxgroup_create(const char** options, const int* checked, int n) {
    auto* checkbox_group = new CoolBoxCheckboxGroup();
    checkbox_group->options = strings_from_c_array(options, n);
    if (n > 0 && checked != nullptr) {
        checkbox_group->checked.assign(checked, checked + n);
    }
    return checkbox_group;
}

void coolbox_checkboxgroup_free(CoolBoxCheckboxGroup* c) {
    delete c;
}

}  // extern "C"