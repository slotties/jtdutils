LDFLAGS=-L ../libtd -I ../libtd -ltd -lexpat $(CUSTOM_LDFLAGS)

all: compile man

compile: 
	@echo "Compiling $(TOOL)..."
	$(CC) $(CFLAGS) -o $(TOOL) *.c $(LDFLAGS)
	@echo "$(MANPATH) $(BINPATH)"

man:
	@echo "Creating manpage for $(TOOL)..."
	@gzip -c $(TOOL).1 > $(TOOL).1.gz

install: man compile
	@echo "Installing binary to $(BINPATH) ..."
	@cp $(TOOL) $(BINPATH)
	@chmod 777 $(BINPATH)/$(TOOL)

	@echo "Installing man page to $(MANPATH)..."
	@cp $(TOOL).1.gz $(MANPATH)

clean:
	@echo "Cleaning up $(TOOL)..."
	@rm -f ./$(TOOL)
	@rm -f ./$(TOOL).1.gz

.PHONY: clean install all compile man
