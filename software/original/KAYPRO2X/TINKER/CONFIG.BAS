REM ************************************************************************
REM *									   *
REM *  KAYPRO 10/4 CONFIGURATION PROGRAM  - 01/24/84 - BY ROBERT SCHRADER  *
REM *									   *
REM ************************************************************************

REM	*********************************************************
REM	*	Copyright (C) 1984 by Non-Linear Systems, Inc   *
REM	*	No warranty is made, expressed, or implied.	*
REM	*********************************************************

REM ********   THIS IS VERSION 3.  IT WORKS ON CP/M 2.2B OR LATER.  ********

REM ************************************************************************
REM *									   *
REM *	Prior to compiling, one line must be deleted from this program.    *
REM *	There are four possible choices: each begins with the keyword      *
REM *	'COMMENT'.  The result will be one of four objects: config for     *
REM *	the domestic 10, the foreign 10, the domestic 4, or the foreign 4. *
REM *									   *
REM ************************************************************************

VAR	RESPONSE;	User's main menu response.   
	LF;		Line feed (chr(10)).
	ESC;		Escape key (chr(27)).
	RTN;		Return.
	INVERSE;	Video attributes.
	REDUCED;	
	CURSOR;
	 = CHAR

VAR	HIGH;		Possible light intensities.
	LOW;
	DMA_1;		Disk Memory Access address.
	DMA_2;
	TRUE;
	FALSE;
	= INTEGER

VAR	IOB;		Pointers to locations in SECTOR_IMAGE.
	WSF;		
	N_SEQ;
	V_SEQ;
	FIRST_VEC;
	LAST_VEC;
	FIRST_KEY;
	LAST_KEY;
	MBR;
	PBR;
	SEQ_OFFSET;
	 = INTEGER

VAR	OLD_V;		Vertical and horizontal locations of hex
	OLD_H;		   values in the vecpad and keypad pictures.
	NEW_V;
	NEW_H;
	READ_OFF; 	Bios vector table offsets.
	WRITE_OFF;
	DSK_OFF;
	TRK_OFF;
	SEC_OFF;
	DMA_OFF;
	 = INTEGER

VAR	WSF_ON_MASK;	Masks to set and reset the low bit of the
	WSF_OFF_MASK;	byte that holds the write safe flag.
	IOB_MAX;	Maximum number of IO byte alterations.
	FIRST_SECTOR;
	SECOND_SECTOR;
	TRACK; 
	 = INTEGER

VAR	COUNTRY_STRING;	Foreign or domestic
	MACHINE_STRING;	10 or 4
	VERSION_STRING;	
	 = STRING

BASED	W_BOOT = INTEGER 

DIM CHAR SECTOR_IMAGE(256)

PROCEDURE INIT
	FALSE = 0
	TRUE = NOT FALSE

	REM First element in memory is a jump to warm boot instruction.
	BASE W_BOOT AT 1

	REM Offsets in the bios vector table.
	DSK_OFF = 0018H
	TRK_OFF = 001BH
	SEC_OFF = 001EH
	DMA_OFF = 0021H
	READ_OFF = 0024H
	WRITE_OFF = 0027H

	REM DMA will point to the sector image.
	LOCATION ARRAY DMA_1 = SECTOR_IMAGE 
	DMA_2 = DMA_1 + 128

	ESC = CHR(27)
	LF = CHR(10)
	RTN = CHR(13)

	OLD_V = 16
	OLD_H = 60
	NEW_V = 18
	NEW_H = 60

	HIGH = TRUE
	LOW = FALSE

	INVERSE = '0'
	REDUCED = '1'
	CURSOR = '4'

	IOB = 51
	
	WSF = 52

	FIRST_VEC = 53
	LAST_VEC = 56

	FIRST_KEY = 57
	LAST_KEY = 70

	MBR = 71
	PBR = 72

	SEQ_OFFSET = 73

	WSF_ON_MASK = 0001H
	WSF_OFF_MASK = 00FEH
END

COMMENT Delete this line before compiling if you want a domestic 10 version
PROCEDURE VERSION_SELECT
	COUNTRY_STRING = "Domestic"
	MACHINE_STRING = "10"
	VERSION_STRING = "Version 3"
	IOB_MAX = 5
	FIRST_SECTOR = 45
	SECOND_SECTOR = 46
	TRACK = 0
