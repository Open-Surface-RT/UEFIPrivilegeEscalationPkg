.equ locked, 1
.equ unlocked, 0

.text

.global lock_mutex
.type lock_mutex, function

lock_mutex:
push {lr}
LDR     r1, =locked
loop:
	LDREX   r2, [r0]
	CMP     r2, r1        
	BEQ     loop      
	STREXNE r2, r1, [r0]  
	CMPNE   r2, #1        
	BEQ     loop         
	# Lock acquired
	DMB                   
pop {pc}
    

.global unlock_mutex
.type unlock_mutex, function

unlock_mutex:
push {lr}
	LDR     r1, =unlocked
	DMB                   
	STR     r1, [r0]      
pop {pc}
