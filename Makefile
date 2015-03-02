
TCL_LIB_PATH = /depot/tcl8.5/lib
TCL_INC_PATH = /depot/tcl8.5/include
TCL_CONFIG   = /depot/tcl8.5/lib/tclConfig.sh

all: bin/parsetcl bin/parsetcl.so

bin/parsetcl: src/parsetcl.main.c src/parsetcl.cpp
	g++ -o $@ -D__MAIN__ -Wl,-rpath=$(TCL_LIB_PATH) -I$(TCL_INC_PATH)  -L$(TCL_LIB_PATH) -l tcl8.5 $<

bin/parsetcl.so: src/parsetcl.cpp
	g++ -shared -o $@ -fPIC -DUSE_TCL_STUBS -I$(TCL_INC_PATH) $(TCL_LIB_PATH)/libtclstub8.5.a $^


clean:
	rm parsetcl parsetcl.so
