#Compile the launch.c by linking it with the lib_simpleloader.so
all:
	gcc -m32 -o launch launch.c -I../loader -L../bin -l_simpleloader

#Provide the command for cleanup
clean:
	-@rm -f launch
