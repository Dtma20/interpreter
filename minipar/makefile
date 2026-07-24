CXX := g++
CXXFLAGS := -std=c++17 -O2 -DNDEBUG -fPIC -Iinclude -MMD -MP
LDFLAGS :=

# Windows needs ws2_32 for socket functions
ifeq ($(OS),Windows_NT)
LDFLAGS += -lws2_32
endif

SRC := main.cpp \
	src/token.cpp src/error.cpp \
	src/ast/ast_base.cpp src/ast/ast_literals.cpp src/ast/ast_expressions.cpp src/ast/ast_statements.cpp \
	src/lexer/lexer_core.cpp src/lexer/lexer_rules.cpp \
	src/parser/parser_core.cpp src/parser/parser_stmt.cpp src/parser/parser_decl.cpp src/parser/parser_expr.cpp \
	src/semantic/semantic_core.cpp src/semantic/semantic_types.cpp src/semantic/semantic_scope.cpp src/semantic/semantic_control.cpp \
	src/interpreter/interpreter_core.cpp src/interpreter/interpreter_expr.cpp src/interpreter/interpreter_func.cpp src/interpreter/interpreter_builtin.cpp src/interpreter/interpreter_channel.cpp src/interpreter/interpreter_stmt.cpp

OBJ := $(SRC:.cpp=.o)

run: $(OBJ)
	$(CXX) $(CXXFLAGS) -o minipar $(OBJ) $(LDFLAGS)

debug: CXXFLAGS = -std=c++17 -g -DDEBUG_MODE -fPIC -Iinclude -MMD -MP
debug: run

%.d: ;

-include $(OBJ:.o=.d)

clean:
	rm -f $(OBJ) minipar *.d src/*.d src/*/*.d

test:
	@bash tests/run_tests.sh

.PHONY: run debug clean test