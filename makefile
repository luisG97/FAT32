TARGET	= mfs
SRC1	= mfs.c

$(TARGET): $(SRC1)
	g++ $(SRC1) -o $(TARGET)

clean:
	rm mfs
