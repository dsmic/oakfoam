bin_PROGRAMS        = oakfoam
oakfoam_SOURCES     = src/Oakfoam.h src/Oakfoam.cc src/main.cc \
                      src/engine/Go.h src/engine/Go.cc \
                      src/engine/Engine.h src/engine/Engine.cc \
                      src/engine/UCT.h src/engine/UCT.cc \
                      src/engine/Random.h src/engine/Random.cc \
                      src/gtp/Gtp.h src/gtp/Gtp.cc
                      
dist_noinst_SCRIPTS = autogen.sh

.PHONY: oakfoam-profiled
oakfoam-profiled:
	@make clean all CXXFLAGS="$(CXXFLAGS) -fprofile-generate -march=native"
	./oakfoam -c profile.gtp
	@make clean all clean-profiled CXXFLAGS="$(CXXFLAGS) -fprofile-use -march=native"

.PHONY: oakfoam-profiled
clean-profiled:
	rm -f src/*.gcda src/gtp/*.gcda src/engine/*.gcda


