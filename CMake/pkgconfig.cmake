if (${LINUX_FLAVOR} MATCHES "^(Ubuntu)")
  set(XRM_PKG_CONFIG_DIR "/usr/lib/pkgconfig")
elseif (${LINUX_FLAVOR} MATCHES "^(RedHat|CentOS)")
  set(XRM_PKG_CONFIG_DIR "/usr/lib64/pkgconfig")
else ()
  set(XRM_PKG_CONFIG_DIR "/usr/share/pkgconfig")
endif ()

configure_file(
    ${CMAKE_SOURCE_DIR}/CMake/config/libxrm.pc.in
    ${CMAKE_SOURCE_DIR}/CMake/config/libxrm.pc @ONLY)
set(PKGCFG_FILE "CMake/config/libxrm.pc")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    install(FILES ${PKGCFG_FILE} DESTINATION ${XRM_PKG_CONFIG_DIR})
endif(CMAKE_BUILD_TYPE STREQUAL "Release")
