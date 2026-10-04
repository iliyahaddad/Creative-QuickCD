# Open Watcom wmake makefile.
#   wmake          build everything -> quickcd.exe (hybrid DOS/Windows EXE)
#   wmake dos      DOS part only
#   wmake windows  Windows part only
#   wmake dlls     CTCCW.DLL / CTRES.DLL
#   wmake clean
#
# Requires the WATCOM environment variable (see BUILD.md).

WCC = wcc
WCL = wcl
WRC = wrc
PYTHON = python

DOS_OBJS = dos/dos_main.obj dos/cdrom.obj dos/ui.obj dos/ports.obj
WIN_OBJS = win/win_main.obj win/dialog.obj win/mci.obj
WIN_RES  = win/resource.res
DLLS     = dlls/ctccw.dll dlls/ctres.dll

DOS_CFLAGS = -bt=dos -ms -i=src/common
WIN_INC    = -i=src/windows -i=$(%WATCOM)/h/win
WIN_CFLAGS = -bt=windows -bw -ms $(WIN_INC)
DLL_CFLAGS = -bt=windows -bd -mc -zu $(WIN_INC)

# NOTE: in wmake, $[@ is the first dependent (the .c file) and $@ the target.

all: .SYMBOLIC prepare quickcd.exe

prepare: .SYMBOLIC
	if not exist dos mkdir dos
	if not exist win mkdir win
	if not exist dlls mkdir dlls

dos: .SYMBOLIC prepare dos/quickcd_dos.exe

windows: .SYMBOLIC prepare win/quickcd_win.exe

dlls: .SYMBOLIC prepare $(DLLS)

merge: .SYMBOLIC quickcd.exe

test: .SYMBOLIC
	$(PYTHON) -m unittest discover -s test/python -v

# ---------------------------------------------------------------- DOS part

dos/dos_main.obj: src/dos/main.c src/common/types.h src/common/cdrom.h src/dos/ui.h
	$(WCC) $(DOS_CFLAGS) -fo=$@ $[@

dos/cdrom.obj: src/dos/cdrom.c src/common/types.h src/common/cdrom.h
	$(WCC) $(DOS_CFLAGS) -fo=$@ $[@

dos/ui.obj: src/dos/ui.c src/common/types.h src/common/cdrom.h src/dos/ui.h
	$(WCC) $(DOS_CFLAGS) -fo=$@ $[@

dos/ports.obj: src/dos/ports.c src/common/types.h src/common/cdrom.h
	$(WCC) $(DOS_CFLAGS) -fo=$@ $[@

dos/quickcd_dos.exe: $(DOS_OBJS)
	$(WCL) -bt=dos -ms $(DOS_OBJS) -fe=$@

# ------------------------------------------------------------ Windows part

win/win_main.obj: src/windows/main.c src/windows/types.h src/windows/window.h src/windows/resource.h src/common/types.h
	$(WCC) $(WIN_CFLAGS) -fo=$@ $[@

win/dialog.obj: src/windows/dialog.c src/windows/types.h src/windows/cdrom.h src/windows/window.h src/windows/resource.h src/common/types.h
	$(WCC) $(WIN_CFLAGS) -fo=$@ $[@

win/mci.obj: src/windows/mci.c src/windows/types.h src/windows/cdrom.h src/windows/window.h src/windows/resource.h src/common/types.h
	$(WCC) $(WIN_CFLAGS) -fo=$@ $[@

win/resource.res: src/windows/resource.rc src/windows/resource.h
	$(WRC) -r -bt=windows $(WIN_INC) -fo=$@ $[@

win/quickcd_win.exe: $(WIN_OBJS) $(WIN_RES)
	$(WCL) -l=windows -bt=windows -bw -ms -k8192 $(WIN_OBJS) $(WIN_RES) -fe=$@ -"library mmsystem"

# --------------------------------------------------------------------- DLLs

dlls/ctccw.dll: dlls/ctccw.c dlls/ctccw.h src/windows/types.h src/common/types.h
	$(WCL) -l=windows_dll $(DLL_CFLAGS) $[@ -fo=dlls/ctccw.obj -fe=$@ -"library mmsystem"

dlls/ctres.dll: dlls/ctres.c dlls/ctres.h
	$(WCL) -l=windows_dll $(DLL_CFLAGS) $[@ -fo=dlls/ctres.obj -fe=$@

# ------------------------------------------------------------ hybrid merge

quickcd.exe: dos/quickcd_dos.exe win/quickcd_win.exe tools/merge_hybrid.py
	$(PYTHON) tools/merge_hybrid.py dos/quickcd_dos.exe win/quickcd_win.exe $@

clean: .SYMBOLIC
	if exist dos\*.obj del /q dos\*.obj
	if exist dos\*.exe del /q dos\*.exe
	if exist win\*.obj del /q win\*.obj
	if exist win\*.exe del /q win\*.exe
	if exist win\*.res del /q win\*.res
	if exist dlls\*.obj del /q dlls\*.obj
	if exist dlls\*.dll del /q dlls\*.dll
	if exist quickcd.exe del /q quickcd.exe
