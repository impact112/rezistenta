add_executable(ReziEdit Entry.cpp Editor.cpp gui/Utils.cpp gui/Button.cpp gui/TextBox.cpp)

include_directories(ReziEdit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libReziEngine)
target_link_libraries(ReziEdit PRIVATE libReziEngine)
target_link_libraries(ReziEdit PRIVATE raylib)
target_link_libraries(ReziEdit PRIVATE Eigen3::Eigen)
