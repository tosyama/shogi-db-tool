PROGRAM = a.out
OBJS = main.o shogiban.o kyokumencode.o sashite.o kifu.o shogidb.o shogirule.o
TEST = test/tester
TESTCASES = test/*Test.cpp

.SUFFIXES: .cpp .o

$(PROGRAM): $(OBJS) $(TEST)
	cd test && $(MAKE) test
	$(CXX) -o $(PROGRAM) $(OBJS) -lsqlite3

.cpp.o:
	$(CXX) -c -g $<

$(TEST): $(OBJS) $(TESTCASES)
	$(MAKE) -C test

.PHONY: depend
depend: $(OBJS:.o=.cpp)
	-@ $(RM) depend.inc
	-@ for i in $^; do $(CXX) -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.cpp//g" >> depend.inc; done
-include depend.inc
.PHONY: package
package:
	-apt-get -y install libsqlite3-dev
	-curl -o test/catch.hpp https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