END

COMMENT Delete this line before compiling if you want a foreign 10 version
PROCEDURE VERSION_SELECT
	COUNTRY_STRING = "Foreign"
	MACHINE_STRING = "10"
	VERSION_STRING = "Version 3"
	IOB_MAX = 6
	FIRST_SECTOR = 45
	SECOND_SECTOR = 46
	TRACK = 0
END

COMMENT Delete this line before compiling if you want a domestic 4 version
PROCEDURE VERSION_SELECT
	COUNTRY_STRING = "Domestic"
	MACHINE_STRING = "4"
	VERSION_STRING = "Version 3"
	IOB_MAX = 5
	FIRST_SECTOR = 21
	SECOND_SECTOR = 22
	TRACK = 1
END

COMMENT Delete this line before compiling if you want a foreign 4 version
PROCEDURE VERSION_SELECT
	COUNTRY_STRING = "Foreign"
	MACHINE_STRING = "4"
	VERSION_STRING = "Version 3"
	IOB_MAX = 6
	FIRST_SECTOR = 21
	SECOND_SECTOR = 22
	TRACK = 1
END

PROCEDURE CPM_VERSION_INIT
	REM Now that the sector image has been read in,
	REM adjust the CP/M-version-dependant 'constants'.
	V_SEQ = FIRST_VEC + SECTOR_IMAGE[SEQ_OFFSET]
	N_SEQ = V_SEQ + 16
END

PROCEDURE SET_ON (ATT=CHAR)
	PRINT ESC;'B'; ATT;
END


PROCEDURE SET_OFF (ATT=CHAR)
	PRINT ESC;'C'; ATT;
END


PROCEDURE POSITION (VERT, HORZ = CHAR)
	PRINT ESC; '=' ; VERT+31; HORZ+31;
END


PROCEDURE CLEAR
	PRINT CHR(26)
END


PROCEDURE PAUSE
VAR X = CHAR
	INPUT2 "Hit any key to continue";X
END


PROCEDURE SELECT_DISK (I = INTEGER)
VAR HL,DE,BC,AF = INTEGER
	BC = I
	REPEAT CALL(W_BOOT+DSK_OFF, HL, DE, BC, AF)
	UNTIL (HL <> 0)
END


PROCEDURE SELECT_TRACK (I = INTEGER)
VAR HL,DE,BC,AF = INTEGER
	BC = I 
	CALL(W_BOOT+TRK_OFF, HL, DE, BC, AF)
END


PROCEDURE SELECT_SECTOR (I = INTEGER)
VAR HL,DE,BC,AF = INTEGER
	BC = I 
	CALL(W_BOOT+SEC_OFF, HL, DE, BC, AF)
END


PROCEDURE DEFINE_DMA (I = INTEGER)
VAR HL,DE,BC,AF = INTEGER
	BC = I 
	CALL(W_BOOT+DMA_OFF, HL, DE, BC, AF)
END


PROCEDURE READ_SECTOR
VAR HL,DE,BC,AF = INTEGER
	CALL(W_BOOT+READ_OFF, HL, DE, BC, AF)
END


PROCEDURE WRITE_SECTOR
VAR HL,DE,BC,AF = INTEGER
	CALL(W_BOOT+WRITE_OFF, HL, DE, BC, AF)
END


PROCEDURE PRINT_MENU
CLEAR
PRINT
PRINT
PRINT"	K A Y P R O  ";MACHINE_STRING;
PRINT"  C O N F I G U R A T I O N   P R O G R A M"
PRINT
PRINT"			";VERSION_STRING;
TEXT 0,$


        	                              Help    Function

	Change IOBYTE .........................i	I
	Redefine the vector pad ...............v	V
	Redefine the number pad ...............n	N
	Set the write safe flag ...............w	W
	Change the printer baud rate ..........p	P
	Change the modem baud rate ............m	M
	Exit the CONFIG program ...............x	X


Please enter your selection ===>$;
END


PROCEDURE H_IOBYTE
CLEAR
TEXT 0,$


