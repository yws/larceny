! Rts/Sparc/barrier.s
! Write barrier code to support new GC with the old remembered sets.
!
! $Id: barrier.s,v 1.4 1997/02/03 18:21:49 lth Exp $
!
! Write barrier.
!
! The assembler may use one of three strategies for the write barrier.
! * In-line code may be generated that calls m_full_barrier in all
!   cases.
! * In-line code may perform a test which ensures that the RHS is not 
!   a constant, and if it is not, then call m_partial_barrier to perform
!   the write barrier operation.
! * If it is known that the young area is allocated below all areas,
!   which is the case in the Sparc version, then the in-line code may
!   test that (a) the LHS is not a young object, and (b) the RHS is not
!   a constant, and if either is not true, then call m_partial_barrier
!   to perform the write barrier operation.  In-lining the generation
!   test is an optimization, since it throws out more assignments.
!
! In the third case, in-line code may use a conservative heap limit check,
! e.g., comparing against %E_LIMIT rather than globals[ G_ELIM ].  On
! systems where the stack lives in the young heap area rather than in 
! a separate cache, %E_LIMIT is not an accurate delimiter for the young
! area as continuation structures may have been flushed above this limit.
! Debuggers may change these structures and these changes need not be
! recorded because the LHS is a young object, but it's ok if the fast
! in-line barrier does not discover this.
!
! There is a particularly fast code sequence for the in-line double check,
! using only one branch and three ALU instructions; see my off-line design
! notes for the details.  --lars

#include "asmdefs.h"
#include "asmmacro.h"

! FIXME: This should be defined somewhere else.
#define PAGESHIFT 12

	.global EXTNAME(m_full_barrier)		! full write barrier
	.global EXTNAME(mem_addtrans)		! alias for full barrier
	.global EXTNAME(m_partial_barrier)	! partial write barrier

	.seg "text"


! m_full_barrier: full write barrier.
!
! Call from: Scheme
! Input:     RESULT = object: object which was assigned to.
!            ARGREG2 = object: object which was assigned.
! Output:    Nothing
! Destroys:  Temporaries

#if !SIMULATE_NEW_BARRIER
EXTNAME(m_full_barrier):
EXTNAME(mem_addtrans):
#endif

	andcc	%ARGREG2, 0x01, %g0		! constant RHS?
	be	9f				! exit if so
	nop

	/* FALL THROUGH TO THE PARTIAL BARRIER */

! m_partial_barrier: Partial write barrier.
!
! Input:    RESULT has lhs
!           ARGREG2 has rhs
! Output:   nothing
! Destroys: Temporaries
!
! Both inputs must be pointers.
!
! Algorithm:
!  /* genv is a vector that maps page number to generation number */
!  /* ssbtopv is a vector that maps generation number to ssb_top pointer */
!  /* ssblimv is a vector that maps generation number to ssb_lim pointer */
!
!  #define page(x)  (((x) - pagebase) >> PAGESHIFT)
!
!  void barrier( word lhs, word rhs )
!  {
!    unsigned *genv, gl, gr;
!    word **ssbtopv, **ssblimv;
!
!    genv = (unsigned*)globals[ G_GENV ];
!    gl = genv[page(lhs)];       /* gl: generation # of lhs */
!    gr = genv[page(rhs)];       /* gr: generation # of rhs */
!    if (gl <= gr) return;  
!  
!    ssbtopv = (word**)globals[ G_SSBTOPV ];
!    ssblimv = (word**)globals[ G_SSBLIMV ];
!    *ssbtopv[gl] = lhs;
!    ssbtopv[gl] = ssbtopv[gl]+1;
!    if (ssbtopv[gl] == ssblimv[gl]) C_wb_compact( gl );
!  }

EXTNAME(m_partial_barrier):
	ld	[%GLOBALS+G_PGBASE], %TMP2	! pagebase in %TMP2
	sub	%RESULT, %TMP2, %TMP0
	srl	%TMP0, PAGESHIFT, %TMP0		! page(RESULT) in TMP0
	sll	%TMP0, 2, %TMP0			!   shifted for indexing
	sub	%ARGREG2, %TMP2, %TMP1
	ld	[%GLOBALS+G_GENV], %TMP2	! genv in %TMP2
	srl	%TMP1, PAGESHIFT, %TMP1		! page(ARGREG2) in TMP1
	sll	%TMP1, 2, %TMP1			!   shifted for indexing
	ld	[%TMP2+%TMP0], %TMP0		! gl in %TMP0
	ld	[%TMP2+%TMP1], %TMP1		! gr in %TMP1
	! %TMP0: gl
	! %TMP1: gr
	! %TMP2: dead
	cmp	%TMP0, %TMP1
	ble	9f
	sll	%TMP0, 2, %TMP1			! gl shifted for indexing

	! Must record a transaction
	! %TMP0: dead
	! %TMP1: gl, shifted for indexing
	! %TMP2: dead
	ld	[%GLOBALS+G_SSBTOPV], %TMP0	! ssbtopv in %TMP0
	ld	[%TMP0+%TMP1], %TMP2		! ssbtopv[gl] in %TMP2
	st	%RESULT, [%TMP2]		! remember pointer
	add	%TMP2, 4, %TMP2
	st	%TMP2, [%TMP0+%TMP1]		! ssbtopv[gl] in %TMP2
	! %TMP0: dead
	! %TMP1: gl, shifted for indexing
	! %TMP2: ssbtopv[gl]
	ld 	[%GLOBALS+G_SSBLIMV], %TMP0	! ssblimv in %TMP0
	ld	[%TMP0+%TMP1], %TMP0		! ssblimv[gl] in %TMP0
	cmp	%TMP0, %TMP2
	bne	9f
	nop

	! Must compact
	! %TMP0: dead
	! %TMP1: gl, shifted for indexing
	! %TMP2: dead
	set	EXTNAME(C_wb_compact), %TMP0
	! %TMP1 has first argument: gl
	b	callout_to_C			! returns to caller
	srl	%TMP1, 2, %TMP1			! unshift gl
	! Does not return

	! All dead.
9:	retl
	nop

! This barrier implementation simulates, at high cost, the new write barrier,
! so that we can gather accurate statistics.

#if SIMULATE_NEW_BARRIER
EXTNAME(m_full_barrier):
EXTNAME(mem_addtrans):
	set	EXTNAME(C_simulate_new_barrier), %TMP0
	b	callout_to_C
	nop
#endif

! eof
