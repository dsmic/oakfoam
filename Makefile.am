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
                      src/engine/DecisionTree.h src/engine/DecisionTree.cc \
                      src/web/Web.h src/web/Web.cc \
                      src/gtp/Gtp.h src/gtp/Gtp.cc
oakfoam_CPPFLAGS    = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)
oakfoam_LDFLAGS     = $(BOOST_LDFLAGS) $(MPI_LDFLAGS) $(HOARD_LDFLAGS)
oakfoam_LDADD       = $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_ASIO_LIB) $(HOARD_LIB)
dist_noinst_SCRIPTS = autogen.sh build-linux.sh
EXTRA_DIST          = regression/sgf regression/*.sh regression/*.suite regression/*.tst \
                      scripts/*/README scripts/*/*.sh scripts/*/*.awk scripts/*/*.sed scripts/*/*.pl \
                      docs/Doxyfile www
dist_pkgdata_DATA   = web.gtp other/icon.png
install-data-hook:
	mkdir -p $(DESTDIR)$(pkgdatadir)
	cp -R $(srcdir)/www $(DESTDIR)$(pkgdatadir)
	if [ -f book.dat ]; then cp book.dat $(DESTDIR)$(pkgdatadir); fi
	echo "#!/bin/bash" > $(DESTDIR)$(bindir)/oakfoam-web
	echo "cd \`dirname \"\$$0\"\`" >> $(DESTDIR)$(bindir)/oakfoam-web
	echo "cd `echo $(bindir) | sed 's|^/||;s|[^/]*|\.\.|g'`$(pkgdatadir)" >> $(DESTDIR)$(bindir)/oakfoam-web
	echo "bin=\"`echo $(pkgdatadir) | sed 's|^/||;s|[^/]*|\.\.|g'`$(bindir)\"" >> $(DESTDIR)$(bindir)/oakfoam-web
	echo "\$$(sleep 3; xdg-open http://localhost:8000)&" >> $(DESTDIR)$(bindir)/oakfoam-web
	echo "\$$bin/oakfoam --web -c web.gtp" >> $(DESTDIR)$(bindir)/oakfoam-web
	echo "wait" >> $(DESTDIR)$(bindir)/oakfoam-web
	echo "echo 'Press <Enter> to continue...'" >> $(DESTDIR)$(bindir)/oakfoam-web
	echo "read -s" >> $(DESTDIR)$(bindir)/oakfoam-web
	chmod +x $(DESTDIR)$(bindir)/oakfoam-web
	mkdir -p $(DESTDIR)/usr/share/menu
	echo "?package(local.oakfoam):\\" > $(DESTDIR)/usr/share/menu/oakfoam
	echo "needs=\"X11\"\\" >> $(DESTDIR)/usr/share/menu/oakfoam
	echo "section=\"Games\"\\" >> $(DESTDIR)/usr/share/menu/oakfoam
	echo "title=\"Oakfoam Web Interface\"\\" >> $(DESTDIR)/usr/share/menu/oakfoam
	echo "command=\"oakfoam-web\"\\" >> $(DESTDIR)/usr/share/menu/oakfoam
	echo "icon=\"$(DESTDIR)$(pkgdatadir)/icon.png\"" >> $(DESTDIR)/usr/share/menu/oakfoam
	mkdir -p $(DESTDIR)/usr/share/applications
	echo "[Desktop Entry]" > $(DESTDIR)/usr/share/applications/oakfoam.desktop
	echo "Name=Oakfoam Web Interface" >> $(DESTDIR)/usr/share/applications/oakfoam.desktop
	echo "Comment=Oakfoam Web Interface for playing Go against the computer" >> $(DESTDIR)/usr/share/applications/oakfoam.desktop
	echo "Exec=oakfoam-web" >> $(DESTDIR)/usr/share/applications/oakfoam.desktop
	echo "Icon=$(DESTDIR)$(pkgdatadir)/icon.png" >> $(DESTDIR)/usr/share/applications/oakfoam.desktop
	echo "Terminal=true" >> $(DESTDIR)/usr/share/applications/oakfoam.desktop
	echo "Type=Application" >> $(DESTDIR)/usr/share/applications/oakfoam.desktop
	echo "Categories=Game;" >> $(DESTDIR)/usr/share/applications/oakfoam.desktop

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/oakfoam-web
	rm -rf $(DESTDIR)$(pkgdatadir)
	rm -f $(DESTDIR)/usr/share/menu/oakfoam
	rm -f $(DESTDIR)/usr/share/applications/oakfoam.desktop

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

