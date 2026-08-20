To recompile the language string template, run `xgettext --package-name=miniCDi --package-version=0.1 --keyword=_ -d miniCDi -o miniCDi.pot ../mainFrame.cpp ../mainFrame.hpp`

For compiling to .mo, run:
```msgfmt -o <language>/miniCDi.mo <language>/miniCDi.po
move /Y <language>/miniCDi.mo ../../../../build/lang/<language>/miniCDi.mo```