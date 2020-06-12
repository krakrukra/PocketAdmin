compile:
	g++ *.cpp -c
	g++ *.o -static -static-libgcc -static-libstdc++ -l gdi32 -l gdiplus -O2 -o ../binaries/scap.exe
	
clean:
	del scap.exe
	del	*.o