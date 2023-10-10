add_executable( jobDispatcher "" )
target_sources( jobDispatcher PRIVATE main.cpp JobDispatcher.cpp JobTask.cpp JobData.cpp JobDataPtrTypeFwd.cpp )
target_include_directories( jobDispatcher PUBLIC ../src )
target_link_libraries( jobDispatcher ReiserRT_Core )
target_compile_options( jobDispatcher PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

