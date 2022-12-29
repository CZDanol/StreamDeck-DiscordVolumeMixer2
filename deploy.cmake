# Is include in the main cmakelists on install

get_filename_component(target_directory "${target_file}" DIRECTORY)

message(STATUS "Deploying Qt...")

get_filename_component(qt_bin_dir "${QMAKE_FILEPATH}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${qt_bin_dir}")

execute_process(
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        "${target_file}"
        --compiler-runtime
)
message(STATUS "Qt deployed.")
