

# Organized library structure (valid subdirectories only)


add_subdirectory(IO/advanced_logging)
add_subdirectory(IO/data_structures)
add_subdirectory(IO/databases/sql)
add_subdirectory(IO/dataformats/json)
add_subdirectory(IO/distributed)
add_subdirectory(IO/file_system)
add_subdirectory(IO/servlets)
add_subdirectory(utils/system_stats)



add_subdirectory(security)

# Machine Learning _Libraries
add_subdirectory(ML)

## Removed explicit add_subdirectory calls for networking/http, networking/distributed, and data_structures (handled by GLOB/foreach above)
## add_subdirectory(dataformats/json)  # Directory does not exist
## add_subdirectory(databases/sql)     # Directory does not exist

# Services
## Removed non-existent services directory

## Removed explicit add_subdirectory calls for ml_server, security, auth, system, and file_system (handled by GLOB/foreach above)