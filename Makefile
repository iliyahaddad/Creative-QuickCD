WCC = wcc
WCL = wcl
WLINK = wlink
WRC = wrc

DOS_OBJS = dos/dos_main.obj dos/cdrom.obj dos/ui.obj dos/ports.obj
WIN_OBJS = win/win_main.obj win/dialog.obj win/mci.obj
WIN_RES  = win/resource.res
DLLS     = dlls/ctccw.dll dlls/ctres.dll

.PHONY: all clean dos windows dlls merge

all: quickcd.exe

dos: $(DOS_OBJS)

windows: $(WIN_OBJS) $(WIN_RES)

dlls: $(DLLS)

dos/dos_main.obj: src/dos/main.c src/common/types.h src/common/player.h
	$(WCC) -bt=dos -ms -zl -fo=$@ $<

dos/cdrom.obj: src/dos/cdrom.c src/common/types.h src/common/player.h
	$(WCC) -bt=dos -ms -zl -fo=$@ $<

dos/ui.obj: src/dos/ui.c src/common/types.h src/common/player.h
	$(WCC) -bt=dos -ms -zl -fo=$@ $<

dos/ports.obj: src/dos/ports.c src/common/types.h src/common/player.h
	$(WCC) -bt=dos -ms -zl -fo=$@ $<

dos/quickcd_dos.exe: $(DOS_OBJS)
	$(WCL) -bt=dos -ms -zl $^ -fe=$@

win/win_main.obj: src/windows/main.c src/common/types.h src/common/player.h src/windows/window.h
	$(WCC) -bt=windows -bw -ms -zl -fo=$@ $<

win/dialog.obj: src/windows/dialog.c src/common/types.h src/common/player.h src/windows/window.h
	$(WCC) -bt=windows -bw -ms -zl -fo=$@ $<

win/mci.obj: src/windows/mci.c src/common/types.h src/common/player.h src/windows/window.h
	$(WCC) -bt=windows -bw -ms -zl -fo=$@ $<

win/resource.res: src/windows/resource.rc src/windows/resource.h
	$(WRC) -r -fo=$@ $<

win/quickcd_win.exe: $(WIN_OBJS) $(WIN_RES)
	$(WCL) -bt=windows -bw -ms -zl $^ -fe=$@

dlls/ctccw.dll: dlls/ctccw.c dlls/ctccw.h src/common/types.h
	$(WCL) -bt=windows -bw -bd -ms -zl $< -fe=$@

dlls/ctres.dll: dlls/ctres.c dlls/ctres.h
	$(WCL) -bt=windows -bw -bd -ms -zl $< -fe=$@

quickcd.exe: dos/quickcd_dos.exe win/quickcd_win.exe
	python tools/merge_hybrid.py dos/quickcd_dos.exe win/quickcd_win.exe $@

clean:
	del /q dos\\*.obj dos\\*.exe
	del /q win\\*.obj win\\*.exe win\\*.res
	del /q dlls\\*.dll
	del /q quickcd.exe
