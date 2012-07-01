include ../Makefile.inc 

LIB=libtd.a
SRC=list.c td.c td_xml.c conf.c
OBJ=${SRC:.c=.o}

.SUFFIXES:
.SUFFIXES: .o .c

all: $(LIB)

clean: 
	@echo "Cleaning up libtd..."
	rm -f $(LIB) $(OBJ)

$(LIB): $(OBJ)
	@echo "Creating library..."
	ar rs $@ $+

.c.o:
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $<

$(OBJ): common.h

.PHONY: all clean
