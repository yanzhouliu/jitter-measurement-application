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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "lide_c_DVL.h"
#include "lide_c_util.h"

struct _lide_c_DVL_context_struct {
#include "lide_c_actor_context_type_common.h"
	/* Actor interface ports. */
	lide_c_fifo_pointer in1; /*input data*/
	/*output sequence*/
	lide_c_fifo_pointer out1;/*medium*/
	lide_c_fifo_pointer out2;/*medium*/
	/* Actor Parameters*/
	int Ws; /*Window Size*/
    int vol1; 
    int vol99;
	int THVOL;
	double Op; /*Overlap*/
	int Tp; /*Threshold*/
	double *vol;/*Voltage sequence*/
	double *vol_copy;/*Voltage sequence*/
	double high,low,medium;
	int mark;

};

int cmpfunc (const void * a, const void * b)
{
	double c = *(double*)a - *(double*)b;
	if (c>0)
		return 1;
	else if (c<0)
		return -1;
	else 
		return 0;
}

lide_c_DVL_context_type *lide_c_DVL_new(int Ws, float Op, int Tp, 
                                        lide_c_fifo_pointer in1, 
                                        lide_c_fifo_pointer out1, 
                                        lide_c_fifo_pointer out2){
	int i;
	lide_c_DVL_context_type *context = NULL;
	context = lide_c_util_malloc(sizeof(lide_c_DVL_context_type));
    context->enable = (lide_c_actor_enable_ftype)lide_c_DVL_enable;
    context->invoke = (lide_c_actor_invoke_ftype)lide_c_DVL_invoke;
    context->in1 = in1;
    context->out1 = out1;
	context->out2 = out2;
	context->mode = 1;
	/*Load table and Parameter*/
	context->Ws = Ws;	
	context->Op = Op;
	context->Tp = Tp;
	context->high = 0.0;
	context->low = 0.0;
	context->medium = 0.0;
	context->mark = 0;
	context->THVOL = 1;
    context->vol1 = (int)(context->Ws/100);
	context->vol99 = (int)(context->Ws - context->vol1);
	/*Allocate memory for sorting*/
	context->vol = (double*)malloc(sizeof(double)*Ws);
	context->vol_copy = (double*)malloc(sizeof(double)*Ws);
	/* Initialize voltage array*/
	for (i=0; i<Ws; i++){  
        context->vol[i] = 0.0; 
        context->vol_copy[i] = 0.0; 
    }
	return context;
}

boolean lide_c_DVL_enable(lide_c_DVL_context_type *context) {
	boolean result = FALSE;	
	switch (context->mode) {
		case LIDE_C_DVL_MODE_PROCESS:
			result = (lide_c_fifo_population(context->in1) >= context->Tp) &&
					(lide_c_fifo_population(context->out1) < lide_c_fifo_capacity(context->out1)) &&
					(lide_c_fifo_population(context->out2) < lide_c_fifo_capacity(context->out2));
		break;
		default:
			result = FALSE;
		break;
	}
	return result;
	
}

void lide_c_DVL_invoke(lide_c_DVL_context_type *context) {
	int i,numc,numr;
    int size;
    
	/*Assign Value*/
    size = context->Ws * sizeof(double);
	lide_c_fifo_read_block(context->in1, context->vol, context->Ws);
    memcpy(context->vol_copy, context->vol, size);

    qsort(context->vol, context->Ws, sizeof(context->vol[0]), cmpfunc);

    context->high = context->vol[context->vol99];
    context->low = context->vol[context->vol1];
    context->medium = (context->high + context->low)/2;
    
	/* Consume and move data from the input FIFO*/
	numr = context->Ws * context->Op;
	numc = context->Ws - numr;
    
	lide_c_fifo_write_block(context->in1, &context->vol[numc], numr);
	
	/* 
		Load result to out FIFO
	*/
	

	lide_c_fifo_write(context->out1, &context->medium);
	lide_c_fifo_write(context->out2, &context->medium);


	return;
	
}

void lide_c_DVL_terminate(lide_c_DVL_context_type *context){  
    free(context->vol);  	
	free(context);
}