CXX=g++ --save-temps 

opal_test: opal_test.o
	$(CXX) -O0 -o opal_test -fopenmp opal_test.o

opal_test.o: opal_test.c
	$(CXX) -O0 -o opal_test.o -fopenmp -c opal_test.c

opal_test2: opal_test2.o
	$(CXX) -O0 -o opal_test2 -fopenmp opal_test2.o

opal_test2.o: opal_test2.c
	$(CXX) -O0 -o opal_test2.o -fopenmp -c opal_test2.c

opal_mlm: opal_mlm.o
	$(CXX) -O0 -o opal_mlm -fopenmp opal_mlm.o

opal_mlm.o: opal_mlm.c
	$(CXX) -O0 -o opal_mlm.o -fopenmp -c opal_mlm.c

all: opal_test opal_test2 opal_mlm

clean:
	rm opal_test opal_test.o opal_test2 opal_test2.o opal_mlm opal_mlm.o