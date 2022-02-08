GCC = gcc

CSRC = $(wildcard *.c)
ALLHEADERS = $(wildcard *.h)

COBJ = $(patsubst %.c, %.o, $(CSRC))
TARGET = demo

$(TARGET) : $(COBJ)
	$(GCC) -std=c99 -O0 -lm -Wall -g -o $@ $^

%.o : %.c
	$(GCC) -std=c99 -O0 -lm -Wall -g -c -o $@ $^

.PHONY : clean 
clean :
	-rm $(COBJ) $(TARGET) 


