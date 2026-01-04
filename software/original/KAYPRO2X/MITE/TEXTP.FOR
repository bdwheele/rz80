C
C	TEXTP - The Text Protocol
C
C	   This program is designed to run on a mainframe or minicomputer
C	and interact with the TEXT protocol in the MITE data communications
C	package.  The protocol allows verified transmission of data into
C	environments that ordinarily are unable to perform verified transfers
C	of information.
C
C	   Because of the extreme variety of systems on which this program
C	may be implemented, this program is only a starting point.  The
C	checksum routine will need extensive modification if the mainframe
C	is not using ASCII representations. The input/output will need to be
C	modified in most instances.  What this program is intended to be is
C	complete documentation for the mainframe side of the protocol and an
C	aid in implementing a version in a specific environment.
C
C	   This version of TEXTP is written in Microsoft Fortran under CP/M-80.
C
C	Developed October 1983 by:
C
C	   Donald Waldo
C	   Mycroft Labs, Inc.
C	   P.O. 6045
C	   Tallahassee, FL
C	   32314
C
	byte ifn(11)
 5	write(1,10)
 10	format(' Send (1) or Receive (2)?')
	read(1,15)iopt
 15	format(i1)
	if((iopt.ne.1).and.(iopt.ne.2)) goto 5
	write(1,20)
 20	format(' Enter Filename')
	read(1,25)ifn
 25	format(11a1)
	call open(5,ifn,0)
	if(iopt.eq.1)call send
	if(iopt.eq.2)call recv
	endfile 5
	goto 5
	end
C
C
C
C
C	Text file protocol - Receive
C
C	This subroutine receives a file using the TEXT
C	file protocol.
C
	subroutine recv
	integer*4 n,ick1,ick2,icksum,iconv,ick3
	byte line(150)
	data iE/69/  iA/65/   iN/78/    iX/88/
C
C	Initialize expected message number
C
	msgnum=0
C
C	Main loop - read lines and verify them.
C	Each line transmitted by MITE starts with a 'D'.  This is followed
C	by a 1 digit message number which helps improve error detection.
C	Following the message number is a 3 digit count of the number of
C	data characters on the line.  This is followed by a 5 digit (16 bit)
C	checksum.  The checksum is a 16 bit sum of all characters on
C	the line not including the checksum itself.  It is computed
C	by summing the ASCII representation of the characters and
C	ignoring overflow.  The checksum is followed by the data characters.
C
 30	read(1,12)line
 12	format(150a1)
C
C	All done if 1st char = 'E'
C
	if(line(1).eq.iE) goto 900
C
C	Aborted if 1st char = 'X'
C
	if(line(1).eq.iX) goto 900
C
C	Get no. chars on line
C
	n=iconv(line,3,3)
	if(n.lt.0) goto 40
C
C	compute checksum and verify against received value
C
	ick1=iconv(line,6,5)
	if(ick1.lt.0) goto 40
	ick2=icksum(line,1,5)
	j=n
	ick3=icksum(line,11,j)
	ick2=ick2+ick3
	if(ick2.ne.ick1) goto 40
C
C	Verify message number
C
	ick3=iconv(line,2,1)
	msg=ick3
	if(msg.eq.msgnum)goto 34
	if(msg.eq.msgnum-1)goto 36
	if((msgnum.eq.0).and.(msg.eq.9)) goto 36
	goto 50
C
C	Line is good.  Write to disk and acknowlege.
C
 34	k=n+11
	write(5,35)(line(i),i=11,k)
 35	format(150a1)
	msgnum=msgnum+1
	if(msgnum.eq.10)msgnum=0
 36	write(1,37)
 37	format(3h RA/)
	goto 30
C
C	Something went wrong.  Request that the line be retransmitted.
 40	write(1,42)
 42	format(3h RN/)
	goto 30
C
C	Error - abort transmission
C
 50	write(1,52)
 52	format(3h RX/)
C
C	Transmission complete.
C
 900	return
	end
C
C
C
C
C
	subroutine send
C
C	Text file protocol send.
C
C	The data lines sent to MITE follow the same format described above.
C
	integer*4 icnt,ichk,iconv,icksum
	byte line(80),line2(80)
	data izero/48/  iblank/32/  iA/65/  iD/68/  iN/78/  iX/88/
C
C	Wait for OK from micro before sending first line.
C
	read(1,20)line
	msgnum=-1
C
C	Get line from file
C
 15	read(5,20,end=50)line
 20	format(80a1)
C
C	Increment message number, but keep in range 0-9
C
	msgnum=msgnum+1
	if(msgnum.eq.10)msgnum=0
C
C	determine count
C
	i=80
 24	if((line(i).ne.iblank).or.(i.eq.0)) goto 30
	i=i-1
	goto 24
 30	icnt=i
C
C	Blank-fill output buffer
C
	do 32 i=1,80
 32	line2(i)=iblank
C
C	compute checksum
C
	line2(1)=iD
	line2(2)=msgnum+izero
	call putnum(icnt,line2,3,3)
	j=10+icnt
	do 35 i=11,j
 35	line2(i)=line(i-10)
	j=icnt+5
	ichk=icksum(line2,1,5)
	ichk=ichk+icksum(line2,11,icnt)
	call putnum(ichk,line2,6,5)
 40	write(1,45)line2
 45	format(1X,80a1/)
C
C	Get ack or nak
C
	read(1,20)line
	if(line(2).eq.iN) goto 40
	if(line(2).eq.iX) goto 60
	if(line(2).ne.iA) goto 60
	goto 15
 50	write(1,55)
 55	format(2h E)
	return
 60	write(1,65)
 65	format(2h X)
	return
	end
C
C
C
	subroutine putnum(num,ibuf,ist,ilen)
	integer*4 ipwr,num,itemp
	byte ibuf(150)
	data izero/48/
	iptr=ist
	itemp=num
	ipwr=1
	j=ilen-1
	do 5 i=1,j
 5	ipwr=ipwr*10
 10	j=itemp/ipwr
	ibuf(iptr)=j+izero
	itemp=itemp-j*ipwr
	ipwr=ipwr/10
	iptr=iptr+1
	if(iptr.lt.ist+ilen)goto 10
	return
	end
C
C
C
	integer*4 function icksum(ibuf,ist,len)
	byte ibuf(150)
	integer*4 k,maxint
C
C	This function computes a checksum as described above.
C
	maxint=2**16-1
	k=0
	j=ist+len-1
	do 10 i=ist,j
	k=k+ibuf(i)
	if(k.gt.maxint)k=k-maxint-1
 10	continue
	icksum=k
	return
	end
C
C
C
	integer*4 function iconv(ibuf,ist,len)
	byte ibuf(150)
	data izero/48/  iblank/32/
	ipw=10**(len-1)
	k=0
	j=ist+len-1
	do 10 i=ist,j
	ich=ibuf(i)
C
C	Convert blanks to zeros
C
	if(ich.eq.iblank)ich=izero
	n=ich-izero
	if((n.lt.0).or.(n.gt.9)) goto 99
	k=k+ipw*n
	ipw=ipw/10
 10	continue
	iconv=k
	return
 99	iconv=-1
	return
	end
