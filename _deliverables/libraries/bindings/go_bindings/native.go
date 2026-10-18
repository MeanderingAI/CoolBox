package coolbox

/*
#cgo CFLAGS: -I${SRCDIR}/../c_bindings/include
#cgo CXXFLAGS: -std=c++17 -I${SRCDIR}/../../../../external/eigen/eigen-3.4.0 -I${SRCDIR}/../python_bindings/include/decision_tree -I${SRCDIR}/../python_bindings/include/nlp -I${SRCDIR}/../python_bindings/include/time_series -I${SRCDIR}/../python_bindings/include/multi_arm_bandit -I${SRCDIR}/../python_bindings/include -I${SRCDIR}/../python_bindings/vendor_include -I${SRCDIR}/../python_bindings/vendor_include/GRAPHICS/charts/headers -I${SRCDIR}/../../groups/cool_car -I${SRCDIR}/../../groups/cool_car/DL/layers/include -I${SRCDIR}/../../groups/cool_car/DL/loss/include -I${SRCDIR}/../../groups/cool_car/DL/optimizer/include -I${SRCDIR}/../../groups/cool_car/DL/wrapper/include -I${SRCDIR}/../../groups/cool_car/DL/agents/include -I${SRCDIR}/../../groups/cool_car/MATRIX/headers -I${SRCDIR}/../../groups/app_builder/GRAPHICS/charts/headers -I${SRCDIR}/../../groups/cool_car/ML/decision_tree/headers -I${SRCDIR}/../../groups/cool_car/ML/generalized_linear_model/headers -I${SRCDIR}/../../groups/trekker/DATASTRUCTURE/trees/headers -I${SRCDIR}/../../groups/trekker/DATASTRUCTURE/maps/headers -I${SRCDIR}/../../groups/trekker/DATASTRUCTURE/list/headers -I${SRCDIR}/../../groups/trekker/DATASTRUCTURE/filters/headers -I${SRCDIR}/../../groups/trekker/DATASTRUCTURE/xarray/headers
#cgo windows LDFLAGS: -L${SRCDIR}/../c_bindings/build -L${SRCDIR}/../c_bindings/build/Debug -L${SRCDIR}/../c_bindings/build/Release -L${SRCDIR}/../c_bindings/build/RelWithDebInfo -L${SRCDIR}/../c_bindings/build/MinSizeRel -lcoolbox_c_bindings
#cgo darwin LDFLAGS: -L${SRCDIR}/../c_bindings/build -Wl,-rpath,${SRCDIR}/../c_bindings/build -lcoolbox_c_bindings
#cgo linux LDFLAGS: -L${SRCDIR}/../c_bindings/build -Wl,-rpath,${SRCDIR}/../c_bindings/build -lcoolbox_c_bindings
*/
import "C"
