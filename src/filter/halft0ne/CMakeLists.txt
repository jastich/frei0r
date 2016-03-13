set (SOURCES halft0ne.c)
set (TARGET halft0ne)

include_directories(${Cairo_INCLUDE_DIR})
set(LIBS ${LIBS} ${Cairo_LIBRARY})

if (MSVC)
  set_source_files_properties (halft0ne.c PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
