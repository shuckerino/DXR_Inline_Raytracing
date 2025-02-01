# Downloads a nuget package and makes it available through
# get_nuget_package(PACKAGE <PackageName>
#                   VERSION <PackageVersion)
#
#
function(get_nuget_package)
    # Nuget Server. 
    set(NUGET_SERVER "http://www.nuget.org/api/v2/package")
    # Where to store the download nuget-package archives
    set(NUGET_DOWNLOAD_CACHE ${CMAKE_CURRENT_BINARY_DIR}/nuget)
    

    
    # A function with name parameters
    # See: https://stackoverflow.com/questions/23327687/how-to-write-a-cmake-function-with-more-than-one-parameter-groups
    cmake_parse_arguments(
        NAMED_ARGS      # prefix of output variables
        ""              # list of names of the boolean arguments (only defined ones will be true)
        PACKAGE VERSION # list of names of mono-valued arguments
        ""              # list of names of multi-valued arguments (output variables are lists)
        ${ARGN}         # arguments of the function to parse, here we take the all original ones
    )
    # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
    if(NOT NAMED_ARGS_PACKAGE)
        message(FATAL_ERROR "get_nuget_package: Missing NAME of package!")
    endif(NOT NAMED_ARGS_PACKAGE)

    if(NOT NAMED_ARGS_VERSION)
        message(FATAL_ERROR "get_nuget_package: Missing VERSION of package!")
    endif(NOT NAMED_ARGS_VERSION)
    
    # Download and extract package, in case it is not yet there.
    set(DOWNLOADED_PACKAGE_DIRECTORY ${NUGET_DOWNLOAD_CACHE}/${NAMED_ARGS_PACKAGE}/${NAMED_ARGS_VERSION})
    set(DOWNLOADED_PACKAGE_ARCHIVE ${DOWNLOADED_PACKAGE_DIRECTORY}/${NAMED_ARGS_PACKAGE}_${NAMED_ARGS_VERSION}.zip)    
    if(NOT EXISTS ${DOWNLOADED_PACKAGE_ARCHIVE})
        set(DOWNLOAD_PACKAGE_URL ${NUGET_SERVER}/${NAMED_ARGS_PACKAGE}/${NAMED_ARGS_VERSION})
        message(STATUS "Download NuGet package ${NAMED_ARGS_PACKAGE}, Version ${NAMED_ARGS_VERSION} from ${DOWNLOAD_PACKAGE_URL} to ${DOWNLOADED_PACKAGE_ARCHIVE}")
        
        file(DOWNLOAD ${DOWNLOAD_PACKAGE_URL} ${DOWNLOADED_PACKAGE_ARCHIVE} STATUS DOWNLOAD_RESULT)

        list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
        if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
            message(FATAL_ERROR "Error downloading NuGet package: ${DOWNLOAD_RESULT}.")    
        endif()
        file(ARCHIVE_EXTRACT INPUT ${DOWNLOADED_PACKAGE_ARCHIVE} DESTINATION ${DOWNLOADED_PACKAGE_DIRECTORY})
    else()
        message(STATUS "Package  ${NAMED_ARGS_PACKAGE}, Version ${NAMED_ARGS_VERSION} already cached at ${DOWNLOADED_PACKAGE_ARCHIVE}")    
    endif()

    
    add_library(${NAMED_ARGS_PACKAGE} INTERFACE)
    target_include_directories(${NAMED_ARGS_PACKAGE} INTERFACE ${DOWNLOADED_PACKAGE_DIRECTORY}/build/native/include)

    # create a targe that copies the binary files
    set(BIN_DIRECTORY ${DOWNLOADED_PACKAGE_DIRECTORY}/build/native/bin/x64)

    file(GLOB BIN_FILES
        ${BIN_DIRECTORY}/*.exe
        ${BIN_DIRECTORY}/*.dll
        ${BIN_DIRECTORY}/*.pdb
    )

    #message(STATUS "NUGET RUNTIME DIRECTORY IS ${NUGET_RUNTIME_LIB_DIRECTORY}")
    
    #foreach(BIN_FILE ${BIN_FILES})
    #    message(status ${BIN_FILE})
    #    get_filename_component(BIN_FILE_NAME ${BIN_FILE} NAME)
    #    add_custom_command(
        #        OUTPUT ${NUGET_RUNTIME_LIB_DIRECTORY}/$<CONFIG>/${BIN_FILE_NAME}
        #        PRE_BUILD
        #        COMMAND ${CMAKE_COMMAND} -E make_directory ${NUGET_RUNTIME_LIB_DIRECTORY}/$<CONFIG>
        #        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BIN_FILE} ${NUGET_RUNTIME_LIB_DIRECTORY}/$<CONFIG>
        #        MAIN_DEPENDENCY  ${BIN_FILE}
        #    )
        #    list(APPEND COPY_FILES ${NUGET_RUNTIME_LIB}/$<CONFIG>/${BIN_FILE_NAME})
        #endforeach()
        
    set(NUGET_RUNTIME_LIB_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    foreach(BIN_FILE ${BIN_FILES})
        get_filename_component(BIN_FILE_NAME ${BIN_FILE} NAME)
        add_custom_command(
            OUTPUT ${NUGET_RUNTIME_LIB_DIRECTORY}/$<CONFIG>/${BIN_FILE_NAME}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${NUGET_RUNTIME_LIB_DIRECTORY}/$<CONFIG>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BIN_FILE} ${NUGET_RUNTIME_LIB_DIRECTORY}/$<CONFIG>
            MAIN_DEPENDENCY  ${BIN_FILE}
        )
        list(APPEND COPY_FILES ${NUGET_RUNTIME_LIB_DIRECTORY}/$<CONFIG>/${BIN_FILE_NAME})
    endforeach()
    
    
    add_custom_target(${NAMED_ARGS_PACKAGE}_copy DEPENDS "${COPY_FILES}")
    set_target_properties(${NAMED_ARGS_PACKAGE}_copy PROPERTIES FOLDER CopyTargets)

    add_dependencies(${NAMED_ARGS_PACKAGE} ${NAMED_ARGS_PACKAGE}_copy)
endfunction(get_nuget_package)