// -*- mode: asm -*-
	
/*
	Monitor the CLK/DAT pair from the syncbox.  Decode the sync
	word.  Timestamp that.  Runs on PRU0 or PRU1.
 */

.origin 0

#include "hdr.hp"
#include "pru.hp"
#include "mem_map.hp"
	
/* Code-wide reserved registers. */
	
#define R_PRU 	R28    	// Will point to PRU_CTRL.
#define R_MAIN  R29     // Location of MainMem.

	
/* Macros for loading or setting PRU ram, according to a user-defined
	structure. */
	
#define MEMNAME MainMem
	
.macro ld_mem
.mparam dest_reg,src
.mparam count=4
	lbbo	dest_reg,R_MAIN,OFFSET(MEMNAME.src),count
.endm

.macro st_mem
.mparam src_reg,dest
.mparam count=4
	sbbo	src_reg,R_MAIN,OFFSET(MEMNAME.dest),count
.endm
	
/* flag_host
	
   Write a value (from a register) to [signal] and then raise the
   host interrupt. */

.macro flag_host
.mparam flag_val
	st32 flag_val,R_MAIN,OFFSET(MEMNAME.signal)
	mov r31.b0, THIS_PRU_SIGNAL+16
.endm

	
/* Main. */
	
start:	

	// Enable the OCP master port
	LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr
	
	mov	R_PRU,THIS_PRU_CTRL
	mov	R_MAIN,MAINMEM_ADDR

	//Enable cycle counter.
	ld32	r1,R_PRU,CONTROL
	or	r1,r1,0x0008	;COUNTER_ENABLE
	st32	r1,R_PRU,CONTROL
	
/* The Main Loop */
	
        mov     r9,10
        st_mem  r9,counter
        
loop_top:
        ld_mem  r9,exit_request
        qbne    exit,r9,0       ; If the user ever sets exit_request != 0, exit.
	
	;; If target=0, just keep waiting.
        ld_mem  r10,target
        qbeq    loop_top,r10,0
        
        ;; Check counter to see if it exceeds target.
        ld_mem  r11,counter
        qbge    signal_target_reached,r10,r11 ; branch if r11 (counter) >= r10 (target)
	
	;; Otherwise, increment counter and loop.
        add     r11,r11,1
        st_mem  r11,counter     ; Store r11 -> counter.
        
        jmp     loop_top
	
	
signal_target_reached:
        mov     r0,0            ; Set target = 0.
        st_mem  r0,target
	
        mov     r0,1            ; Signal host with signal = 1.
        flag_host r0
	
        jmp loop_top            ; Back to main loop (to wait for target != 0).
	

exit:	
	// Send notification to host for program completion
	mov r0,2
	flag_host r0
	HALT
	
