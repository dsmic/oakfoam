## Update the build date and time
.PHONY: build.h

bin_PROGRAMS        = oakfoam
oakfoam_SOURCES     = build.h \
                      src/Oakfoam.h src/Oakfoam.cc src/main.cc \
                      src/engine/Go.h src/engine/Go.cc \
                      src/engine/Engine.h src/engine/Engine.cc \
                      src/engine/Tree.h src/engine/Tree.cc \
                      src/engine/Random.h src/engine/Random.cc \
                      src/engine/Pattern.h src/engine/Pattern.cc \
                      src/engine/Parameters.h src/engine/Parameters.cc \
                      src/engine/Features.h src/engine/Features.cc \
                      src/engine/Time.h src/engine/Time.cc \
                      src/engine/Book.h src/engine/Book.cc \
                      src/engine/Playout.h src/engine/Playout.cc \
                      src/engine/Benson.h src/engine/Benson.cc \
                      src/engine/Worker.h src/engine/Worker.cc \
                      src/web/Web.h src/web/Web.cc \
                      src/gtp/Gtp.h src/gtp/Gtp.cc
oakfoam_CPPFLAGS    = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)
oakfoam_LDFLAGS     = $(BOOST_LDFLAGS) $(MPI_LDFLAGS)
oakfoam_LDADD       = $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB)
dist_noinst_SCRIPTS = autogen.sh

.PHONY: oakfoam-profiled
oakfoam-profiled:
	@make clean all CXXFLAGS='$(CXXFLAGS) -fprofile-generate -march=native' oakfoam_LDFLAGS="-pthread -static $(BOOST_LDFLAGS)"
	./oakfoam --nobook -c profile.gtp
	@make clean all clean-profiled CXXFLAGS='$(CXXFLAGS) -fprofile-use -march=native' oakfoam_LDFLAGS="-pthread -static $(BOOST_LDFLAGS)"

.PHONY: clean-profiled
clean-profiled:
	rm -f src/*.gcda src/gtp/*.gcda src/engine/*.gcda

docs: docs/Doxyfile $(oakfoam_SOURCES)
	cd docs; \
	doxygen Doxyfile; \
	cd ..

