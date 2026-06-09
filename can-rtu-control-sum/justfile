os_name  := os()
compiler := if os_name == "windows" { "g++" }    else { "clang++" }
output   := if os_name == "windows" { "can_crc.exe" } else { "can_crc" }
runner   := if os_name == "windows" { "can_crc.exe" } else { "./can_crc" }
cxxflags := "-std=c++17 -O3 -march=native -Wall -Wextra" \
          + if os_name == "windows" { " -static -static-libgcc -static-libstdc++" } \
          else { "" }

default: build

# Kompilacja dla bieżącej platformy
build:
    {{compiler}} {{cxxflags}} -o {{output}} src/main.cpp
    @echo "Built: {{output}}  [{{os_name}} / {{arch()}}]"

# Kompilacja i uruchomienie
run: build
    {{runner}}

# Usunięcie pliku wykonywalnego
[unix]
clean:
    rm -f can_crc

[windows]
clean:
    -del /f can_crc.exe
