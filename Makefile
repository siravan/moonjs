EMCC=/usr/local/emscripten/emcc
CFLAGS=-O1
DEPS = yaAGC.h agc_engine.h agc_symtab.h queue.h

%.o: %.c $(DEPS)
	$(EMCC) -c -o $@ $< $(CFLAGS)

agc.js: main.o \
        queue.o \
        agc_engine.o \
        agc_engine_init.o \
        agc_utilities.o 
        
	$(EMCC) -o $@ $^ $(CFLAGS) -s EXPORTED_FUNCTIONS="['_main','_advance', '_sendPort', '_scanPort']" --preload-file Core.bin


