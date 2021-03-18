OBJECTS = main.o \
	  agc.o \
	  subinst.o \
	  control.o \
	  mem.o \
	  utils.o \

magc: $(OBJECTS)
	$(CC) -o magc $(OBJECTS)

clean:
	rm -f *.o magc
