# Is include in the main cmakelists on install

get_filename_component(target_directory "${target_file}" DIRECTORY)

message(STATUS "Deploying Qt...")

get_filename_component(qt_bin_dir "${QMAKE_FILEPATH}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${qt_bin_dir}")

file(REMOVE
        "${target_directory}/libcrypto-1_1-x64.dll"
        "${target_directory}/libssl-1_1-x64.dll"
        "${target_directory}/libeay32.dll"
        "${target_directory}/ssleay32.dll"
        "${target_directory}/tls/qcertonlybackend.dll"
        "${target_directory}/tls/qopensslbackend.dll"
        "${target_directory}/tls/qschannelbackend.dll"
)

execute_process(
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        "${target_file}"
        --compiler-runtime
)
message(STATUS "Qt deployed.")
