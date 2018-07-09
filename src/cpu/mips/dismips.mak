dismips.exe: dismips.c mipsdasm.c
	gcc -O3 -I../../windows dismips.c -odismips
