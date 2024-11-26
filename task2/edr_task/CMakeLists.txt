add_executable (edr_task "edr_task.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET edr_task PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(edr_task PUBLIC "${PROJECT_SOURCE_DIR}/edr_task/include/")
