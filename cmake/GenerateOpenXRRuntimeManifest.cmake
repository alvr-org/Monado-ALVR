# Copyright 2019-2023, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
#
# SPDX-License-Identifier: BSL-1.0
#
# Maintained by:
# 2019-2023 Rylie Pavlik <rylie.pavlik@collabora.com> <rylie@ryliepavlik.com>

#[[.rst:
GenerateOpenXRRuntimeManifest
---------------

The following functions are provided by this module:

- :command:`generate_openxr_runtime_manifest_buildtree`
- :command:`generate_openxr_runtime_manifest_at_install`


.. command:: generate_openxr_runtime_manifest_buildtree

  Generates a runtime manifest suitable for use in the build tree,
  with absolute paths, at build time::

    generate_openxr_runtime_manifest_buildtree(
        RUNTIME_TARGET <target>          # Name of your runtime target
        OUT_FILE <outfile>               # Name of the manifest file (with path) to generate
        [MANIFEST_TEMPLATE <template>]   # Optional: Specify an alternate template to use
        )


.. command:: generate_openxr_runtime_manifest_at_install

  Generates a runtime manifest at install time and installs it where desired.
  A configure-generated install script invokes the Python helper when
  ``cmake --install`` runs so absolute paths use the effective install prefix::

    generate_openxr_runtime_manifest_at_install(
        RUNTIME_TARGET <target>            # Name of your runtime target
        DESTINATION <dest>                 # The install-prefix-relative path to install the manifest to.
        RELATIVE_RUNTIME_DIR <dir>         # The install-prefix-relative path that the runtime library is installed to.
        [COMPONENT <comp>]                 # If present, the component to place the manifest in.
        [ABSOLUTE_RUNTIME_PATH|            # If present, path in generated manifest is absolute
         RUNTIME_DIR_RELATIVE_TO_MANIFEST <dir>]
                                           # If present (and ABSOLUTE_RUNTIME_PATH not present), specifies the
                                           # runtime directory relative to the manifest directory in the installed layout
        [OUT_FILENAME <outfilename>        # Optional: Alternate name of the manifest file to generate
        [MANIFEST_TEMPLATE <template>]     # Optional: Specify an alternate template to use
        [LIBMONADO <path>]                 # Optional: path to libmonado to include in manifest
        )
#]]

get_filename_component(_OXR_MANIFEST_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}"
                       PATH)
set(_OXR_MANIFEST_SCRIPT
    "${_OXR_MANIFEST_CMAKE_DIR}/generate_openxr_runtime_manifest.py"
    CACHE INTERNAL "" FORCE)
set(_OXR_MANIFEST_TEMPLATE
    "${_OXR_MANIFEST_CMAKE_DIR}/openxr_manifest.in.json"
    CACHE INTERNAL "" FORCE)
set(_OXR_MANIFEST_INSTALL_SCRIPT
    "${_OXR_MANIFEST_CMAKE_DIR}/GenerateOpenXRRuntimeManifest.cmake.in"
    CACHE INTERNAL "" FORCE)

