OBJECTS = main.o kbd.o command.o display.o \
          insert.o search.o files.o utils.o
     
edit : $(OBJECTS)
	cc -o edit $(OBJECTS)
     
main.o    : defs.h
files.o   : defs.h buffer.h command.h
command.o : defs.h command.h
kbd.o     : defs.h command.h
display.o : defs.h buffer.h
insert.o  : defs.h buffer.h
search.o  : defs.h buffer.h
utils.o   : defs.h
clean     :
	rm edit $(OBJECTS)
