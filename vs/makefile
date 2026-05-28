all: Xonix.exe

Xonix.exe: ../src/Xonix.c Xonix.res
	cl /O2 /EHsc /D NDEBUG ../src/Xonix.c Xonix.res /link user32.lib kernel32.lib gdi32.lib

Xonix.res: ../src/Xonix.rc
	rc /fo Xonix.res ../src/Xonix.rc

clean:
	del *.obj *.res *.exe
