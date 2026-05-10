# CMakeLists.txt for DL/layers

# Add test for deep learning layers
enable_testing()
add_executable(test_deep_learning
	test/test_deep_learning.cpp
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
	${CMAKE_SOURCE_DIR}/_libraries/groups/cool_car/MATRIX/headers
	${CMAKE_SOURCE_DIR}/external/eigen/eigen-3.4.0
)
target_link_libraries(test_deep_learning PRIVATE tyst_framework_main)
add_test(NAME DeepLearningLayerTests COMMAND test_deep_learning)