In order to understand this command you should read the CP/M manual
"AN INTRODUCTION TO CP/M FEATURES AND FACILITIES", in particular the
section on the STAT command and logical and physical devices.  The
logical CP/M devices are CON:, LST:, RDR:, and PUN:.
The physical devices are:

	CRT: -- Video and keyboard.
	TTY: -- Serial port. (See manual for connector wiring)
	LPT: -- Centronics port.
$;

IF ( COUNTRY_STRING = "Foreign" )\
	THEN PRINT" 	UL1: -- User-defined list device."
	ELSE PRINT"	UL1: -- Same as TTY: above."

TEXT 0,$

Possible logical to physical assignments that are meaningful are:

	CON: = TTY: or CRT:
	RDR: = TTY:
	PUN: = TTY:
$;

IF ( COUNTRY_STRING = "Foreign" )\
	THEN PRINT "	LST: = TTY: or LPT: or CRT: or UL1:"
	ELSE PRINT "	LST: = TTY: or LPT: or CRT:"
PRINT
PRINT "BEFORE using this option, try it with the STAT command."
PRINT

PAUSE
END


PROCEDURE H_WRITE_SAFE
CLEAR
TEXT 0,$


Your KAYPRO computer comes with a special "Write Safe" option that corrects
an incompatibility between CP/M and some application programs. The following
is rather technicaly involved, so if you do not understand it, ask your
dealer.  The incorrect setting of the write flag may cause IRREVOCABLE LOSS 
of data or programs.  When in doubt, leave it off.

The KAYPRO computer uses deblocking.  When a disk operation is
immediately followed by a warm boot, the deblocking buffer may not be  
written to the disk.  However, if the write safe flag is set, the BIOS
thinks that all disk operations are of the directory type. All directory
operations force the deblocking buffer to the disk on a write.  There 
will be a 2- to 4-fold decrease in speed when writing to the disk.

$
PAUSE 
END


PROCEDURE H_VECPAD
CLEAR
TEXT 0,$


	 Your KAYPRO computer has four keys with arrows on them, at
the upper right of your keyboard.  They are refered to as cursor keys
or vector keys.  They are used in programs such as word processors to
move the cursor. They may be used in other ways by other programs, and
can be modified accordingly. The modification consists of telling the 
BIOS to send a different 1 to 4 byte code to the program when the key 
is hit. The 'V' option allows the user to change these codes on the disk.
The next time that the computer is cold booted (reset) from the modified
disk, these changes will be effected.  

$
PAUSE
END


PROCEDURE H_NUMPAD
CLEAR
TEXT 0,$


	On the far right side of your keyboard is a set of 14 keys,
arranged in a square.  Each of these keys produces a 1 to 4 byte code 
when hit. The codes produced by these keys can be changed with the 'N'
option.   The next time that the computer is cold booted (reset) from 
the modified disk, these changes will be effected.  


$
PAUSE
END



PROCEDURE H_PRINTER_BAUD
CLEAR
TEXT 0,$


	The KAYPRO can be connected to a printer if it is told to
send information at the same rate as the printer that is expecting to
receive it. This rate of information transfer is refered to as the 
BAUD rate.

	The KAYPRO is capable of the following rates:

	  110
	  300 (Default)
	 1200
	 2400
	 4800
	 9600
	19200

$
PAUSE
END


PROCEDURE H_MODEM_BAUD
CLEAR
TEXT 0,$


	The KAYPRO can be connected to a telephone if it is told to
send and receive information at the same rate as the device on the 
other end of the line is expecting to send or receive it. This rate of
information transfer is refered to as the BAUD rate.

	The KAYPRO is capable of the following rates:

	  110
	  300 (Default)
	 1200
	 2400
	 4800
	 9600
	19200

$
PAUSE
END


