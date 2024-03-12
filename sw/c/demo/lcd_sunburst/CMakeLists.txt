
add_library(lcd_sunburst_lib
${CMAKE_CURRENT_SOURCE_DIR}/../../../../vendor/display_drivers/core/lcd_base.c
${CMAKE_CURRENT_SOURCE_DIR}/../../../../vendor/display_drivers/core/lucida_console_10pt.c
${CMAKE_CURRENT_SOURCE_DIR}/../../../../vendor/display_drivers/core/lucida_console_12pt.c
${CMAKE_CURRENT_SOURCE_DIR}/../../../../vendor/display_drivers/st7735/lcd_st7735.c
)

# add_executable(lcd_st7735 main.c)
add_executable(lcd_sunburst main.c lcd.c)

# pull in core dependencies and additional i2c hardware support
target_link_libraries(lcd_sunburst common lcd_sunburst_lib)

target_include_directories(lcd_sunburst PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../vendor/display_drivers)

add_custom_command(
  TARGET lcd_sunburst POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:lcd_sunburst>" "$<TARGET_FILE:lcd_sunburst>.bin"
  COMMAND srec_cat "$<TARGET_FILE:lcd_sunburst>.bin" -binary -offset 0x0000 -byte-swap 4 -o "$<TARGET_FILE:lcd_sunburst>.vmem" -vmem
  VERBATIM)
