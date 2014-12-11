# Hey Emacs, this is a -*- makefile -*-

all: host client

host:
	$(MAKE) -C host

client:
	$(MAKE) -C client

.PHONY: all host client
