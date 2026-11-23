zephyr_library()
# ZMK keeps its behavior driver interface under app/include/drivers.
zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library_sources(src/behavior_persistent_os_toggle.c)
