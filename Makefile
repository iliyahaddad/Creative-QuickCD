WCC = wcc
WCL = wcl
WRC = wrc
PYTHON = python

DOS_OBJS = dos/dos_main.obj dos/cdrom.obj dos/ui.obj dos/ports.obj
WIN_OBJS = win/win_main.obj win/dialog.obj win/mci.obj
WIN_RES  = win/resource.res
DLLS     = dlls/ctccw.dll dlls/ctres.dll

.PHONY: all prepare clean dos windows dlls merge

all: prepare quickcd.exe

prepare:
	if not exist dos mkdir dos
	if not exist win mkdir win
	if not exist dlls mkdir dlls

dos: prepare dos/quickcd_dos.exe

windows: prepare win/quickcd_win.exe

dlls: prepare $(DLLS)

dos/dos_main.obj: src/dos/main.c src/common/types.h src/common/cdrom.h src/common/player.h
	$(WCC) -bt=dos -ms -i=src/common -fo=$@ $<

dos/cdrom.obj: src/dos/cdrom.c src/common/types.h src/common/cdrom.h src/common/player.h
	$(WCC) -bt=dos -ms -i=src/common -fo=$@ $<

dos/ui.obj: src/dos/ui.c src/common/types.h src/common/cdrom.h src/common/player.h
	$(WCC) -bt=dos -ms -i=src/common -fo=$@ $<

dos/ports.obj: src/dos/ports.c src/common/types.h src/common/cdrom.h src/common/player.h
	$(WCC) -bt=dos -ms -i=src/common -fo=$@ $<

dos/quickcd_dos.exe: $(DOS_OBJS)
	$(WCL) -bt=dos -ms $(DOS_OBJS) -fe=$@

win/win_main.obj: src/windows/main.c src/windows/types.h src/windows/cdrom.h src/windows/window.h src/common/types.h
	$(WCC) -bt=windows -bw -ms -i=src/common -fo=$@ $<

win/dialog.obj: src/windows/dialog.c src/windows/types.h src/windows/cdrom.h src/windows/window.h src/windows/resource.h src/common/types.h
	$(WCC) -bt=windows -bw -ms -i=src/common -fo=$@ $<

win/mci.obj: src/windows/mci.c src/windows/types.h src/windows/cdrom.h src/windows/window.h src/windows/resource.h src/common/types.h
	$(WCC) -bt=windows -bw -ms -i=src/common -fo=$@ $<

win/resource.res: src/windows/resource.rc src/windows/resource.h
	$(WRC) -r -fo=$@ $<

win/quickcd_win.exe: $(WIN_OBJS) $(WIN_RES)
	$(WCL) -bt=windows -bw -ms $(WIN_OBJS) $(WIN_RES) -fe=$@

dlls/ctccw.dll: dlls/ctccw.c dlls/ctccw.h src/windows/types.h src/common/types.h src/common/player.h
	$(WCL) -bt=windows -bw -bd -ms -i=src/common -i=src/windows $< -fe=$@

dlls/ctres.dll: dlls/ctres.c dlls/ctres.h src/common/types.h
	$(WCL) -bt=windows -bw -bd -ms -i=src/common $< -fe=$@

quickcd.exe: dos/quickcd_dos.exe win/quickcd_win.exe tools/merge_hybrid.py
	$(PYTHON) tools/merge_hybrid.py dos/quickcd_dos.exe win/quickcd_win.exe $@

merge: quickcd.exe

clean:
	if exist dos\*.obj del /q dos\*.obj
	if exist dos\*.exe del /q dos\*.exe
	if exist win\*.obj del /q win\*.obj
	if exist win\*.exe del /q win\*.exe
	if exist win\*.res del /q win\*.res
	if exist dlls\*.dll del /q dlls\*.dll
	if exist quickcd.exe del /q quickcd.exe
