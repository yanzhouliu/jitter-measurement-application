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

#include "lide_c_TRT.h"
#include "lide_c_util.h"

struct _lide_c_TRT_context_struct {
#include "lide_c_actor_context_type_common.h"
	/* Actor interface ports. */
	/*input sequence*/
	lide_c_fifo_pointer in1; /* input data */
	lide_c_fifo_pointer in2; /* medium voltage threshold*/
	lide_c_fifo_pointer in3; /* trt_num*/
	lide_c_fifo_pointer in4; /* TRindex array*/
	/*Output sequence*/
	lide_c_fifo_pointer out1; /* TRT array */
	lide_c_fifo_pointer out20; /* TRall: to RE */
	lide_c_fifo_pointer out21; /* TRall: to RRE */
	lide_c_fifo_pointer out22; /* TRall: to CRE */
	lide_c_fifo_pointer out23; /* TRall: to PHS */
    lide_c_fifo_pointer out30; /* TRall_num: to RE */
	lide_c_fifo_pointer out31; /* TRall_num: to RRE */
	lide_c_fifo_pointer out32; /* TRall_num: to CRE */
	lide_c_fifo_pointer out33; /* TRall_num: to PHS */
    
    
    
	/* Actor Parameters*/
	int Ws; /*Window Size*/
	int k,len;
	int TRlength;
	int TRall_num;
    int trt_num;
	double *vol;
	int *state;
	int *TR;
	int *TRindex;
	double *TRT,*TRall;
    double medium; /* medium voltage threshold*/
};

lide_c_TRT_context_type *lide_c_TRT_new(int Ws, lide_c_fifo_pointer in1, 
                    lide_c_fifo_pointer in2, lide_c_fifo_pointer in3, 
                    lide_c_fifo_pointer in4, lide_c_fifo_pointer out1, 
                    lide_c_fifo_pointer out20,lide_c_fifo_pointer out21, 
                    lide_c_fifo_pointer out22,lide_c_fifo_pointer out23, 
                    lide_c_fifo_pointer out30,lide_c_fifo_pointer out31, 
                    lide_c_fifo_pointer out32,lide_c_fifo_pointer out33){
	int i;
	lide_c_TRT_context_type *context = NULL;
	context = lide_c_util_malloc(sizeof(lide_c_TRT_context_type));
    context->enable = (lide_c_actor_enable_ftype)lide_c_TRT_enable;
    context->invoke = (lide_c_actor_invoke_ftype)lide_c_TRT_invoke;
    context->in1 = in1;
	context->in2 = in2;
	context->in3 = in3;
	context->in4 = in4;
    context->out20 = out20;
    context->out21 = out21;
	context->out22 = out22;
	context->out23 = out23;
    context->out30 = out30;
    context->out31 = out31;
	context->out32 = out32;
	context->out33 = out33;
	context->out1 = out1;
    
	context->mode = LIDE_C_TRT_MODE_LENGTH;
	
    /*Load table and Parameter*/
	context->Ws = Ws;
	context->medium = 0.0;
	context->k = 0;
	context->len = 0;
    context->trt_num = 0;
    context->TRall_num = 0;
	/*Allocate memory for sorting*/
	context->TRindex = (int*)malloc(sizeof(int)*Ws);
	context->TRT = (double*)malloc(sizeof(double)*Ws);
	context->TRall = (double*)malloc(sizeof(double)*TRT_DATA_MAX);
	context->vol = (double*)malloc(sizeof(double)*Ws);
	/* Initialize the array*/
	for (i=0; i<Ws; i++){   
        context->TRT[i] = 0.0; 
        context->vol[i] = 0.0;
        context->TRindex[i] = 0;
    }
	for (i = 0; i<TRT_DATA_MAX; i++)
		context->TRall[i] = 0.0;
	return context;
}