PROCEDURE SET_LIGHT(L, K=INTEGER)
	IF L THEN SET_ON INVERSE
	IF L THEN SET_ON REDUCED

	CASE K OF
	    53: BEGIN
		POSITION 6,2
		PRINT "  /|\  "
		POSITION 7,2
		PRINT "   |   "
		POSITION 8,2
		PRINT "   |   "
		END
	    54: BEGIN
		POSITION 6,10
		PRINT "   |   "
		POSITION 7,10
		PRINT "   |   "
		POSITION 8,10
		PRINT "  \|/  "
		END
	    55: BEGIN
		POSITION 6,18
		PRINT "       "
		POSITION 7,18
		PRINT "  <--  "
		POSITION 8,18
		PRINT "       "
		END
	    56: BEGIN
		POSITION 6,26
		PRINT "       "
		POSITION 7,26
		PRINT "  -->  "
		POSITION 8,26
		PRINT "       "
		END
	    57: BEGIN
		POSITION 18,2 
		PRINT "               "
		POSITION 19,2
		PRINT "       0       "
		POSITION 20,2
		PRINT "               "
		END
	    58: BEGIN
		POSITION 14,2
		PRINT "       "
		POSITION 15,2
		PRINT "   1   "
		POSITION 16,2
		PRINT "       "
		END
	    59: BEGIN
		POSITION 14,10
		PRINT "       "
		POSITION 15,10
		PRINT "   2   "
		POSITION 16,10
		PRINT "       "
		END
	    60: BEGIN
		POSITION 14,18
		PRINT "       "
		POSITION 15,18
		PRINT "   3   "
		POSITION 16,18
		PRINT "       "
		END
	    61: BEGIN
		POSITION 10,2
		PRINT "       "
		POSITION 11,2
		PRINT "   4   "
		POSITION 12,2
		PRINT "       "
		END
	    62: BEGIN
		POSITION 10,10
		PRINT "       "
		POSITION 11,10
		PRINT "   5   "
		POSITION 12,10
		PRINT "       "
		END
	    63: BEGIN
		POSITION 10,18
		PRINT "       "
		POSITION 11,18
		PRINT "   6   "
		POSITION 12,18
		PRINT "       "
		END
	    64: BEGIN
		POSITION 6,2
		PRINT "       "
		POSITION 7,2
		PRINT "   7   "
		POSITION 8,2
		PRINT "       "
		END
	    65: BEGIN
		POSITION 6,10
		PRINT "       "
		POSITION 7,10
		PRINT "   8   "
		POSITION 8,10
		PRINT "       "
		END
	    66: BEGIN
		POSITION 6,18
		PRINT "       "
		POSITION 7,18
		PRINT "   9   "
		POSITION 8,18
		PRINT "       "
		END
	    67: BEGIN
		POSITION 6,26
		PRINT "       "
		POSITION 7,26
		PRINT "   -   "
		POSITION 8,26
		PRINT "       "
		END
	    68: BEGIN
		POSITION 10,26
		PRINT "       "
		POSITION 11,26
		PRINT "   ,   "
		POSITION 12,26
		PRINT "       "
		END
	    69: BEGIN
		POSITION 14,26
		PRINT "       "
		POSITION 15,26
		PRINT "   E   "
		POSITION 16,26
		PRINT "   N   "
		POSITION 17,26
		PRINT "   T   "
		POSITION 18,26
		PRINT "   E   "
		POSITION 19,26
		PRINT "   R   "
		POSITION 20,26
		PRINT "       "
		END
	    70: BEGIN
		POSITION 18,18
		PRINT "       "
		POSITION 19,18
		PRINT "   .   "
		POSITION 20,18
		PRINT "       "
		END
	END
	SET_OFF INVERSE
	SET_OFF REDUCED
END


