set (SOURCES halft0ne.c)
set (TARGET halft0ne)

if (Cairo_FOUND)
  include_directories(${Cairo_INCLUDE_DIR})
  set(LIBS ${LIBS} ${Cairo_LIBRARY})
endif (Cairo_FOUND)

if (MSVC)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
target_link_libraries(halft0ne ${LIBS})

set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
