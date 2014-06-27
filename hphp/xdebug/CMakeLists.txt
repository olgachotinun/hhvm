project(xdebug)

#set(CXX_SOURCES)
#auto_sources(files "*.cpp" "")
#list(APPEND CXX_SOURCES ${files})



set(CXX_SOURCES)
add_definitions("-DHHVM_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/../hhvm/hhvm\"")

set(RECURSIVE_SOURCE_SUBDIRS ext)
foreach (dir ${RECURSIVE_SOURCE_SUBDIRS})
  auto_sources(files "*.cpp" "RECURSE" "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
  list(APPEND CXX_SOURCES ${files})
endforeach(dir ${RECURSIVE_SOURCE_SUBDIRS})

list(APPEND CXX_SOURCES "main.cpp")
#auto_sources(files "*.cpp" "")
#list(APPEND CXX_SOURCES ${files})

list(APPEND CXX_SOURCES ${files})
list(APPEND CXX_SOURCES
"${CMAKE_CURRENT_SOURCE_DIR}/../hhvm/process-init.cpp")
list(APPEND CXX_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/../hhvm/global-variables.cpp")

if(NOT HAVE_CUSTOM_LIBEVENT)
  list(REMOVE_ITEM CXX_SOURCES
       "${CMAKE_CURRENT_SOURCE_DIR}/ext/test_ext_curl.cpp")
endif()

if (NOT ENABLE_FASTCGI)
  list(REMOVE_ITEM CXX_SOURCES
       "${CMAKE_CURRENT_SOURCE_DIR}/ext/test_fastcgi.cpp")
  list(REMOVE_ITEM CXX_SOURCES
       "${CMAKE_CURRENT_SOURCE_DIR}/ext/test_fastcgi_protocol.cpp")
endif()



add_executable(${PROJECT_NAME} ${CXX_SOURCES})

target_link_libraries(${PROJECT_NAME} ${HHVM_LINK_LIBRARIES})
embed_all_systemlibs(${PROJECT_NAME} "${CMAKE_CURRENT_BINARY_DIR}/.."
"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}")
add_dependencies(${PROJECT_NAME} systemlib)
if (ENABLE_COTIRE)
  cotire(${PROJECT_NAME})
endif()

HHVM_INSTALL(${PROJECT_NAME} bin)

auto_sources(files "*.h" "")
HHVM_PUBLIC_HEADERS(${PROJECT_NAME} ${files})
