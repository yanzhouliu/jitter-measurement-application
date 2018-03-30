#ifndef _lide_ocl_trt_h
#define _lide_ocl_trt_h

/*******************************************************************************
@ddblock_begin copyright

Copyright (c) 1999-2012
Maryland DSPCAD Research Group, The University of Maryland at College Park 

Permission is hereby granted, without written agreement and without
license or royalty fees, to use, copy, modify, and ditrtibute this
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

#include "lide_ocl_actor.h"
#include "lide_ocl_fifo.h"
#include "lide_ocl_gpu.h"

/*******************************************************************************
Transition Time Actor
*******************************************************************************/

/* Actor modes */
#define LIDE_OCL_TRT_MODE_ERROR          0
#define LIDE_OCL_TRT_MODE_PROCESS        1 

/*******************************************************************************
TYPE DEFINITIONS
*******************************************************************************/

/* Structure and pointer types associated with inner product objects. */
struct _lide_ocl_trt_context_struct;
typedef struct _lide_ocl_trt_context_struct
        lide_ocl_trt_context_type;

/*******************************************************************************
INTERFACE FUNCTIONS
*******************************************************************************/

/*****************************************************************************
Construct function of the lide_ocl_trt actor. 
*****************************************************************************/
lide_ocl_trt_context_type *lide_ocl_trt_new(
        lide_ocl_fifo_pointer in2trt, lide_ocl_fifo_pointer trstate, 
		lide_ocl_fifo_pointer volth, lide_ocl_fifo_pointer trtime, 
        lide_ocl_fifo_pointer trtnum2re, 
		lide_ocl_gpu_pointer gpu, int szGlobalWorkSize, 
		int szLocalWorkSize, int Ws, int NUM);

/*****************************************************************************
Enable function of the lide_ocl_trt actor.
*****************************************************************************/
boolean lide_ocl_trt_enable(lide_ocl_trt_context_type *context);

/*****************************************************************************
Invoke function of the lide_ocl_trt actor.
*****************************************************************************/
void lide_ocl_trt_invoke(lide_ocl_trt_context_type *context);

/*****************************************************************************
Terminate function of the lide_ocl_trt actor.
*****************************************************************************/
void lide_ocl_trt_terminate(lide_ocl_trt_context_type *context);



#endif
