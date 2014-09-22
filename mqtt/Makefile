DIROBJ := obj/
DIREXE := exec/
DIRSRC := src/
DIRCLIENT := client/

CFLAGS := -I$(DIRSRC) -ggdb -c -Wall
LDFLAGS := -lrt -lcunit
CC := gcc
EXPORT := `export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/`

all: dirs mqtt-test mqtt_sub mqtt_pub

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

mqtt_sub: $(DIROBJ)mqtt_sub.o $(DIROBJ)mqtt.o
	$(CC) $(LDFLAGS) -o $(DIREXE)$@ $^

mqtt_pub: $(DIROBJ)mqtt_pub.o $(DIROBJ)mqtt.o
	$(CC) $(LDFLAGS) -o $(DIREXE)$@ $^

mqtt-test: $(DIROBJ)mqtt-test.o $(DIROBJ)mqtt.o
	$(CC) $(LDFLAGS) -o $(DIREXE)$@ $^

test: $(DIREXE)mqtt-test
	./$^

$(DIROBJ)%.o: $(DIRSRC)%.c
	$(CC) $(CFLAGS) $^ -o $@

$(DIROBJ)%.o: $(DIRCLIENT)%.c
	$(CC) $(CFLAGS) $^ -o $@

depend:
	makedepend $(CFLAGS) $(DIRSRC)

clean:
	rm -rf *~ core $(DIROBJ) $(DIREXE)
