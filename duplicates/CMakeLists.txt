project(duplicates VERSION 0.0.1)

set(projectLib "${PROJECT_NAME}")
set(projectApp "${PROJECT_NAME}-app")
set(projectTest "${PROJECT_NAME}-test")

add_subdirectory(src)
add_subdirectory(test)
