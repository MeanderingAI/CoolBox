# CMakeLists.txt for DL/layers

# Add test for deep learning layers
enable_testing()
add_executable(test_deep_learning
	test/test_deep_learning.cpp
	src/ifinder.cpp
	src/layer.cpp
	src/tensor.cpp
	../loss/src/loss.cpp
	../optimizer/src/optimizer.cpp
	../wrapper/src/neural_network.cpp
	../wrapper/src/templates.cpp
)
target_include_directories(test_deep_learning PRIVATE
	include
	../loss/include
	../optimizer/include
	../wrapper/include
	${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/MATRIX/headers
	${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
)
find_package(Eigen3 QUIET)
if(TARGET Eigen3::Eigen)
	target_link_libraries(test_deep_learning PRIVATE Eigen3::Eigen)
endif()
target_link_libraries(test_deep_learning PRIVATE)
add_test(NAME DeepLearningLayerTests COMMAND test_deep_learning)

