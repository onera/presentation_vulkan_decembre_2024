#include Make_linux.inc
include Make_msys2.inc
#include Make_osx.inc

CXXFLAGS = -std=c++14
ifdef DEBUG
CXXFLAGS += -g -O0 -Wall -fbounds-check -pedantic -D_GLIBCXX_DEBUG
CXXFLAGS2 = CXXFLAGS
else
CXXFLAGS2 = ${CXXFLAGS} -O3 -march=native
CXXFLAGS += -march=native -Wall
endif

ALL= TestProductMatrix.exe test_product_matrice_blas.exe 

default:	help

all: $(ALL)

clean:
	@rm -fr *.o *.exe *~

.cpp.o:
	$(CXX) $(CXXFLAGS2) -c $^ -o $@	


TestProductMatrix.exe : TestProductMatrix.o Matrix.hpp Matrix.o ProdMatMat.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)	

test_product_matrice_blas.exe : test_product_matrice_blas.o Matrix.hpp Matrix.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)	$(BLAS)

help:
	@echo "Available targets : "
	@echo "    all            : compile all executables"
	@echo "Add DEBUG=yes to compile in debug"
	@echo "Configuration :"
	@echo "    CXX      :    $(CXX)"
	@echo "    CXXFLAGS :    $(CXXFLAGS)"

%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@
