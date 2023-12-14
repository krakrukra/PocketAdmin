compile:
	gcc *.c -c
	gcc *.o -static -static-libgcc -l psapi -O2 -mwindows -o ../binaries/klog.exe
	
clean:
	del ..\binaries\klog.exe
	del	*.o
