@echo off

set prodname=StarcraftAI

del /Q Products\*

echo Compiling...

:: Compile source files
for %%f in ( Source\*.cpp ) do ^
cl /c /nologo ^
 /O2 /Oi /GL ^
 /I ..\include ^
 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "EXAMPLEAIMODULE_EXPORTS" ^
			/D "_SECURE_SCL 0" /D "_WINDLL" /D "_UNICODE" /D "UNICODE" ^
 /EHsc /MD /Gy ^
 /FoProducts\ %%f

:: Link the obj files into a dll
echo Linking to Products\%prodname%.dll

link /nologo ^
 /OUT:Products\%prodname%.dll ^
 /OPT:REF /OPT:ICF /LTCG /NXCOMPAT ^
 /DLL /SUBSYSTEM:WINDOWS /MACHINE:X86 ^
 ../lib/BWAPI.lib ../lib/BWTA.lib ../lib/tinyxml.lib ../lib/CGAL-vc90-mt.lib ^
          ../lib/libboost_thread-vc90-mt-1_40.lib ../lib/gmp-vc90-mt.lib ^
		  ../lib/mpfr-vc90-mt.lib kernel32.lib user32.lib gdi32.lib winspool.lib ^
		  comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ^
		  odbc32.lib odbccp32.lib /NODEFAULTLIB:LIBCMT^
 Products\*.obj ^
 Resources\embed.manifest.res

:: Copy the dll into the BWAPI AI folder
copy /Y Products\*.dll "\Program Files (x86)\StarCraft\bwapi-data\AI\" >nul & echo Copying Products\%prodname%.dll to starcraft folder