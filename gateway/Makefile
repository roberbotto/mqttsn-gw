DIROBJ := obj/
DIREXE := exec/
DIRSRC := src/

CFLAGS := -I$(DIRSRC) -ggdb -c -Wall
LDFLAGS := -lrt
CC := g++

all: dirs tGatewayServer

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

tGatewayServer: $(DIROBJ)mqttsnSF.o $(DIROBJ)utils.o $(DIROBJ)mqtt.o $(DIROBJ)tGatewayServer.o
	$(CC) $(LDFLAGS) -o $(DIREXE)$@ $^

$(DIROBJ)%.o: $(DIRSRC)%.c
	$(CC) $(CFLAGS) $^ -o $@

$(DIROBJ)%.o: $(DIRSRC)%.cpp
	$(CC) $(CFLAGS) $^ -o $@

depend:
	makedepend $(CFLAGS) $(DIRSRC)

clean:
	rm -rf *~ core $(DIROBJ) $(DIREXE)
