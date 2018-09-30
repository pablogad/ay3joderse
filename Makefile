OBJ=vgm_170.o ay-3-8910.o gen_pwm.o
LIB=-lz -lbcm2835

.c.o:
	gcc -g -c $<

vgmplay: ${OBJ} ay-3-8910.h 
	gcc -g -o $@ ${OBJ} ${LIB}
