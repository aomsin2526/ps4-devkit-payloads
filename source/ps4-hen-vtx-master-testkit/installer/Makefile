LIBPS4	:=	$(PS4SDK)/libPS4

TEXT	:=	0x926200000
DATA	:=	0x926300000

CC		:=	gcc
AS		:=	gcc
OBJCOPY	:=	objcopy
ODIR	:=	build
SDIR	:=	source
SDIR2	:=	source/kern-resolver
IDIRS	:=	-I$(LIBPS4)/include -I. -Iinclude -I/usr/src/sys
LDIRS	:=	-L$(LIBPS4) -L. -Llib
CFLAGS	:=	$(IDIRS) -O0 -std=gnu11 -fno-builtin -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=large -DTEXT_ADDRESS=$(TEXT) -DDATA_ADDRESS=$(DATA)
SFLAGS	:=	-nostartfiles -nostdlib -march=btver2 -mtune=btver2
LFLAGS	:=	$(LDIRS) -Xlinker -T $(LIBPS4)/linker.x -Wl,--build-id=none -Ttext=$(TEXT) -Tdata=$(DATA)
CFILES	:=	$(wildcard $(SDIR)/*.c)
CFILES2	:=	$(wildcard $(SDIR2)/*.c)
SFILES	:=	$(wildcard $(SDIR)/*.s)
SFILES2	:=	$(wildcard $(SDIR2)/*.s)
OBJS	:=	$(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) $(patsubst $(SDIR2)/%.c, $(ODIR)/%.o, $(CFILES2)) $(patsubst $(SDIR)/%.s, $(ODIR)/%.o, $(SFILES)) $(patsubst $(SDIR2)/%.s, $(ODIR)/%.o, $(SFILES2))

LIBS	:=	-lPS4

TARGET = $(shell basename $(CURDIR)).bin

$(TARGET): $(ODIR) $(OBJS)
	$(CC) $(LIBPS4)/crt0.s $(ODIR)/*.o -o temp.t $(CFLAGS) $(LFLAGS) $(LIBS)
	$(OBJCOPY) -O binary temp.t $(TARGET)
#	rm -f temp.t

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(SDIR2)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.s
	$(AS) -c -o $@ $< $(SFLAGS)

$(ODIR):
	@mkdir $@

.PHONY: clean

clean:
	rm -f $(TARGET) $(ODIR)/*.o
