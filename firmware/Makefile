compile: obj elf bin

obj:
	arm-none-eabi-gcc -Wall -fno-common -std=gnu11 -mthumb -mcpu=cortex-m0 -Os -c main/*.c usb/*.c fatfs/*.c

elf:
	arm-none-eabi-ld -X -T linkScript.ld -o firmware.elf *.o -lgcc

bin:
	arm-none-eabi-objcopy -j .vectab -j .text -j .rodata -j .data -O binary firmware.elf firmware.bin

asm:
	arm-none-eabi-gcc -Wall -fno-common -std=gnu11 -mthumb -mcpu=cortex-m0 -Os -S main/*.c usb/*.c fatfs/*.c

clean:
	rm -f *.o *.s firmware.* *~ *#
	rm -f main/*~ main/*#
	rm -f usb/*~ usb/*#
	rm -f fatfs/*~ fatfs/*#
	rm -f cmsis/*~ cmsis/*#

upload:
	openocd -s ./openocd -f stlink-v2.cfg -f stm32f0x.cfg -c "program firmware.bin verify reset exit 0x08000000"

dfu: compile
	./dfuse-pack.py -b 0x08000000:firmware.bin firmware_13000.dfu
