compile:
	g++ *.cpp -c
	g++ *.o -static -static-libgcc -static-libstdc++ -l gdi32 -l gdiplus -O2 -mwindows -o ../binaries/scap.exe
	
clean:
	del ..\binaries\scap.exe
	del	*.o