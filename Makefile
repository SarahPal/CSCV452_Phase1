TARGET=libphase1.a
ASSIGNMENT= 452phase1
CC=gcc
AR=ar
COBJS= phase1.o
CSRCS=${COBJS:.o=.c}
HDRS=kernel.h
INCLUDE = ./usloss/include

CFLAGS = -Wall -g -I${INCLUDE} -I.
UNAME := $(shell uname -s)

ifeq ($(UNAME), Darwin)
	CFLAGS += -D_XOPEN_SOURCE
endif

LDFLAGS = -L. -L./usloss/lib

TESTDIR=testcases
TESTS= test00 test01 test02 test03 test04 test05 test06 test07 test08 \
       test09 test10 test11 test12 test13 test14 test15 test16 test17 \
       test18 test19 test20 test21 test22 test23 test24 test25 test26\
       test27 test28 test29 test30 test31 test32 test33 test34 test35 test36
LIBS = -lphase1 -lusloss


$(TARGET):	$(COBJS)
#		$(AR) -r $@ $(COBJS)
		$(AR) -r $@ phase1.o

#$(TESTS):	$(TARGET) $(TESTDIR)/$@.c
$(TESTS):	$(TARGET) p1.o
	$(CC) $(CFLAGS) -c $(TESTDIR)/$@.c
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS) p1.o

$(TESTDIR)/$(TESTS).c:

clean:
	rm -f $(COBJS) $(TARGET) test?.o test??.o test? test?? \
		core term*.out p1.o
cleanAll:
	rm -f test??.c
	make clean

phase1.o:	kernel.h
