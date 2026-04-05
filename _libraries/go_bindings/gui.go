package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/gui.h"
*/
import "C"

import (
	"runtime"
	"unsafe"
)

type Component struct{ handle *C.CoolBoxComponent }

func NewComponent(componentType int) *Component {
	h := C.coolbox_component_create(C.int(componentType))
	c := &Component{handle: h}
	runtime.SetFinalizer(c, func(c *Component) { C.coolbox_component_free(c.handle) })
	return c
}

type Toolbar struct{ handle *C.CoolBoxToolbar }

func NewToolbar(actions []string) *Toolbar {
	cActions := make([]*C.char, len(actions))
	for i, s := range actions {
		cActions[i] = C.CString(s)
	}
	defer func() {
		for _, s := range cActions {
			C.free(unsafe.Pointer(s))
		}
	}()
	if len(cActions) == 0 {
		return nil
	}
	h := C.coolbox_toolbar_create(&cActions[0], C.int(len(actions)))
	t := &Toolbar{handle: h}
	runtime.SetFinalizer(t, func(t *Toolbar) { C.coolbox_toolbar_free(t.handle) })
	return t
}

type DockPanel struct{ handle *C.CoolBoxDockPanel }

func NewDockPanel(title string, floating bool) *DockPanel {
	ctitle := C.CString(title)
	defer C.free(unsafe.Pointer(ctitle))
	h := C.coolbox_dockpanel_create(ctitle, C.int(boolToInt(floating)))
	d := &DockPanel{handle: h}
	runtime.SetFinalizer(d, func(d *DockPanel) { C.coolbox_dockpanel_free(d.handle) })
	return d
}

type LayerList struct{ handle *C.CoolBoxLayerList }

func NewLayerList(layers []string, selected int) *LayerList {
	cLayers := make([]*C.char, len(layers))
	for i, s := range layers {
		cLayers[i] = C.CString(s)
	}
	defer func() {
		for _, s := range cLayers {
			C.free(unsafe.Pointer(s))
		}
	}()
	if len(cLayers) == 0 {
		return nil
	}
	h := C.coolbox_layerlist_create(&cLayers[0], C.int(len(layers)), C.int(selected))
	l := &LayerList{handle: h}
	runtime.SetFinalizer(l, func(l *LayerList) { C.coolbox_layerlist_free(l.handle) })
	return l
}

type PropertyInspector struct{ handle *C.CoolBoxPropertyInspector }

func NewPropertyInspector(keys, values []string) *PropertyInspector {
	n := len(keys)
	if n != len(values) || n == 0 {
		return nil
	}
	cKeys := make([]*C.char, n)
	cVals := make([]*C.char, n)
	for i := 0; i < n; i++ {
		cKeys[i] = C.CString(keys[i])
		cVals[i] = C.CString(values[i])
	}
	defer func() {
		for _, s := range cKeys {
			C.free(unsafe.Pointer(s))
		}
		for _, s := range cVals {
			C.free(unsafe.Pointer(s))
		}
	}()
	h := C.coolbox_propertyinspector_create(&cKeys[0], &cVals[0], C.int(n))
	p := &PropertyInspector{handle: h}
	runtime.SetFinalizer(p, func(p *PropertyInspector) { C.coolbox_propertyinspector_free(p.handle) })
	return p
}

type FileTree struct{ handle *C.CoolBoxFileTree }

func NewFileTree(rootName string) *FileTree {
	croot := C.CString(rootName)
	defer C.free(unsafe.Pointer(croot))
	h := C.coolbox_filetree_create(croot)
	f := &FileTree{handle: h}
	runtime.SetFinalizer(f, func(f *FileTree) { C.coolbox_filetree_free(f.handle) })
	return f
}

type RadioSelector struct{ handle *C.CoolBoxRadioSelector }

func NewRadioSelector(options []string, selected int) *RadioSelector {
	cOpts := make([]*C.char, len(options))
	for i, s := range options {
		cOpts[i] = C.CString(s)
	}
	defer func() {
		for _, s := range cOpts {
			C.free(unsafe.Pointer(s))
		}
	}()
	if len(cOpts) == 0 {
		return nil
	}
	h := C.coolbox_radioselector_create(&cOpts[0], C.int(len(options)), C.int(selected))
	r := &RadioSelector{handle: h}
	runtime.SetFinalizer(r, func(r *RadioSelector) { C.coolbox_radioselector_free(r.handle) })
	return r
}

type CheckboxGroup struct{ handle *C.CoolBoxCheckboxGroup }

func NewCheckboxGroup(options []string, checked []bool) *CheckboxGroup {
	n := len(options)
	if n == 0 || len(checked) != n {
		return nil
	}
	cOpts := make([]*C.char, n)
	cChecked := make([]C.int, n)
	for i, s := range options {
		cOpts[i] = C.CString(s)
	}
	for i, b := range checked {
		if b {
			cChecked[i] = 1
		}
	}
	defer func() {
		for _, s := range cOpts {
			C.free(unsafe.Pointer(s))
		}
	}()
	h := C.coolbox_checkboxgroup_create(&cOpts[0], &cChecked[0], C.int(n))
	c := &CheckboxGroup{handle: h}
	runtime.SetFinalizer(c, func(c *CheckboxGroup) { C.coolbox_checkboxgroup_free(c.handle) })
	return c
}
