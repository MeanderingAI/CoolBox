if(BUILD_TESTING)
	add_executable(math_tests
		tests/test_math.cpp
		${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
	)
	target_link_libraries(math_tests PRIVATE cool_car_math mytrix)
	target_include_directories(math_tests
		SYSTEM PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}
			../MATRIX/headers
			${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
	)
	add_test(NAME MathTests COMMAND math_tests)
endif()

add_library(cool_car_math INTERFACE)
target_include_directories(cool_car_math
	SYSTEM INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../MATRIX/headers>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers>
)
set_target_properties(cool_car_math PROPERTIES LINKER_LANGUAGE CXX)
