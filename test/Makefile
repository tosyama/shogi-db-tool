PROGRAM = tester
OBJS = mainTest.o shogiruleTest.o \
	$(filter-out ../main.o,$(wildcard ../*.o))

.SUFFIXES: .cpp .o

$(PROGRAM): $(OBJS)
	$(CXX) -o $(PROGRAM) $^ -lsqlite3

.cpp.o:
	$(CXX) -c -g $<

.PHONY: test
test:
	./$(PROGRAM)

