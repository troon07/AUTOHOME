cmake_minimum_required(VERSION 3.16)
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/../native/sdkconfig.defaults;${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
set(COMPONENTS main __pio_env esp-sr)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(clawgrid_speech)
# PlatformIO supplies one outer archive group. Nested flags exported by the
# upstream component are reordered by its CMake importer; remove those flags
# from target properties while retaining every upstream library and source.
idf_component_get_property(sr_library esp-sr COMPONENT_LIB)
foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
    get_target_property(sr_links ${sr_library} ${property})
    if(sr_links)
        list(REMOVE_ITEM sr_links "-Wl,--start-group" "-Wl,--end-group")
        set_property(TARGET ${sr_library} PROPERTY ${property} "${sr_links}")
    endif()
endforeach()
