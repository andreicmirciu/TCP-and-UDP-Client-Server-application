#Mirciu Andrei-Constantin 323CD

CFLAGS = -Wall -g

# Portul pe care asculta serverul
PORT_SERVER = 8080

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server subscriber

# Compileaza server.c
server: server.c

# Compileaza subscriber.c
subscriber: subscriber.c -lm

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT_SERVER}

# Ruleaza subscriberul
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber
