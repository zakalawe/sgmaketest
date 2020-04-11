
file(WRITE ${PROJECT_BINARY_DIR}/simgear/version.h "#define SIMGEAR_VERSION ${SIMGEAR_VERSION}")

foreach( mylibfolder
        bucket
        debug
        misc
        threads
        timing
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )


set(HEADERS compiler.h constants.h sg_inlines.h ${PROJECT_BINARY_DIR}/simgear/version.h)
install (FILES ${HEADERS}  DESTINATION include/simgear/)

get_property(coreSources GLOBAL PROPERTY CORE_SOURCES)
get_property(sceneSources GLOBAL PROPERTY SCENE_SOURCES)
get_property(publicHeaders GLOBAL PROPERTY PUBLIC_HEADERS)
get_property(localExpatSources GLOBAL PROPERTY LOCAL_EXPAT_SOURCES)

    message(STATUS "Library building mode: SHARED LIBRARIES")
    add_library(SimGearCore SHARED ${coreSources} ${localExpatSources})

    set_property(TARGET SimGearCore PROPERTY LINKER_LANGUAGE CXX)
    set_property(TARGET SimGearCore PROPERTY VERSION   ${SIMGEAR_VERSION})
    set_property(TARGET SimGearCore PROPERTY SOVERSION ${SIMGEAR_SOVERSION})

target_include_directories(SimGearCore BEFORE PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

# so simgear/simgear_config.h is found
target_include_directories(SimGearCore BEFORE PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)

target_include_directories(SimGearCore PUBLIC
  ${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR})
target_include_directories(SimGearCore PRIVATE
  ${EXPAT_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})

if (NOT SYSTEM_EXPAT)
    # XML_STATIC is important to avoid sg_expat_external.h
    # declaring symbols as declspec(import)
    target_compile_definitions(SimGearCore PRIVATE HAVE_SIMGEAR_EXPAT_CONFIG_H XML_STATIC)
endif()

    install(TARGETS SimGearCore
            EXPORT SimGearTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (NOT SIMGEAR_HEADLESS)
  install(TARGETS SimGearScene
          EXPORT SimGearTargets
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  target_include_directories(SimGearScene BEFORE PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

    target_include_directories(SimGearScene PUBLIC ${OPENSCENEGRAPH_INCLUDE_DIRS})

    if (USE_AEONWAVE)
      target_include_directories(SimGearScene PRIVATE ${AAX_INCLUDE_DIR} )
    else()
      target_include_directories(SimGearScene PRIVATE ${OPENAL_INCLUDE_DIR} )
    endif()
endif()

# we expose ZLib in some of our headers
target_link_libraries(SimGearCore PUBLIC ${ZLIB_LIBRARY})

target_link_libraries(SimGearCore PRIVATE
    ${RT_LIBRARY}
    ${DL_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${COCOA_LIBRARY}
    ${CURL_LIBRARIES}
    ${WINSOCK_LIBRARY})

if(SYSTEM_EXPAT)
    target_link_libraries(SimGearCore PRIVATE ${EXPAT_LIBRARIES})
endif()

if(ENABLE_DNS AND SYSTEM_UDNS)
    target_link_libraries(SimGearCore PRIVATE ${UDNS_LIBRARIES})
endif()
