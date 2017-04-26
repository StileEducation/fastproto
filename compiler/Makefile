all: rb_fastproto_compiler

.PHONY: all clean

XXD ?= xxd

CFLAGS += -I/usr/local/include
CXXFLAGS += $(CFLAGS) -std=c++14
LDFLAGS += -L/usr/local/lib
XXDFLAGS ?=

runtime_SOURCES := $(wildcard runtime/*.cpp runtime/*.h runtime/*.rb)
runtime_BUNDLES := $(addsuffix .res.c,$(runtime_SOURCES))
runtime_OBJECTS := $(runtime_BUNDLES:.res.c=.res.o)

rb_fastproto_compiler_SOURCES := $(wildcard src/*.cpp)
rb_fastproto_compiler_HEADERS := $(wildcard src/*.h)
rb_fastproto_compiler_OBJECTS := $(rb_fastproto_compiler_SOURCES:.cpp=.o)

%.res.c: %
	$(XXD) $(XXDFLAGS) -i $< $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

rb_fastproto_compiler: $(rb_fastproto_compiler_OBJECTS) $(runtime_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -lprotoc -lprotobuf -lboost_filesystem -lboost_system -lboost_regex

clean:
	rm -vf src/*.o runtime/*.o runtime/*.res.c rb_fastproto_compiler