PROCEDURE C_IOBYTE
VAR C = CHAR
VAR A, CON, LST = INTEGER
	IF ASC(SECTOR_IMAGE[IOB]) >191 THEN LST = 192
	ELSE IF ASC(SECTOR_IMAGE[IOB]) >127 THEN LST = 128
		ELSE IF ASC(SECTOR_IMAGE[IOB]) >63 THEN LST = 64
			ELSE LST = 0
	CON = ASC(SECTOR_IMAGE[IOB]) - LST
	A = 1
	CLEAR
	ECHO OFF
	SET_OFF CURSOR
	TEXT 0,$


		I O B Y T E    M O D I F I C A T I O N
		======================================



    DEFAULT SETTINGS 	POSSIBLE CHANGES	Type a line feed to choose  
						the assignment that you
      CON: = CRT:	  CON: = TTY:		want. After each line feed,
      RDR: = TTY:	   - NONE -		another assignment will be
      PUN: = TTY:	   - NONE -		highlighted.  Hit a return 
      LST: = LPT:	  LST: = TTY:		and that assignment will  
			  LST: = CRT:		selected.  Hit an 'X' to
						return to the main menu.
					
	


				

	$
	IF ( COUNTRY_STRING = "Foreign" ) THEN BEGIN
		POSITION 16,27
		PRINT "LST: = UL1:";
		END

	SET_ON INVERSE
	SET_ON REDUCED
	 
	POSITION 11,7
	PRINT "CON: = CRT:";

	SET_OFF INVERSE
	SET_OFF REDUCED

	REPEAT BEGIN
		INPUT3 C
		CASE C OF 
		    LF: BEGIN
			CASE A OF 
			    1: BEGIN
				POSITION 11,7
				PRINT "CON: = CRT:";
				END
			    2: BEGIN
				POSITION 11,27
				PRINT "CON: = TTY:";
				END
			    3: BEGIN
				POSITION 14,7
				PRINT "LST: = LPT:";
				END
			    4: BEGIN
				POSITION 14,27
				PRINT "LST: = TTY:";
				END
			    5: BEGIN
				POSITION 15,27
				PRINT "LST: = CRT:";
				END
			    6: BEGIN
				POSITION 16,27
				PRINT "LST: = UL1:";
				END
			END

			IF A=IOB_MAX THEN A=1 ELSE A=A+1

			SET_ON INVERSE
			SET_ON REDUCED
			CASE A OF 
			    1: BEGIN
				POSITION 11,7
				PRINT "CON: = CRT:";
				END
			    2: BEGIN
				POSITION 11,27
				PRINT "CON: = TTY:";
				END
			    3: BEGIN
				POSITION 14,7
				PRINT "LST: = LPT:";
				END
			    4: BEGIN
				POSITION 14,27
				PRINT "LST: = TTY:";
				END
			    5: BEGIN
				POSITION 15,27
				PRINT "LST: = CRT:";
				END
			    6: BEGIN
				POSITION 16,27
				PRINT "LST: = UL1:";
				END
			END
			SET_OFF REDUCED
			SET_OFF INVERSE
		    END
		    RTN: BEGIN
			CASE A OF 
			    1: CON = 1
			    2: CON = 0
			    3: LST = 128
			    4: LST = 0
			    5: LST = 64
			    6: LST = 192
			END
			SECTOR_IMAGE[IOB] = CHR(CON + LST)
		    END
		END
	END
	UNTIL ((C = 'X') OR (C = 'x'))
	ECHO ON
	SET_ON CURSOR
END


PROCEDURE PRINT_KEY_NAME( C=CHAR )
	IF C = 00 THEN PRINT "ERROR";
	IF (C => 1) AND (C <= 26) THEN PRINT "CTR-";CHR(C+64);	
	IF C = 27 THEN PRINT " ESC ";
	IF C = 28 THEN PRINT " FS  ";
	IF C = 29 THEN PRINT " GS  ";
	IF C = 30 THEN PRINT " RS  ";
	IF C = 31 THEN PRINT " US  ";
	IF C = 32 THEN PRINT "SPACE";
	IF (C => 33) AND (C <= 126) THEN PRINT "  ";C;"  ";
	IF C = 127 THEN PRINT " DEL ";
	IF C => 128 THEN PRINT "ERROR";
END	

PROCEDURE PRINT_SEQUENCE (I = INTEGER)
	VAR K = INTEGER
	POSITION 16, 48
	PRINT "                          "
	POSITION 17, 48
	PRINT "                          "
	POSITION 18, 48
	PRINT "                          "
	POSITION 19, 48
	PRINT "                          "
	POSITION 20, 48
	PRINT "                          "

	IF SECTOR_IMAGE[I] <> CHR(0)THEN BEGIN
		POSITION 18, 41
		PRINT_KEY_NAME SECTOR_IMAGE[I]
		END
	    ELSE BEGIN
		I = ((I-57) * 4) + N_SEQ
		POSITION 18, 41
		PRINT_KEY_NAME SECTOR_IMAGE[I]
		K = 1
		WHILE ((SECTOR_IMAGE[I+K]<>CHR(0)) AND (K<4)) DO BEGIN
			POSITION 16,(40+K*8)
			PRINT "-------"
			POSITION 17,(40+K*8)
			PRINT "       |"
			POSITION 18,(41+K*8)
			PRINT_KEY_NAME SECTOR_IMAGE[I+K]
			PRINT " |" 
			POSITION 19,(40+K*8)
			PRINT "       |"
			POSITION 20,(40+K*8)
			PRINT "-------"
			K = K + 1
			END
		END
