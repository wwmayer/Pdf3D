
project(Pdf3d)

# determine compiler name
set(COMPILER_LABEL "unknown")
if(CMAKE_COMPILER_IS_GNUCC)
  set(COMPILER_LABEL "gcc")
endif(CMAKE_COMPILER_IS_GNUCC)
if(MSVC)
  set(COMPILER_LABEL "vc")
endif(MSVC)

# information about libharu
set(LIBHPDF_MAJOR 2)
set(LIBHPDF_MINOR 2)
set(LIBHPDF_PATCH 0)
set(LIBHPDF_VERSION ${LIBHPDF_MAJOR}.${LIBHPDF_MINOR}.${LIBHPDF_PATCH})

# we want cmake version 2.4.8 at least
cmake_minimum_required(VERSION 2.4.8 FATAL_ERROR)

# Location where the haru cmake build system first looks for cmake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/libharu/cmake/modules)

if(MSVC)
 set(CMAKE_DEBUG_POSTFIX "d")
endif(MSVC)


# =======================================================================
# look for headers and libraries
# =======================================================================

# check zlib availibility
find_package(ZLIB)
if(ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIR})
  set(ADDITIONAL_LIBRARIES ${ZLIB_LIBRARIES})
endif(ZLIB_FOUND)
  
# check png availibility
find_package(PNG)
if(PNG_FOUND)
  include_directories(${PNG_INCLUDE_DIR})
  add_definitions(${PNG_DEFINITIONS})
  set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${PNG_LIBRARIES})
endif(PNG_FOUND)


# =======================================================================
# configure header files, add compiler flags
# =======================================================================
# add definitions and directories to include
#if(CMAKE_COMPILER_IS_GNUCC)
#  add_definitions("-Wall")
#endif(CMAKE_COMPILER_IS_GNUCC)
if(MSVC_VERSION GREATER 1399)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC_VERSION GREATER 1399)
include_directories(${CMAKE_SOURCE_DIR}/libharu/include)


# these are options
option (LIBHPDF_DEBUG "Enable HPDF Debug")
option (LIBHPDF_DEBUG_TRACE "Enable HPDF Debug trace")

# Just set to 1, we'll assume they are always available.
# If not, then someone will have to add some tests in here to correctly determine
# the headers existance.
set (LIBHPDF_STDC_HEADERS 1)

# support all of the different variations of LIBPNG defines in HARU
set (LIBHPDF_HAVE_LIBPNG ${PNG_FOUND})
if (NOT PNG_FOUND)
   set (LIBHPDF_HAVE_NOPNGLIB 1)
   set (HPDF_NOPNGLIB 1)
endif (NOT PNG_FOUND)

# support different zlib defines
set (LIBHPDF_HAVE_LIBZ ${ZLIB_FOUND})
if (NOT ZLIB_FOUND)
   set (LIBHPDF_HAVE_NOZLIB 1)
endif (NOT ZLIB_FOUND)


# create hpdf_config.h
configure_file(
  ${CMAKE_SOURCE_DIR}/libharu/include/hpdf_config.h.cmake
  ${CMAKE_BINARY_DIR}/include/hpdf_config.h
)
include_directories(${CMAKE_BINARY_DIR}/include)

