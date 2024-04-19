CXX=g++
CXXFLAGS=-Wall -pedantic -std=c++17

make: PoC/stakeGrindingAttack.PoC 

PoC/stakeGrindingAttack.PoC: src/grindingAttack.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) src/grindingAttack.cpp -o PoC/stakeGrindingAttack.PoC -lcrypto -lssl 

clean:
	rm -r PoC
