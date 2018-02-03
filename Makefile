### Assumes GNU Make

EXE=test_orx_parsejson
EXTRAS=jsmn.c jsmn.h

all: $(EXE)

test: $(EXE)
	./test_orx_parsejson minimal.json

test_%: \
%.c %.h \
avltree.c avltree.h \
buffer_file.c buffer_file.h \
$(EXTRAS)
	gcc -DDO_MAIN $< -o $@

jsmn.%:
	wget -q https://raw.githubusercontent.com/zserge/jsmn/master/$@

clean:
	$(RM) $(EXE)

deepclean: clean
	$(RM) $(EXTRAS)

.PRECIOUS: $(EXTRAS)
