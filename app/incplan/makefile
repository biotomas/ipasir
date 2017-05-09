IPASIRSOLVER	?= picosat961

all:
	cd src/incplan/build; \
		cmake ../src -DBUILD_TESTS=OFF -DIPASIR_DIR="../../../../../sat/${IPASIRSOLVER}"; \
		make
	cp src/incplan/bin/incplan-${IPASIRSOLVER} ./incplan
