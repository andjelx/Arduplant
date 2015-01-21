#Контроллер, установленный на плате. Может быть другим, например atmega328 
DEVICE     = atmega328

#Тактовая частота 16 МГц 
CLOCK      = 16000000

#Команда запуска avrdude. Ее нужно скопировать из Arduino IDE.
AVRDUDE = /Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avrdude -C/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/etc/avrdude.conf -carduino -P/dev/tty.usbserial-A600dAAQ -b19200 -D -p atmega168

OBJECTS    = Arduplant.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

all:	Arduplant.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:Arduplant.hex:i

clean:
	rm -f Arduplant.hex Arduplant.elf $(OBJECTS)

Arduplant.elf: $(OBJECTS)
	$(COMPILE) -o Arduplant.elf $(OBJECTS)

Arduplant.hex: Arduplant.elf
	rm -f Arduplant.hex
	avr-objcopy -j .text -j .data -O ihex Arduplant.elf Arduplant.hex
	avr-size --format=avr --mcu=$(DEVICE) Arduplant.elf