END

PROCEDURE C_VECPAD
ECHO OFF
VAR C = CHAR
VAR VEC, I, K = INTEGER

	CLEAR
	TEXT 0,$



 -------------------------------
|  /|\  |   |   |       |       |        Enter a line feed to choose a key
|   |   |   |   |  -->  |  <--  |   in the picture to the left.  Each line
|   |   |  \|/  |       |       |   feed will highlight another of the keys.
 -------------------------------    When the key  that you want to change is
				    highlighted, enter the number of keys  
				    (1-4) in the sequence that you want to
				    be generated when that key is pressed.
				    Then enter the values. Hit an X to exit. 

				    Keys(s) currently generated: 
				       -------
				      |       |
				      |       |
				      |       |
				       -------



$;

SET_OFF CURSOR
VEC = FIRST_VEC
SET_LIGHT HIGH,VEC
PRINT_SEQUENCE VEC

REPEAT BEGIN
	INPUT3 C
	IF C = LF THEN BEGIN
		POSITION 15, 37
		PRINT "Key(s) currently generated: "
		SET_LIGHT LOW, VEC
		IF VEC=LAST_VEC THEN VEC=FIRST_VEC ELSE VEC=VEC+1	
		SET_LIGHT HIGH,VEC
		PRINT_SEQUENCE VEC
		END
	IF ((C=>'1') AND (C<='4')) THEN BEGIN
		FOR K = 1 TO 4
			SECTOR_IMAGE[V_SEQ+(VEC-FIRST_VEC)*4+K-1] = CHR(0)
		NEXT K
		POSITION 15, 37
		PRINT "Enter new key(s):           "
		POSITION 18,41
		PRINT "     "
		POSITION 16, 48
		PRINT "                          "
		POSITION 17, 48
		PRINT "                          "
		POSITION 18, 48
		PRINT "                          "
		POSITION 19, 48
		PRINT "                          "
		POSITION 20, 48
		PRINT "                          "
		END
	IF ((C=>'2') AND (C<='4')) THEN BEGIN
		I = ASC(C) - 48
		SECTOR_IMAGE[VEC] = CHR(0)
		FOR K = 2 TO I
			POSITION 16,(32+K*8)
			PRINT "-------"
			POSITION 17,(32+K*8)
			PRINT "       |"
			POSITION 18,(32+K*8)
			PRINT "       |" 
			POSITION 19,(32+K*8)
			PRINT "       |"
			POSITION 20,(32+K*8)
			PRINT "-------"
		NEXT K
		FOR K = 1 TO I
			INPUT3 SECTOR_IMAGE[V_SEQ+(VEC-FIRST_VEC)*4+K-1]
			POSITION 18, (33+K*8)
			PRINT_KEY_NAME SECTOR_IMAGE[V_SEQ+(VEC-FIRST_VEC)*4+K-1]
		NEXT K
		END	
	IF C = '1' THEN BEGIN
		INPUT3 SECTOR_IMAGE[VEC]
		POSITION 18, 41
		PRINT_KEY_NAME SECTOR_IMAGE[VEC]
		END	
	END
UNTIL ((C = 'X') OR (C = 'x'))

SET_ON CURSOR
ECHO ON
END


PROCEDURE C_NUMPAD
ECHO OFF
VAR C = CHAR
VAR KEY, I, K = INTEGER

	CLEAR
	TEXT 0,$



 -------------------------------
|       |       |       |       |        Enter a line feed to choose a key
|   7   |   8   |   9   |   -   |   in the picture to the left.  Each line
|       |       |       |       |   feed will highlight another of the keys.
|-------+-------+-------+-------|   When the key  that you want to change is
|       |       |       |       |   highlighted, enter the number of keys  
|   4   |   5   |   6   |   ,   |   (1-4) in the sequence that you want to
|       |       |       |       |   be generated when that key is pressed.
|-------+-------+-------+-------|   Then enter the values. Hit an X to exit. 
|       |       |       |       |
|   1   |   2   |   3   |   E   |   Keys(s) currently generated: 
|       |       |       |   N   |      -------
|---------------+-------|   T   |     |       |
|               |       |   E   |     |       |
|       0       |   .   |   R   |     |       |
|               |       |       |      -------
 -------------------------------


