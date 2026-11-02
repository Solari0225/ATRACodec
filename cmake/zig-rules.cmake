# Applied after CMake's platform defaults, also in compiler try_compile checks.
foreach(_language C CXX)
    set(CMAKE_${_language}_ARCHIVE_CREATE "<CMAKE_AR> ar qc <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_${_language}_ARCHIVE_APPEND "<CMAKE_AR> ar q <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_${_language}_ARCHIVE_FINISH "<CMAKE_RANLIB> ranlib <TARGET>")
endforeach()