# =======================================================================
# libharu source files
# =======================================================================
set(
  LIBHPDF_SRCS
    libharu/src/hpdf_annotation.c
    libharu/src/hpdf_array.c
    libharu/src/hpdf_binary.c
    libharu/src/hpdf_boolean.c
    libharu/src/hpdf_catalog.c
    libharu/src/hpdf_destination.c
    libharu/src/hpdf_dict.c
    libharu/src/hpdf_doc_png.c
    libharu/src/hpdf_doc.c
    libharu/src/hpdf_encoder_cns.c
    libharu/src/hpdf_encoder_cnt.c
    libharu/src/hpdf_encoder_jp.c
    libharu/src/hpdf_encoder_kr.c
    libharu/src/hpdf_encoder.c
    libharu/src/hpdf_encrypt.c
    libharu/src/hpdf_encryptdict.c
    libharu/src/hpdf_error.c
    libharu/src/hpdf_ext_gstate.c
    libharu/src/hpdf_font_cid.c
    libharu/src/hpdf_font_tt.c
    libharu/src/hpdf_font_type1.c
    libharu/src/hpdf_font.c
    libharu/src/hpdf_fontdef_base14.c
    libharu/src/hpdf_fontdef_cid.c
    libharu/src/hpdf_fontdef_cns.c
    libharu/src/hpdf_fontdef_cnt.c
    libharu/src/hpdf_fontdef_jp.c
    libharu/src/hpdf_fontdef_kr.c
    libharu/src/hpdf_fontdef_tt.c
    libharu/src/hpdf_fontdef_type1.c
    libharu/src/hpdf_fontdef.c
    libharu/src/hpdf_gstate.c
    libharu/src/hpdf_image_ccitt.c
    libharu/src/hpdf_image_png.c
    libharu/src/hpdf_image.c
    libharu/src/hpdf_info.c
    libharu/src/hpdf_list.c
    libharu/src/hpdf_mmgr.c
    libharu/src/hpdf_name.c
    libharu/src/hpdf_namedict.c
    libharu/src/hpdf_null.c
    libharu/src/hpdf_number.c
    libharu/src/hpdf_objects.c
    libharu/src/hpdf_outline.c
    libharu/src/hpdf_page_label.c
    libharu/src/hpdf_page_operator.c
    libharu/src/hpdf_pages.c
    libharu/src/hpdf_real.c
    libharu/src/hpdf_streams.c
    libharu/src/hpdf_string.c
    libharu/src/hpdf_u3d.c
    libharu/src/hpdf_utils.c
    libharu/src/hpdf_xref.c
    libharu/src/hpdf_pdfa.c
    libharu/src/hpdf_3dmeasure.c
    libharu/src/hpdf_exdata.c
    libharu/src/hpdf_encoder_utf.c
)

# =====================================================

set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/libPRC/CMakeModules ${CMAKE_MODULE_PATH} )
include( CMakeMacros )

add_definitions( -DPRC_USE_ASYMPTOTE )


set (
  LIBPRC_SRCS
    libPRC/src/libPRC/Export.h
    libPRC/src/libPRC/libPRC.cpp
    libPRC/src/libPRC/libPRC.h
    libPRC/src/libPRC/PRCFile.cpp
    libPRC/src/libPRC/PRCFile.h
    libPRC/src/libPRC/PRCNode.cpp
    libPRC/src/libPRC/PRCNode.h
)

set (
  ASYMPTOTE_SRCS
    libPRC/src/asymptote/PRC.h
    libPRC/src/asymptote/PRCbitStream.cc
    libPRC/src/asymptote/PRCbitStream.h
    libPRC/src/asymptote/PRCdouble.cc
    libPRC/src/asymptote/PRCdouble.h
    libPRC/src/asymptote/oPRCFile.cc
    libPRC/src/asymptote/oPRCFile.h
    libPRC/src/asymptote/writePRC.cc
    libPRC/src/asymptote/writePRC.h
)

# =====================================================

set (
  MSHTOPDF_SRCS
    ${LIBHPDF_SRCS}
    ${LIBPRC_SRCS}
    ${ASYMPTOTE_SRCS}
    mshtoprc/mshtoprc.cpp
    mshtoprc/Stream.cpp
    mshtoprc/Swap.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/libPRC/src/asymptote)
include_directories(${CMAKE_SOURCE_DIR}/libPRC/src/libPRC)

add_executable(mshtopdf WIN32 ${MSHTOPDF_SRCS})
target_link_libraries(mshtopdf ${ADDITIONAL_LIBRARIES})
