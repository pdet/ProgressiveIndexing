if(NOT "${JEMALLOC_HOME}" STREQUAL "")
    file(TO_CMAKE_PATH "${JEMALLOC_HOME}" _native_path)
    list(APPEND _jemalloc_roots ${_native_path})
elseif(JEMALLOC_HOME)
    list(APPEND _jemalloc_roots ${JEMALLOC_HOME})
endif()

set(LIBJEMALLOC_NAMES jemalloc libjemalloc.so.1 libjemalloc.so.2 libjemalloc.dylib)

# Try the parameterized roots, if they exist
if(_jemalloc_roots)
    find_path(JEMALLOC_INCLUDE_DIR
            NAMES jemalloc/jemalloc.h
            PATHS ${_jemalloc_roots}
            NO_DEFAULT_PATH
            PATH_SUFFIXES "include")
    find_library(JEMALLOC_SHARED_LIB
            NAMES ${LIBJEMALLOC_NAMES}
            PATHS ${_jemalloc_roots}
            NO_DEFAULT_PATH
            PATH_SUFFIXES "lib")
    find_library(JEMALLOC_STATIC_LIB
            NAMES jemalloc_pic
            PATHS ${_jemalloc_roots}
            NO_DEFAULT_PATH
            PATH_SUFFIXES "lib")
else()
    find_path(JEMALLOC_INCLUDE_DIR NAMES jemalloc/jemalloc.h)
    message(STATUS ${JEMALLOC_INCLUDE_DIR})
    find_library(JEMALLOC_SHARED_LIB NAMES ${LIBJEMALLOC_NAMES})
    message(STATUS ${JEMALLOC_SHARED_LIB})
    find_library(JEMALLOC_STATIC_LIB NAMES jemalloc_pic)
    message(STATUS ${JEMALLOC_STATIC_LIB})
endif()

if(JEMALLOC_INCLUDE_DIR AND JEMALLOC_SHARED_LIB)
    set(JEMALLOC_FOUND TRUE)
else()
    set(JEMALLOC_FOUND FALSE)
endif()

if(JEMALLOC_FOUND)
    if(NOT jemalloc_FIND_QUIETLY)
        message(STATUS "Found the jemalloc library: ${JEMALLOC_LIBRARIES}")
    endif()
else()
    if(NOT jemalloc_FIND_QUIETLY)
        set(JEMALLOC_ERR_MSG "Could not find the jemalloc library. Looked in ")
        if(_flatbuffers_roots)
            set(JEMALLOC_ERR_MSG "${JEMALLOC_ERR_MSG} in ${_jemalloc_roots}.")
        else()
            set(JEMALLOC_ERR_MSG "${JEMALLOC_ERR_MSG} system search paths.")
        endif()
        if(jemalloc_FIND_REQUIRED)
            message(FATAL_ERROR "${JEMALLOC_ERR_MSG}")
        else(jemalloc_FIND_REQUIRED)
            message(STATUS "${JEMALLOC_ERR_MSG}")
        endif(jemalloc_FIND_REQUIRED)
    endif()
endif()

mark_as_advanced(JEMALLOC_INCLUDE_DIR JEMALLOC_SHARED_LIB)