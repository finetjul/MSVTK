set(KIT FFS)
project(msv${KIT})
# --------------------------------------------------------------------------

# Find IBTK and add config and use files
# --------------------------------------------------------------------------
include_directories(${IBAMR_INCLUDE_DIRS})

# Sources
# --------------------------------------------------------------------------

set(msv${KIT}_SRCS
  msvQFFSMainWindow.cxx
  msvQFFSAboutDialog.cxx
  msvFluidSimulator.cxx
  )

set(msv${KIT}_MOC_SRCS
  msvQFFSMainWindow.h
  msvQFFSAboutDialog.h
  )

set(msv${KIT}_UI_SRCS
  Resources/UI/msvQFFSMainWindow.ui
  Resources/UI/msvQFFSAboutDialog.ui
  )

set(msv${KIT}_QRC
  Resources/msvQFFS.qrc
  )

QT4_WRAP_CPP(msv${KIT}_SRCS ${msv${KIT}_MOC_SRCS})
QT4_WRAP_UI(msv${KIT}_UI_CXX ${msv${KIT}_UI_SRCS})
QT4_ADD_RESOURCES(msv${KIT}_QRC_SRCS ${msv${KIT}_QRC})

# --------------------------------------------------------------------------
# Include dirs
# --------------------------------------------------------------------------
set(include_dirs
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MSVTK_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIRS}
  ${CTK_INCLUDE_DIRS}
  ${msvVTKParallel_INCLUDE_DIRS}
  ${msvVTKWidgets_INCLUDE_DIRS}
  ${msvVTKFiltering_INCLUDE_DIRS}
  ${msvQtWidgets_INCLUDE_DIRS}
  ${msvIBTK_INCLUDE_DIRS}
  ${msvIBAMR_INCLUDE_DIRS}
  )

include_directories(${include_dirs})

# --------------------------------------------------------------------------
# Build the library so it can be tested in Testing/Cpp.
# --------------------------------------------------------------------------

add_library(msv${KIT}
  ${msv${KIT}_SRCS}
  ${msv${KIT}_UI_CXX}
  ${msv${KIT}_QRC_SRCS}
  )
set_target_properties(msv${KIT} PROPERTIES COMPILE_DEFINITIONS NDIM=3)

target_link_libraries(msv${KIT}
  ${MSVTK_LIBRARIES}
  ${CTK_LIBRARIES}
  ${VTK_LIBRARIES}
  ${IBAMR_LIBRARIES}
  ${IBAMR_EXTERNAL_LIBRARIES}
  msvVTKParallel
  msvVTKWidgets
  msvQtWidgets
  msvIBTK  
  msvIBAMR
  )

install(TARGETS msv${KIT}
  RUNTIME DESTINATION ${MSVTK_INSTALL_BIN_DIR} COMPONENT RuntimeLibraries
  LIBRARY DESTINATION ${MSVTK_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
  ARCHIVE DESTINATION ${MSVTK_INSTALL_LIB_DIR} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Build the executable
# --------------------------------------------------------------------------
add_executable(${KIT}
  ffs.cxx
  )
set_target_properties(${KIT} PROPERTIES OUTPUT_NAME ffs)

target_link_libraries(${KIT}
  msv${KIT}
  )

#-----------------------------------------------------------------------------
# Configure
# --------------------------------------------------------------------------
set(MY_LIBRARY_EXPORT_DIRECTIVE "MSV_${KIT}_EXPORT")
set(MY_EXPORT_HEADER_PREFIX MSV_${KIT})
set(MY_LIBNAME ${PROJECT_NAME})

configure_file(
  ${MSVTK_SOURCE_DIR}/CMake/msvExport.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Export.h
  )

set(dynamicHeaders
  "${dynamicHeaders};${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Export.h")

# --------------------------------------------------------------------------
# Testing
#
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()

# --------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------
if(NOT APPLE)
  set(${KIT}_INSTALL_DESTINATION_ARGS RUNTIME DESTINATION ${MSVTK_INSTALL_BIN_DIR})
else()
  set(${KIT}_INSTALL_DESTINATION_ARGS RUNTIME DESTINATION ".")
endif()

install(TARGETS ${KIT}
  ${${KIT}_INSTALL_DESTINATION_ARGS}
  COMPONENT Runtime)
