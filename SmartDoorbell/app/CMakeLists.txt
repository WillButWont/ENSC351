# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(smart_doorbell ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(smart_doorbell LINK_PRIVATE hal jpeg gpiod)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET smart_doorbell POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:smart_doorbell>"
     "~/ensc351/public/myApps/smart_doorbell" 
  COMMENT "Copying ARM executable to public NFS directory")

# Copy the 'security-system' folder to ~/ensc351/public/myApps/security-system/
add_custom_command(TARGET smart_doorbell POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
     "${CMAKE_SOURCE_DIR}/security-system"
     "$ENV{HOME}/ensc351/public/myApps/security-system"
  COMMENT "Copying Wave files to public NFS directory")

  # Copy the 'audio-files' folder to ~/ensc351/public/myApps/audio-files
add_custom_command(TARGET smart_doorbell POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
     "${CMAKE_SOURCE_DIR}/audio-files"
     "$ENV{HOME}/ensc351/public/myApps/audio-files"
  COMMENT "Copying Wave files to public NFS directory")
