CC = cc -Wall -O2
OBJ = npipe.o
BIN = npipe

default: $(OBJ)
	$(CC) -o $(BIN) $(OBJ)

clean:
	rm -f $(OBJ) $(BIN)
	rm -f in out

