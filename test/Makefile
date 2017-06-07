PROGRAM = tester
TESTOBJS = mainTest.o kyokumencodeTest.o shogiruleTest.o shogigameTest.o shogidbTest.o
OBJS = $(filter-out ../main.o,$(wildcard ../*.o))

.SUFFIXES: .cpp .o

$(PROGRAM): $(TESTOBJS) $(OBJS)
	$(CXX) -o $(PROGRAM) $^ -lsqlite3

.cpp.o:
	$(CXX) -c -g $<

.PHONY: test
test:
	./$(PROGRAM)

.PHONY: depend
depend: $(TESTOBJS:.o=.cpp)
	-@ $(RM) depend.inc
	-@ for i in $^; do $(CXX) -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.cpp//g" >> depend.inc; done
-include depend.inc
