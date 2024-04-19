CXX=g++
CXXFLAGS=-Wall -pedantic -std=c++17
LDFLAGS=-Wall -pedantic -std=c++17
LD=g++

make: PoC/raceAttack.PoC PoC/finneyAttack.PoC PoC/vector76Attack.PoC PoC/51Attack.PoC

PoC/raceAttack.PoC: build/raceAttack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) build/raceAttack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o -o PoC/raceAttack.PoC

PoC/finneyAttack.PoC: build/finneyAttack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) build/finneyAttack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o -o PoC/finneyAttack.PoC

PoC/vector76Attack.PoC: build/vector76Attack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) build/vector76Attack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o -o PoC/vector76Attack.PoC

PoC/51Attack.PoC: build/51Attack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) build/51Attack.o build/CParameterPreparator.o build/CSynchronizationGuard.o build/CTransactionHandler.o -o PoC/51Attack.PoC

build/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/CParameterPreparator.o: src/CParameterPreparator.cpp src/CParameterPreparator.h 
build/CSynchronizationGuard.o: src/CSynchronizationGuard.cpp src/CSynchronizationGuard.h
build/CTransactionHandler.o: src/CTransactionHandler.cpp src/CTransactionHandler.h
build/raceAttack.o: src/raceAttack.cpp
build/finneyAttack.o: src/finneyAttack.cpp 
build/vector76Attack.o: src/vector76Attack.cpp 
build/51Attack.o: src/51Attack.cpp  

clean:
	rm -r build PoC;
