#Compile the launch.c by linking it with the lib_simpleloader.so
all:
	gcc -m32 -I../loader -o ../bin/launch launch.c -L../bin -l_simpleloader -Wl,-rpath='$$ORIGIN'

#Provide the command for cleanup
clean:
	-@rm -f ../bin/launch
