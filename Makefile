CFLAGS = -Wall -Wextra -Werror -O3 -MMD -MP
LDFLAGS = -lrt

SOURCES = main \
	  agc \
	  scaler \
	  counter \
	  subinst \
	  control \
	  mem \
	  utils \
	  dsky \

OBJECTS = $(addprefix build/,$(addsuffix .o,$(SOURCES)))
DEPENDS = $(patsubst %.o,%.d,$(OBJECTS))

magc: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

build:
	mkdir -p build

build/%.o: src/%.c build
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(DEPENDS)

clean:
	rm -rf build magc
