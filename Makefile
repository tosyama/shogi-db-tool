PROGRAM = a.out
OBJS = main.o shogiban.o kyokumencode.o sashite.o kifu.o

.SUFFIXES: .cpp .o

$(PROGRAM): $(OBJS)
	$(CXX) -o $(PROGRAM) $^ -lsqlite3

.cpp.o:
	$(CXX) -c $<

main.o: shogiban.h kyokumencode.h sashite.h
shogiban.o: shogiban.h
kyokumencode.o: shogiban.h kyokumencode.h
sashite.o: shogiban.h sashite.h
kifu.o: kifu.h
.PHONY: depend
depend: $(OBJS:.o=.cpp)
	-@ for i in $^ ;do $(CXX) -MM $$i; done
