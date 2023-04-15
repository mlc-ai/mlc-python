if(APPLE)
  find_program(DSYMUTIL_PROGRAM dsymutil)
  mark_as_advanced(DSYMUTIL_PROGRAM)
endif()

function(add_debug_symbol_apple _target _directory)
  if (APPLE)
    add_custom_command(TARGET ${_target} POST_BUILD
      COMMAND ${DSYMUTIL_PROGRAM} ARGS $<TARGET_FILE:${_target}>
      COMMENT "Running dsymutil" VERBATIM
    )
    install(FILES $<TARGET_FILE:${_target}>.dSYM DESTINATION ${_directory})
  endif (APPLE)
endfunction()
