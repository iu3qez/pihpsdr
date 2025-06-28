@echo off
REM Setup environment for piHPSDR

REM Set GDK_PIXBUF_MODULEDIR to use relative path
set GDK_PIXBUF_MODULEDIR=%~dp0lib\gdk-pixbuf-2.0\2.10.0\loaders
set GDK_PIXBUF_MODULE_FILE=%~dp0lib\gdk-pixbuf-2.0\2.10.0\loaders.cache

REM Set GTK paths
set GTK_DATA_PREFIX=%~dp0
set GTK_EXE_PREFIX=%~dp0

REM Run piHPSDR
"%~dp0pihpsdr.exe" %*