$;

SET_OFF CURSOR
KEY = FIRST_KEY
SET_LIGHT HIGH,KEY
PRINT_SEQUENCE KEY

REPEAT BEGIN
	INPUT3 C
	IF C = LF THEN BEGIN
		POSITION 15, 37
		PRINT "Key(s) currently generated: "
		SET_LIGHT LOW, KEY
		IF KEY=LAST_KEY THEN KEY=FIRST_KEY ELSE KEY=KEY+1	
		SET_LIGHT HIGH,KEY
		PRINT_SEQUENCE KEY
		END
	IF ((C=>'1') AND (C<='4')) THEN BEGIN
		FOR K = 1 TO 4
			SECTOR_IMAGE[N_SEQ+(KEY-FIRST_KEY)*4+K-1] = CHR(0)
		NEXT K
		POSITION 15, 37
		PRINT "Enter new key(s):           "
		POSITION 18,41
		PRINT "     "
		POSITION 16, 48
		PRINT "                          "
		POSITION 17, 48
		PRINT "                          "
		POSITION 18, 48
		PRINT "                          "
		POSITION 19, 48
		PRINT "                          "
		POSITION 20, 48
		PRINT "                          "
		END
	IF ((C=>'2') AND (C<='4')) THEN BEGIN
		I = ASC(C) - 48
		SECTOR_IMAGE[KEY] = CHR(0)
		FOR K = 2 TO I
			POSITION 16,(32+K*8)
			PRINT "-------"
			POSITION 17,(32+K*8)
			PRINT "       |"
			POSITION 18,(32+K*8)
			PRINT "       |" 
			POSITION 19,(32+K*8)
			PRINT "       |"
			POSITION 20,(32+K*8)
			PRINT "-------"
		NEXT K
		FOR K = 1 TO I
			INPUT3 SECTOR_IMAGE[N_SEQ+(KEY-FIRST_KEY)*4+K-1]
			POSITION 18, (33+K*8)
			PRINT_KEY_NAME SECTOR_IMAGE[N_SEQ+(KEY-FIRST_KEY)*4+K-1]
		NEXT K
		END	
	IF C = '1' THEN BEGIN
		INPUT3 SECTOR_IMAGE[KEY]
		POSITION 18, 41
		PRINT_KEY_NAME SECTOR_IMAGE[KEY]
		END	
	END
UNTIL ((C = 'X') OR (C = 'x'))

SET_ON CURSOR
ECHO ON
END


PROCEDURE C_WRITE_SAFE
REM Wsf_dummy is used to force bit by bit logical operations.
VAR WSF_DUMMY = INTEGER 
VAR FLAG = CHAR
	CLEAR
	TEXT 0,$





	    Enter a '1' to set the write safe flag on, any
	other character to set it off.



		$;
	INPUT FLAG
	IF FLAG='1' THEN BEGIN
			WSF_DUMMY = SECTOR_IMAGE [WSF]
			WSF_DUMMY = WSF_DUMMY OR WSF_ON_MASK
			SECTOR_IMAGE [WSF] = WSF_DUMMY
			PRINT "        Write safe flag is now ON."
			END
		    ELSE BEGIN
			WSF_DUMMY = SECTOR_IMAGE [WSF]
			WSF_DUMMY = WSF_DUMMY AND WSF_OFF_MASK
			SECTOR_IMAGE [WSF] = WSF_DUMMY
			PRINT "        Write safe flag is now OFF."
			END
	PRINT
	PAUSE
END


