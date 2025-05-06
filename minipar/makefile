run:
	g++ -g -o minipar main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/token.cpp src/error.cpp src/ast.cpp src/interpreter.cpp -std=c++17 -fPIC

debug:
	g++ -DDEBUG_MODE -g -o minipar main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/token.cpp src/error.cpp src/ast.cpp src/interpreter.cpp -std=c++17 -fPIC