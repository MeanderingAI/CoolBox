# Services
add_subdirectory(cache_server)
add_subdirectory(dns)
add_subdirectory(proxy)
add_subdirectory(server_launch)
add_subdirectory(distributed_fs)
add_subdirectory(mail_server)

# Example: set properties for a service target
# Replace my_service with your actual service target name
#set_target_properties(my_service PROPERTIES
#    CXX_STANDARD 17
#    CXX_STANDARD_REQUIRED ON
#    OUTPUT_NAME "my_service_bin"
#)
