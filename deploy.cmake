# Is include in the main cmakelists on install

get_filename_component(target_directory "${target_file}" DIRECTORY)

message(STATUS "Deploying Qt...")

get_filename_component(qt_bin_dir "${QMAKE_FILEPATH}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${qt_bin_dir}")
if (NOT WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "windeployqt was not found next to qmake: ${qt_bin_dir}")
endif ()

execute_process(
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        "${target_file}"
        --compiler-runtime
        --no-translations
        RESULT_VARIABLE deploy_result
        OUTPUT_VARIABLE deploy_output
        ERROR_VARIABLE deploy_error
)
if (NOT deploy_result EQUAL 0)
    message(FATAL_ERROR "windeployqt failed (${deploy_result}): ${deploy_error}")
endif ()
message(STATUS "${deploy_output}")
message(STATUS "Qt deployed.")
