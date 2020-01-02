CXX=/usr/local/cuda/bin/nvcc
CXXFLAGS=-g --compiler-options -Wall -O3 
SRC=main.cu 
INC=common

main: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -I $(INC) -o $@

clean:
	rm -f *.o *~ *~ main
	rm -rf *.dSYM
