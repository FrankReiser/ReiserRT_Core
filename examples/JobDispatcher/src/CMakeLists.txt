add_executable( jobDispatcher "" )
target_sources( jobDispatcher PRIVATE main.cpp JobDispatcher.cpp JobTask.cpp JobData.cpp )
target_include_directories( jobDispatcher PUBLIC ../src )
target_link_libraries( jobDispatcher ReiserRT_Core )
