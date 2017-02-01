PROGRAM = a.out
OBJS = main.o shogiban.o kyokumencode.o

.SUFFIXES: .cpp .o

$(PROGRAM): $(OBJS)
	$(CXX) -o $(PROGRAM) $^ -lsqlite3

.cpp.o:
	$(CXX) -c $<

main.o: shogiban.h
shogiban.o: shogiban.h
kyokumencode.o: shogiban.h kyokumencode.h