PROCEDURE C_PRINTER_BAUD
VAR R, VALID = INTEGER
	CLEAR
	TEXT 0,$


	Possible baud rates:  110, 300, 1200, 2400, 4800, 9600, 19200.

	Enter the rate that you want for the printer.
	(Enter a '0' to return to the main menu.)$;

	REPEAT BEGIN
		VALID = TRUE
		POSITION 12,12
		PRINT "                                       ";
		POSITION 12,12
		INPUT R
		POSITION 13,1
		PRINT "                                       ";
		POSITION 13,1
		CASE R OF 
		     110: SECTOR_IMAGE [PBR] = CHR(02H)
		     300: SECTOR_IMAGE [PBR] = CHR(05H)
		    1200: SECTOR_IMAGE [PBR] = CHR(07H)
		    2400: SECTOR_IMAGE [PBR] = CHR(0AH)
		    4800: SECTOR_IMAGE [PBR] = CHR(0CH)
		    9600: SECTOR_IMAGE [PBR] = CHR(0EH)
		   19200: SECTOR_IMAGE [PBR] = CHR(0FH)
		       0: VALID = TRUE
		       R: VALID = FALSE
		END
		IF NOT VALID THEN PRINT "INVALID BAUD RATE"
			ELSE IF R<>0 THEN PRINT"Baud rate changed to ";R
		END
	UNTIL R = 0
END


PROCEDURE C_MODEM_BAUD
VAR R, VALID = INTEGER
	CLEAR
	TEXT 0,$


	Possible baud rates:  110, 300, 1200, 2400, 4800, 9600, 19200.

	Enter the rate that you want for the modem.
	(Enter a '0' to return to the main menu.)$;

	REPEAT BEGIN
		VALID = TRUE
		POSITION 12,12
		PRINT "                                       ";
		POSITION 12,12
		INPUT R
		POSITION 13,1
		PRINT "                                       ";
		POSITION 13,1
		CASE R OF 
		     110: SECTOR_IMAGE [MBR] = CHR(02H)
		     300: SECTOR_IMAGE [MBR] = CHR(05H)
		    1200: SECTOR_IMAGE [MBR] = CHR(07H)
		    2400: SECTOR_IMAGE [MBR] = CHR(0AH)
		    4800: SECTOR_IMAGE [MBR] = CHR(0CH)
		    9600: SECTOR_IMAGE [MBR] = CHR(0EH)
		   19200: SECTOR_IMAGE [MBR] = CHR(0FH)
		       0: VALID = TRUE
		       R: VALID = FALSE
		END
		IF NOT VALID THEN PRINT "INVALID BAUD RATE"
			ELSE IF R<>0 THEN PRINT"Baud rate changed to ";R
		END
	UNTIL R = 0
END


PROCEDURE XPLAIN
	CLEAR
	TEXT 0,$

	   After all changes have been entered and you wish to enact
	these changes, or if you wish to abandon the CONFIG program,
	enter an 'X', and you will be asked if you wish to implement
 	the changes.  If you answer 'y', the program will implement
 	the changes and then terminate.  If you answer 'n', then the
	program will simply terminate.



	$;

	PAUSE
END 


REM  *****************  Mainline starts here.  **********************

INIT

VERSION_SELECT

SELECT_DISK 0
SELECT_TRACK TRACK

SELECT_SECTOR FIRST_SECTOR
DEFINE_DMA DMA_1
READ_SECTOR

SELECT_SECTOR SECOND_SECTOR
DEFINE_DMA DMA_2
READ_SECTOR

CPM_VERSION_INIT

REPEAT 
	BEGIN
	PRINT_MENU
	INPUT2 RESPONSE

	CASE RESPONSE OF
		'i': H_IOBYTE
		'I': C_IOBYTE

		'v': H_VECPAD
		'V': C_VECPAD

		'n': H_NUMPAD
		'N': C_NUMPAD

		'w': H_WRITE_SAFE
		'W': C_WRITE_SAFE

		'p': H_PRINTER_BAUD
		'P': C_PRINTER_BAUD

		'm': H_MODEM_BAUD
		'M': C_MODEM_BAUD

		'x': XPLAIN
	END
	END
UNTIL (RESPONSE = 'X')

INPUT "Do you really want the requested changes";RESPONSE

IF RESPONSE THEN BEGIN
	
	SELECT_SECTOR FIRST_SECTOR
	DEFINE_DMA DMA_1
	WRITE_SECTOR

	SELECT_SECTOR SECOND_SECTOR
	DEFINE_DMA DMA_2
	WRITE_SECTOR

	REM An arbitrary read to ensure that the write is completed.
	SELECT_TRACK 6
	READ_SECTOR

	PRINT "Changes now recorded.  They will become effective"
	PRINT "after the next reset."
	END
ELSE PRINT "No changes made."

 

INIT

VERSION_SELECT

SELECT_DISK 0
SELECT_TRACK TRACK

SELECT_SEC