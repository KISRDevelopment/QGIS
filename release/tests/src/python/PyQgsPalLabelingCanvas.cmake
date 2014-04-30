
IF(WIN32)
  SET(ENV{QGIS_PREFIX_PATH} "/home/aziz/Development/QGIS/release/output/bin/${CMAKE_BUILD_TYPE}")
  SET(ENV{PATH} "/home/aziz/Development/QGIS/release/output/bin/${CMAKE_BUILD_TYPE};$ENV{PATH}")
  SET(ENV{PYTHONPATH} "/home/aziz/Development/QGIS/release/output/python/;$ENV{PYTHONPATH}")
  MESSAGE("PATH:$ENV{PATH}")
ELSE(WIN32)
  SET(ENV{QGIS_PREFIX_PATH} "/home/aziz/Development/QGIS/release/output")
  SET(ENV{LD_LIBRARY_PATH} "NOTFOUND:/home/aziz/Development/QGIS/release/output/lib:$ENV{LD_LIBRARY_PATH}")
  SET(ENV{PYTHONPATH} "/home/aziz/Development/QGIS/release/output/python/:$ENV{PYTHONPATH}")
  MESSAGE("LD_LIBRARY_PATH:$ENV{LD_LIBRARY_PATH}")
ENDIF(WIN32)

MESSAGE("PYTHONPATH:$ENV{PYTHONPATH}")
MESSAGE(STATUS "Running /usr/bin/python /home/aziz/Development/QGIS/tests/src/python/test_qgspallabeling_canvas.py ")
EXECUTE_PROCESS(
  COMMAND /usr/bin/python /home/aziz/Development/QGIS/tests/src/python/test_qgspallabeling_canvas.py 
  #WORKING_DIRECTORY @LIBRARY_OUTPUT_PATH@
  RESULT_VARIABLE import_res
  OUTPUT_VARIABLE import_output
  ERROR_VARIABLE  import_output
)
# Pass the output back to ctest
IF(import_output)
  MESSAGE(${import_output})
ENDIF(import_output)
IF(import_res)
  MESSAGE(SEND_ERROR ${import_res})
ENDIF(import_res)
