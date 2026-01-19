# Event system library
add_library(EventSystem INTERFACE)

target_include_directories(EventSystem INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(EventSystem INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Event.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EventListener.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EventSource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SyncGlobals.h
)
