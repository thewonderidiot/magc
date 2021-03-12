OBJECTS = main.o \
	  agc.o \
	  subinst.o \
	  control.o \
	  mem.o \
	  utils.o \

magc: $(OBJECTS)
	gcc -o magc $(OBJECTS)

clean:
	rm *.o magc