function(generate_openxr_runtime_manifest_buildtree)
    set(options)
    set(oneValueArgs MANIFEST_TEMPLATE RUNTIME_TARGET OUT_FILE LIBMONADO)
    set(multiValueArgs)
    cmake_parse_arguments(_genmanifest "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})

    if(NOT _genmanifest_MANIFEST_TEMPLATE)
        set(_genmanifest_MANIFEST_TEMPLATE "${_OXR_MANIFEST_TEMPLATE}")
    endif()
    if(NOT _genmanifest_RUNTIME_TARGET)
        message(FATAL_ERROR "Need RUNTIME_TARGET specified!")
    endif()
    if(NOT _genmanifest_OUT_FILE)
        message(FATAL_ERROR "Need OUT_FILE specified!")
    endif()

    set(_genmanifest_cmd
        "${Python3_EXECUTABLE}"
        "${_OXR_MANIFEST_SCRIPT}"
        buildtree
        --out-file "${_genmanifest_OUT_FILE}"
        --target-path "$<TARGET_FILE:${_genmanifest_RUNTIME_TARGET}>"
        --template "${_genmanifest_MANIFEST_TEMPLATE}"
        )
    if(_genmanifest_LIBMONADO)
        list(APPEND _genmanifest_cmd
             --libmonado-path "$<TARGET_FILE:${_genmanifest_LIBMONADO}>")
    endif()
    if(WIN32)
        list(APPEND _genmanifest_cmd --win32)
    endif()

    add_custom_command(
        TARGET ${_genmanifest_RUNTIME_TARGET}
        POST_BUILD
        BYPRODUCTS "${_genmanifest_OUT_FILE}"
        COMMAND ${_genmanifest_cmd}
        COMMENT
            "Generating OpenXR runtime manifest named ${_genmanifest_OUT_FILE} for build tree usage"
        VERBATIM
        )
endfunction()

function(generate_openxr_runtime_manifest_at_install)
    set(options ABSOLUTE_RUNTIME_PATH)
    set(oneValueArgs
        MANIFEST_TEMPLATE
        DESTINATION
        OUT_FILENAME
        COMPONENT
        RUNTIME_TARGET
        RUNTIME_DIR_RELATIVE_TO_MANIFEST
        RELATIVE_RUNTIME_DIR
        LIBMONADO
    )
    set(multiValueArgs)
    cmake_parse_arguments(_genmanifest "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})

    if(NOT _genmanifest_MANIFEST_TEMPLATE)
        set(_genmanifest_MANIFEST_TEMPLATE "${_OXR_MANIFEST_TEMPLATE}")
    endif()
    if(NOT _genmanifest_RUNTIME_TARGET)
        message(FATAL_ERROR "Need RUNTIME_TARGET specified!")
    endif()
    if(NOT _genmanifest_DESTINATION)
        message(FATAL_ERROR "Need DESTINATION specified!")
    endif()
    if(NOT _genmanifest_RELATIVE_RUNTIME_DIR)
        message(FATAL_ERROR "Need RELATIVE_RUNTIME_DIR specified!")
    endif()
    if(NOT _genmanifest_OUT_FILENAME)
        set(_genmanifest_OUT_FILENAME "${_genmanifest_RUNTIME_TARGET}.json")
    endif()
    if(NOT _genmanifest_COMPONENT)
        set(_genmanifest_COMPONENT Unspecified)
    endif()

    set(_runtime_filename
        "${CMAKE_SHARED_MODULE_PREFIX}${_genmanifest_RUNTIME_TARGET}${CMAKE_SHARED_MODULE_SUFFIX}"
        )
    set(_libmonado_filename)
    if(_genmanifest_LIBMONADO)
        set(_libmonado_filename
            "${CMAKE_SHARED_MODULE_PREFIX}${_genmanifest_LIBMONADO}${CMAKE_SHARED_MODULE_SUFFIX}"
            )
    endif()

    set(_manifest_file
        "${CMAKE_CURRENT_BINARY_DIR}/${_genmanifest_OUT_FILENAME}")

    set(_oxr_path_type soname)
    if(_genmanifest_ABSOLUTE_RUNTIME_PATH)
        set(_oxr_path_type absolute)
    elseif(_genmanifest_RUNTIME_DIR_RELATIVE_TO_MANIFEST)
        set(_oxr_path_type relative)
    endif()

    set(_install_script
        "${CMAKE_CURRENT_BINARY_DIR}/install_manifest_${_genmanifest_RUNTIME_TARGET}.cmake"
        )
    configure_file(
        "${_OXR_MANIFEST_INSTALL_SCRIPT}"
        "${_install_script}"
        @ONLY
        )

    install(SCRIPT "${_install_script}" COMPONENT ${_genmanifest_COMPONENT})
endfunction()
