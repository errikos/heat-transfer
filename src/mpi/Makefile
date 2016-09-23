CXX = mpicxx
CXXFLAGS = -pedantic -Wall -Wno-long-long -Wno-format-security

HDRS = macros.h heat_transfer.h heat_map.h mpi_wrapper.h optparse.h
SRCS = mpi.cc
OBJS = $(SRCS:.cc=.o)
EXEC = mpi_heat

%.o: %.cc $(HDRS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -rf $(EXEC) $(OBJS)
