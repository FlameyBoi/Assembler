; file ps.as

.entry LENGTH
.extern W
MAIN:	mov r8 ,LEN
LOOP:	jmp L1(#-1,r6)
	prn #-5
	bne W(r4,r5)
	sub r1,r4
	bne L3
L1:	inc K
	inc 1
	inc K,K
	sub a,b,c
.entry LOOP
bne	LOOP(K,W)
END:	stop
STR:	.string "abcdef"
LENGTH:	.data -8192,9999,15
K:	.deta 22
;.extern L3
