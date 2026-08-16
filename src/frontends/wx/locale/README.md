To recompile the language string template, run `xgettext --package-name=miniCDi --package-version=0.1 --keyword=_ -d miniCDi -o miniCDi.pot ../mainFrame.cpp ../mainFrame.hpp`

For compiling to .mo, run:
```msgfmt -o <language>.mo <language>.po
move /Y <language>.mo ../../../../build/lang/<language>/miniCDi.mo```