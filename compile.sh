# compile the test_app program
# input files: ./test_app/main.cpp
# output files: ./test_app/test_app
# headers files: none
# libraries: none
# dependencies: none

g++ -std=c++11 -Wall -Wextra -Wpedantic -Werror -o ./test_app/test_app ./test_app/main.cpp

# compile the dylib
# input files: ./dylib/main.cpp
# output files: ./dylib/libdylib.dylib
# headers files: ./dylib/main.h
# libraries: none
# dependencies: none

g++ -std=c++11 -Wall -Wextra -Wpedantic -Werror -dynamiclib -o ./dylib/libdylib.dylib ./dylib/main.cpp

# compile the encrypter program
# input files: ./encrypter/main.cpp
# output files: ./encrypter/encrypter
# headers files: none
# libraries: none
# dependencies: none

g++ -std=c++11 -Wall -Wextra -Wpedantic -Werror -o ./encrypter/encrypter ./encrypter/main.cpp