CXX = mpicxx
CXXFLAGS = -pedantic -Wall -fopenmp -Wno-long-long -Wno-format-security -Wno-unused-variable

HDRS = macros.h heat_transfer.h heat_map.h mpi_wrapper.h optparse.h
SRCS = hybrid.cc
OBJS = $(SRCS:.cc=.o)
EXEC = hybrid_heat

%.o: %.cc $(HDRS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -rf $(EXEC) $(OBJS)
