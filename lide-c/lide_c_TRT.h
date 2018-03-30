#ifndef _lide_c_TRT_h
#define _lide_c_TRT_h
/*******************************************************************************
@ddblock_begin copyright

Copyright (c) 1997-2017
Maryland DSPCAD Research Group, The University of Maryland at College Park 

Permission is hereby granted, without written agreement and without
license or royalty fees, to use, copy, modify, and distribute this
software and its documentation for any purpose, provided that the above
copyright notice and the following two paragraphs appear in all copies
of this software.

IN NO EVENT SHALL THE UNIVERSITY OF MARYLAND BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
THE UNIVERSITY OF MARYLAND HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

THE UNIVERSITY OF MARYLAND SPECIFICALLY DISCLAIMS ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE
PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF
MARYLAND HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
ENHANCEMENTS, OR MODIFICATIONS.

@ddblock_end copyright
*******************************************************************************/


/*******************************************************************************
DESCRIPTION:
This is a header file of the TRT (Computing Transition Time) actor.
*******************************************************************************/


#include "lide_c_actor.h"
#include "lide_c_fifo.h"

/* Actor modes */
#define LIDE_C_TRT_MODE_ERROR     0
#define LIDE_C_TRT_MODE_PROCESS     1
#define LIDE_C_TRT_MODE_LENGTH      2
#define LIDE_C_TRT_MODE_HALT 0
#define LIDE_C_TRT_INPUT_THRESHOLD 5
#define LIDE_C_STATE_HIGH	1
#define LIDE_C_STATE_LOW	0
#define LIDE_C_STATE_MID	2
#define LIDE_C_TRAN			1
#define LIDE_C_NON_TRAN		0
#define TRT_DATA_MAX		15999986

/*******************************************************************************
TYPE DEFINITIONS
*******************************************************************************/
/* Structure and pointer types associated with add objects. */
struct _lide_c_TRT_context_struct;
typedef struct _lide_c_TRT_context_struct lide_c_TRT_context_type;



/*******************************************************************************
Create a new actor with 4 input fifos and 9 output fifos. 
*******************************************************************************/
lide_c_TRT_context_type *lide_c_TRT_new(int Ws, lide_c_fifo_pointer in1, 
                    lide_c_fifo_pointer in2, lide_c_fifo_pointer in3, 
                    lide_c_fifo_pointer in4, lide_c_fifo_pointer out1, 
                    lide_c_fifo_pointer out20,lide_c_fifo_pointer out21, 
                    lide_c_fifo_pointer out22,lide_c_fifo_pointer out23, 
                    lide_c_fifo_pointer out30,lide_c_fifo_pointer out31, 
                    lide_c_fifo_pointer out32,lide_c_fifo_pointer out33);
/*******************************************************************************
Enable function checks whether there is enough data for firing the actor
*******************************************************************************/
boolean lide_c_TRT_enable(lide_c_TRT_context_type *context); 
/*******************************************************************************
Invoke function will compute transition time between two transitions.
*******************************************************************************/
void lide_c_TRT_invoke(lide_c_TRT_context_type *context); 
/*******************************************************************************
Terminate function will free the actor including all the memory allocated.
*******************************************************************************/
void lide_c_TRT_terminate(lide_c_TRT_context_type *context);

/*******************************************************************************
Get funtion for TRall_num
*******************************************************************************/
int lide_c_TRT_get_trall_num(lide_c_TRT_context_type *context);

/*******************************************************************************
Get funtion for trt_num
*******************************************************************************/
int lide_c_TRT_get_trt_num(lide_c_TRT_context_type *context);
#endif

