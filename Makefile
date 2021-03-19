OBJECTS = main.o \
	  agc.o \
	  scaler.o \
	  counter.o \
	  subinst.o \
	  control.o \
	  mem.o \
	  utils.o \

magc: $(OBJECTS)
	$(CC) -o magc $(OBJECTS)

clean:
	rm -f *.o magc
