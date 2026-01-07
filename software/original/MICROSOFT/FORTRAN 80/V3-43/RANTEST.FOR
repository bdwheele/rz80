	CALL OPEN(8,'RFILE      ',0)
	DO1I=1,20,2
1	WRITE(8,100,REC=I)I
100	FORMAT(I20)
	DO2I=2,20,2
2	WRITE(8,100,REC=I)I
	I=1
3	READ(8,100,REC=I,END=99)J
	WRITE(3,100)J
	I=I+1
	GOTO3
99	END
