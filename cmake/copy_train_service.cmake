# Incremental sync of the train_json_service onedir build into the app output dir.
# robocopy exit codes 0-7 all mean success; only >= 8 is a failure.

if(NOT DEFINED SRC OR NOT DEFINED DST)
    message(FATAL_ERROR "SRC and DST must be defined")
endif()

if(EXISTS "${DST}/train_json_service.exe")
    file(TIMESTAMP "${SRC}/train_json_service.exe" _src_ts "%s")
    file(TIMESTAMP "${DST}/train_json_service.exe" _dst_ts "%s")
    if(_dst_ts GREATER_EQUAL _src_ts)
        return()
    endif()
endif()

execute_process(
    COMMAND robocopy "${SRC}" "${DST}" /E /R:1 /W:1 /NFL /NDL /NJH /NJS /NP
    RESULT_VARIABLE _rc
)
if(_rc GREATER_EQUAL 8)
    message(FATAL_ERROR "robocopy failed with ${_rc}: ${SRC} -> ${DST}")
endif()
