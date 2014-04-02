#
# Makefile for EasyPay Test module
#

CC     = gcc
CFLAGS = -g -Wall -Wstrict-prototypes -ansi -pedantic
ODIR   = obj

_OBJS = aes.o des.o queue.o serial.o rand.o mifare_crypto.o mifare_key.o \
	mifare_aid.o mifare.o \
	test_general.o test_aes.o test_des.o test_queue.o \
	test_mifare_desfire_aes.o \
	test_mifare_desfire_des.o test_mifare_desfire_key.o test_mifare_aid.o \
	test_mifare_crypto.o test_main.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

SRC = ../
MIFARE_SRC = ../mifare/

test_main: $(OBJS)
	$(CC) $(OBJS) -o test_main

$(ODIR)/aes.o: $(MIFARE_SRC)aes.c $(MIFARE_SRC)aes.h
	$(CC) $(CFLAGS) -c -o $@ $(MIFARE_SRC)aes.c

$(ODIR)/des.o: $(MIFARE_SRC)des.c $(MIFARE_SRC)des.h $(MIFARE_SRC)spr.h
	$(CC) $(CFLAGS) -c -o $@ $(MIFARE_SRC)des.c

$(ODIR)/queue.o: $(SRC)queue.c $(SRC)queue.h
	$(CC) $(CFLAGS) -c -o $@ $(SRC)queue.c

$(ODIR)/serial.o: serial_dummy.c $(SRC)serial.h
	$(CC) $(CFLAGS) -c -o $@ serial_dummy.c

$(ODIR)/rand.o: $(MIFARE_SRC)rand.c $(MIFARE_SRC)rand.h
	$(CC) $(CFLAGS) -c -o $@ $(MIFARE_SRC)rand.c

$(ODIR)/mifare_crypto.o: $(MIFARE_SRC)mifare_crypto.c $(MIFARE_SRC)mifare_crypto.h $(MIFARE_SRC)mifare.h $(MIFARE_SRC)des.h $(MIFARE_SRC)aes.h
	$(CC) $(CFLAGS) -c -o $@ $(MIFARE_SRC)mifare_crypto.c

$(ODIR)/mifare_key.o: $(MIFARE_SRC)mifare_key.c $(MIFARE_SRC)mifare_key.h $(MIFARE_SRC)des.h
	$(CC) $(CFLAGS) -c -o $@ $(MIFARE_SRC)mifare_key.c

$(ODIR)/mifare_aid.o: $(MIFARE_SRC)mifare_aid.c $(MIFARE_SRC)mifare_aid.h
	$(CC) $(CFLAGS) -c -o $@ $(MIFARE_SRC)mifare_aid.c

$(ODIR)/mifare.o: $(MIFARE_SRC)mifare.c $(MIFARE_SRC)mifare.h
	$(CC) $(CFLAGS) -c -o $@ $(MIFARE_SRC)mifare.c

$(ODIR)/test_general.o: test_general.c test_general.h
	$(CC) $(CFLAGS) -c -o $@ test_general.c

$(ODIR)/test_aes.o: test_aes.c test_general.h $(MIFARE_SRC)aes.h
	$(CC) $(CFLAGS) -c -o $@ test_aes.c

$(ODIR)/test_des.o: test_des.c test_general.h $(MIFARE_SRC)des.h
	$(CC) $(CFLAGS) -c -o $@ test_des.c

$(ODIR)/test_queue.o: test_queue.c test_general.h $(SRC)queue.h $(SRC)general.h
	$(CC) $(CFLAGS) -c -o $@ test_queue.c

$(ODIR)/test_mifare_desfire_aes.o: test_mifare_desfire_aes.c test_general.h $(MIFARE_SRC)mifare.h $(MIFARE_SRC)mifare_crypto.h $(MIFARE_SRC)mifare_key.h
	$(CC) $(CFLAGS) -c -o $@ test_mifare_desfire_aes.c

$(ODIR)/test_mifare_desfire_des.o: test_mifare_desfire_des.c test_general.h $(MIFARE_SRC)mifare.h $(MIFARE_SRC)mifare_crypto.h $(MIFARE_SRC)mifare_key.h
	$(CC) $(CFLAGS) -c -o $@ test_mifare_desfire_des.c

$(ODIR)/test_mifare_desfire_key.o: test_mifare_desfire_key.c test_general.h $(MIFARE_SRC)mifare.h $(MIFARE_SRC)mifare_crypto.h $(MIFARE_SRC)mifare_key.h
	$(CC) $(CFLAGS) -c -o $@ test_mifare_desfire_key.c

$(ODIR)/test_mifare_aid.o: test_mifare_aid.c test_general.h $(MIFARE_SRC)mifare.h $(MIFARE_SRC)mifare_aid.h
	$(CC) $(CFLAGS) -c -o $@ test_mifare_aid.c

$(ODIR)/test_mifare_crypto.o: test_mifare_crypto.c test_general.h $(MIFARE_SRC)mifare.h $(MIFARE_SRC)mifare_crypto.h
	$(CC) $(CFLAGS) -c -o $@ test_mifare_crypto.c

$(ODIR)/test_main.o: test_main.c test_general.h test_main.h
	$(CC) $(CFLAGS) -c -o $@ test_main.c

test:
	./test_main

clean:
	rm -f $(ODIR)/*.o test_main



