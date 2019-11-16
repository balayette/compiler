CXXFLAGS = -Wall -Wextra -Werror -std=c++17 -pedantic -g -MMD
LINK.o = $(LINK.cc)

GENERATED_SRC = \
		src/parser.cc \
		src/scanner.cc \

GENERATED_HDR = \
		src/parser.hh \
		src/location.hh \

GENERATED = $(GENERATED_SRC) $(GENERATED_HDR)
GENERATED_OBJ = $(GENERATED_SRC:.cc=.o)

OBJ = \
      $(GENERATED_OBJ) \
      src/scanner.o \
      src/parser.o \
      src/main.o \
      src/symbol.o \
      src/driver.o \
      src/compile.o \
      src/types.o \
      src/exp.o \

DEP = $(OBJ:.o=.d)

BIN_OUT = jit

$(BIN_OUT): src/main
	cp $^ $@

src/main: $(OBJ)

src/main.o: $(GENERATED)

src/location.hh: src/parser.hh
src/parser.hh: src/parser.cc
src/parser.cc: src/parser.yy
	bison -v -t src/parser.yy -o src/parser.cc --defines=src/parser.hh

src/scanner.cc: src/parser.hh src/scanner.ll
	flex -f -o src/scanner.cc src/scanner.ll

clean:
	$(RM) $(OBJ) src/main $(BIN_OUT) $(DEP) $(GENERATED)

-include $(DEP)