boolean lide_c_TRT_enable(lide_c_TRT_context_type *context) {
	boolean result = FALSE;
	switch (context->mode) {
        case LIDE_C_TRT_MODE_LENGTH:
            result = (lide_c_fifo_population(context->in3) >= 1);
        break;
		case LIDE_C_TRT_MODE_PROCESS:
			result = (lide_c_fifo_population(context->in1) >= context->Ws) &&
					(lide_c_fifo_population(context->in2) >= 1) &&
					(lide_c_fifo_population(context->in4) >= context->trt_num) &&
					(lide_c_fifo_population(context->out1) + context->trt_num <= 
                    lide_c_fifo_capacity(context->out1)) &&
					(lide_c_fifo_population(context->out20) + context->TRall_num 
                    + context->trt_num < lide_c_fifo_capacity(context->out20)) &&
					(lide_c_fifo_population(context->out21) + context->TRall_num 
                    + context->trt_num < lide_c_fifo_capacity(context->out21)) &&
                    (lide_c_fifo_population(context->out22) + context->TRall_num 
                    + context->trt_num < lide_c_fifo_capacity(context->out22)) &&
                    (lide_c_fifo_population(context->out23) + context->TRall_num 
                    + context->trt_num < lide_c_fifo_capacity(context->out23)) &&
                    (lide_c_fifo_population(context->out30) < 
                    lide_c_fifo_capacity(context->out30)) &&
                    (lide_c_fifo_population(context->out31) < 
                    lide_c_fifo_capacity(context->out31)) &&
                    (lide_c_fifo_population(context->out32) < 
                    lide_c_fifo_capacity(context->out32)) &&
                    (lide_c_fifo_population(context->out33) < 
                    lide_c_fifo_capacity(context->out33));
		break;
		default:
			result = FALSE;
		break;
	}
	return result;
}

void lide_c_TRT_invoke(lide_c_TRT_context_type *context) {
	int tmp,i;
	double trt_p1;
	double trt_p2;
    
    switch (context->mode) {
        case LIDE_C_TRT_MODE_LENGTH:
            /*Load data from three fifo*/
            lide_c_fifo_read (context->in3, &context->trt_num);
            context->mode = LIDE_C_TRT_MODE_PROCESS;
        break;
		case LIDE_C_TRT_MODE_PROCESS:
            /*Load data from three fifo*/
            lide_c_fifo_read_block (context->in1, context->vol, context->Ws);
            lide_c_fifo_read (context->in2, &context->medium);
            lide_c_fifo_read_block (context->in4, context->TRindex, 
                                    context->trt_num);
            /* Compute transition time*/
            for (i = 0; i<context->trt_num; i++){
                trt_p1 = fabs(context->vol[context->TRindex[i]]-context->medium);
                trt_p2 = fabs(context->vol[context->TRindex[i]+1]-
                            context->vol[context->TRindex[i]]); 
                context->TRT[i] = 1 + trt_p1/trt_p2 + context->TRindex[i];
            }	
            
            tmp = context->Ws*context->k;
            for(i = 0; i<context->trt_num;i++){
                context->TRall[i+context->TRall_num] = context->TRT[i]+tmp;
            }
            
            context->TRall_num += context->trt_num;
            context->k += 1;
            lide_c_fifo_write_block(context->out1, context->TRT, context->trt_num);
            lide_c_fifo_write_block(context->out20, context->TRall, 
                                    context->TRall_num);
            lide_c_fifo_write_block(context->out21, context->TRall, 
                                    context->TRall_num);
            lide_c_fifo_write_block(context->out22, context->TRall, 
                                    context->TRall_num);
            lide_c_fifo_write_block(context->out23, context->TRall, 
                                    context->TRall_num);
            lide_c_fifo_write(context->out30, &context->TRall_num);
            lide_c_fifo_write(context->out31, &context->TRall_num);
            lide_c_fifo_write(context->out32, &context->TRall_num);
            lide_c_fifo_write(context->out33, &context->TRall_num);
            context->mode = LIDE_C_TRT_MODE_LENGTH;
            
		break;
		default:
			context->mode = LIDE_C_TRT_MODE_ERROR;
		break;
	}
	return;
	
}

void lide_c_TRT_terminate(lide_c_TRT_context_type *context){  
    free(context->vol);  	
    free(context->TRindex);  	
    free(context->TRT);  	
    free(context->TRall);  	
	free(context);
}

int lide_c_TRT_get_trall_num(lide_c_TRT_context_type *context){
    return context->TRall_num;
}


int lide_c_TRT_get_trt_num(lide_c_TRT_context_type *context){
    return context->trt_num;
}