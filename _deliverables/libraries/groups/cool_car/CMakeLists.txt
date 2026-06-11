
cmake_minimum_required(VERSION 3.16)

project(COOL_CAR VERSION 1.0 LANGUAGES C CXX)




add_subdirectory(DL)
add_subdirectory(DM)
add_subdirectory(FINANCE)
add_subdirectory(MATRIX)
add_subdirectory(ML)
add_subdirectory(OPT)
add_subdirectory(SP)
add_subdirectory(MATH)

if(TARGET DL)
	target_link_libraries(DL PUBLIC tyst_framework_main)
endif()
if(TARGET MATRIX)
	target_link_libraries(MATRIX PUBLIC tyst_framework_main)
endif()
if(TARGET ML)
	target_link_libraries(ML PUBLIC tyst_framework_main)
endif()
if(TARGET OPT)
	target_link_libraries(OPT PUBLIC tyst_framework_main)
endif()
if(TARGET SP)
	target_link_libraries(SP PUBLIC tyst_framework_main)
endif()
