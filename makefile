all:
	@echo "makefile: use 'make verbose' for verbose output"
	@scripts/mkall.sh|grep 'mkone'
verbose:
	scripts/mkall.sh
clean:
	scripts/mkcln.sh
