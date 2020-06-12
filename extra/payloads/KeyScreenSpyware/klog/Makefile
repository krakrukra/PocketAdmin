compile:
	gcc *.c -c
	gcc *.o -static -static-libgcc -l psapi -O2 -o ../binaries/klog.exe
	
clean:
	del klog.exe
	del	*.o
	del *.txt