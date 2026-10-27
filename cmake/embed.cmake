file(READ ${IN} hex HEX)
string(REGEX REPLACE "(..)" "0x\\1," bytes "${hex}")
file(WRITE ${OUT} "#include <cstddef>\nextern const unsigned char ${NAME}[] = {${bytes}};\nextern const size_t ${NAME}_size = sizeof(${NAME});\n")
