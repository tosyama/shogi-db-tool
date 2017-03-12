PROGRAM = a.out
OBJS = main.o shogiban.o kyokumencode.o sashite.o kifu.o shogidb.o shogirule.o

.SUFFIXES: .cpp .o

$(PROGRAM): $(OBJS)
	$(CXX) -o $(PROGRAM) $^ -lsqlite3

.cpp.o:
	$(CXX) -c -g $<

main.o: shogiban.h kyokumencode.h sashite.h
shogiban.o: shogiban.h
kyokumencode.o: shogiban.h kyokumencode.h
sashite.o: shogiban.h sashite.h
kifu.o: kifu.h
.PHONY: depend
depend: $(OBJS:.o=.cpp)
	-@ $(RM) depend.inc
	-@ for i in $^; do $(CXX) -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.cpp//g" >> depend.inc; done
-include depend.inc
.PHONY: package
package:
	apt-get -y install libsqlite3-dev

