# Embed a file as an byte array.
# Makes a C file containing the array and appends it to the variable
# ${EMBED_FILE_LIST}, which can then be included in a build command.
function(embed_file input_file object_name)
    file(READ ${input_file} hex_string HEX)
    string(REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" hex_array ${hex_string})
    set(i 0)
    foreach(byte IN LISTS hex_array)
        string(APPEND output_values "0x${byte},")
        math(EXPR i "${i}+1")
        if(i EQUAL 32)
            string(APPEND output_values "\n")
            set(i 0)
        endif()
    endforeach()
    set(output_string "// Generated from ${input_file}.
#include <stdint.h>
#include <stddef.h>
const uint8_t ${object_name}[] = {
${output_values}
};
const size_t ${object_name}_size = sizeof(${object_name});
")
    set(output_file ${CMAKE_CURRENT_BINARY_DIR}/${input_file}.c)
    file(WRITE ${output_file} "${output_string}")
    list(APPEND EMBED_FILE_LIST ${output_file})
    set(EMBED_FILE_LIST ${EMBED_FILE_LIST} PARENT_SCOPE)
endfunction()
