#ifndef _lide_c_DVL_h
#define _lide_c_DVL_h
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
This is a header file of the DVL (Determine Voltage Level) actor.
*******************************************************************************/


#include "lide_c_actor.h"
#include "lide_c_fifo.h"

/* Actor modes */
#define LIDE_C_DVL_MODE_PROCESS  1
#define LIDE_C_DVL_MODE_HALT 0


/*******************************************************************************
TYPE DEFINITIONS
*******************************************************************************/
/* Structure and pointer types associated with add objects. */
struct _lide_c_DVL_context_struct;
typedef struct _lide_c_DVL_context_struct lide_c_DVL_context_type;

/*******************************************************************************
Create a new actor with specified window size and window overlap for computation 
of high and low level. This function will allocate a trunk of memory for storing 
input data for computation. Ws(Window size) specifies the number of tokens for 
HIGH/LOW level determination. Op(Overlap: larger than or equal to 0 and less than 1) 
specifies that the percentage of tokens is used in last windows and will be 
still used in the next window computation. Tp(Threshold) is the threshold which 
is the value of minimum number of tokens in the related fifo for DVL actor 
execution.
*******************************************************************************/
lide_c_DVL_context_type *lide_c_DVL_new(int Ws, float Op, int Tp, 
                                        lide_c_fifo_pointer in1, 
                                        lide_c_fifo_pointer out1, 
                                        lide_c_fifo_pointer out2);

/*******************************************************************************
Enable function checks whether there is enough data for firing the actor:
Number of data in input fifo should be larger than Tp and number of data in 
output fifo should be less than the buffer capacity.
*******************************************************************************/
boolean lide_c_DVL_enable(lide_c_DVL_context_type *context); 

/*******************************************************************************
Invoke function will compute the voltage corresponding the HIGH/LOW state.
In this function, all the data in the input fifo will be loaded and then sorted;
1st percentile and 99th percentile of the data loaded will be determined as LOW 
and HIGH voltage level respectively.
*******************************************************************************/
void lide_c_DVL_invoke(lide_c_DVL_context_type *context); 

/*******************************************************************************
Terminate function will free the actor including all the memory allocated.
*******************************************************************************/
void lide_c_DVL_terminate(lide_c_DVL_context_type *context);

#endif

