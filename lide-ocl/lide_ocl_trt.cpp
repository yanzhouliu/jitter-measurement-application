/*******************************************************************************
@ddblock_begin copyright

Copyright (c) 1999-2012
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
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cmath>
#include <math.h>
#include <time.h>
#include <windows.h>

#include "lide_ocl_fifo.h"
#include "lide_ocl_trt.h"
#include "lide_ocl_util.h"
#include "lide_ocl_gpu.h"
#include "lide_ocl_kernel.h"
#include "lide_ocl_jitter_util.h"



/*******************************************************************************
STRUCTURE DEFINITION
*******************************************************************************/

struct _lide_ocl_trt_context_struct { 
#include "lide_ocl_actor_context_type_common.h"

    /* Actor Parameters*/
    unsigned int Ws; /*Window Size*/
    unsigned int EXP_2;
	unsigned int EXP_2_c1;
	unsigned int EXP_2_c2;
    unsigned int index;
    double time;
    int NUM;
    int multi;
    size_t SubElements;
    /* Input ports. */
    lide_ocl_fifo_pointer in2trt;
    lide_ocl_fifo_pointer trstate;
    lide_ocl_fifo_pointer volth;
    /* Output port. */
    lide_ocl_fifo_pointer trtime;
    lide_ocl_fifo_pointer trtnum2re;  
    /* GPU setting*/
    lide_ocl_gpu_pointer gpu;
    lide_ocl_kernel_pointer kernel_trt;
    lide_ocl_kernel_pointer kernel_trt_prefix_sum_sub;
    lide_ocl_kernel_pointer kernel_trt_prefix_sum_sum;
    lide_ocl_kernel_pointer kernel_trt_prefix_offset_sum;
    lide_ocl_kernel_pointer kernel_trt_stream_cmpt;
    lide_ocl_kernel_pointer kernel_trt_stream_cmpt_2;
    /* Kernel Settings*/
    /* GPU buffer*/
    cl_mem cmDevDstTrt;
    cl_mem cmDevDstTrt_local;
    cl_mem cmDevDstTrt_num;
    cl_mem cmDevPred_scan;
    cl_mem cmDevSub_sum;
    
    /* Performance var */
    double t_k_trt[6];
    __int64 counter_trt_k;
    
    double t_trt;
    __int64 counter_trt;
};



/*******************************************************************************
IMPLEMENTATIONS OF INTERFACE FUNCTIONS.
*******************************************************************************/

lide_ocl_trt_context_type *lide_ocl_trt_new(
        lide_ocl_fifo_pointer in2trt, lide_ocl_fifo_pointer trstate, 
        lide_ocl_fifo_pointer volth, lide_ocl_fifo_pointer trtime, 
        lide_ocl_fifo_pointer trtnum2re, 
        lide_ocl_gpu_pointer gpu, int szGlobalWorkSize, 
        int szLocalWorkSize, int Ws, int NUM){ 
    double *vol;

    int i;
    lide_ocl_trt_context_type *context = NULL;
    
    context = (lide_ocl_trt_context_type *)lide_ocl_util_malloc(
                sizeof(lide_ocl_trt_context_type));
    context->mode = LIDE_OCL_TRT_MODE_PROCESS;
    context->enable =
            (lide_ocl_actor_enable_function_type)lide_ocl_trt_enable;
    context->invoke = 
            (lide_ocl_actor_invoke_function_type)lide_ocl_trt_invoke;
    /* Variable */
    context->Ws = Ws;
    context->multi = 1;
    context->index = 0;	
    context->time = 0;	
    context->NUM = NUM;	
    context->EXP_2 = (int)(LOG2((double)szLocalWorkSize));
    //printf("exp_2 in trt actor:%d\n", context->EXP_2);
    /* FIFO*/
    context->volth = volth;
    context->trstate = trstate;
    context->in2trt = in2trt;
    context->trtime = trtime;
    context->trtnum2re = trtnum2re;
    /* GPU construction*/
    context->kernel_trt = (lide_ocl_kernel_pointer) lide_ocl_util_malloc (
                            sizeof (lide_ocl_kernel_type));
    lide_ocl_kernel_set_gpu_setting(context->kernel_trt, gpu);
    lide_ocl_kernel_set_cSourceFile(context->kernel_trt, 
                                    "TRT.cl");
    lide_ocl_kernel_set_cPathAndName(context->kernel_trt, JITTER_OCL, 
                                    "TRT.cl");	

    context->kernel_trt_prefix_sum_sub = (lide_ocl_kernel_pointer) 
                                            lide_ocl_util_malloc (
                                            sizeof (lide_ocl_kernel_type));
    lide_ocl_kernel_set_gpu_setting(context->kernel_trt_prefix_sum_sub, gpu);
    lide_ocl_kernel_set_cSourceFile(context->kernel_trt_prefix_sum_sub, 
                                    "trt_stream_cmpt.cl");
    lide_ocl_kernel_set_cPathAndName(context->kernel_trt_prefix_sum_sub, 
                                    JITTER_OCL, "trt_stream_cmpt.cl");	
    
    context->kernel_trt_prefix_sum_sum = (lide_ocl_kernel_pointer) 
                                            lide_ocl_util_malloc (
                                            sizeof (lide_ocl_kernel_type));
    lide_ocl_kernel_set_gpu_setting(context->kernel_trt_prefix_sum_sum, gpu);
    lide_ocl_kernel_set_cSourceFile(context->kernel_trt_prefix_sum_sum, 
                                    "trt_stream_cmpt.cl");
    lide_ocl_kernel_set_cPathAndName(context->kernel_trt_prefix_sum_sum, 
                                    JITTER_OCL, "trt_stream_cmpt.cl");
                                    
    context->kernel_trt_prefix_offset_sum = (lide_ocl_kernel_pointer) 
                                                lide_ocl_util_malloc (
                                                sizeof (lide_ocl_kernel_type));
    lide_ocl_kernel_set_gpu_setting(context->kernel_trt_prefix_offset_sum, gpu);
    lide_ocl_kernel_set_cSourceFile(context->kernel_trt_prefix_offset_sum, 
                                    "trt_stream_cmpt.cl");
    lide_ocl_kernel_set_cPathAndName(context->kernel_trt_prefix_offset_sum, 
                                    JITTER_OCL, "trt_stream_cmpt.cl");

    context->kernel_trt_stream_cmpt = (lide_ocl_kernel_pointer) 
                                    lide_ocl_util_malloc (
                                    sizeof (lide_ocl_kernel_type));
    lide_ocl_kernel_set_gpu_setting(context->kernel_trt_stream_cmpt, gpu);
    lide_ocl_kernel_set_cSourceFile(context->kernel_trt_stream_cmpt, 
                                    "trt_stream_cmpt.cl");
    lide_ocl_kernel_set_cPathAndName(context->kernel_trt_stream_cmpt, 
                                    JITTER_OCL, "trt_stream_cmpt.cl");
    
    context->kernel_trt_stream_cmpt_2 = (lide_ocl_kernel_pointer) 
                                    lide_ocl_util_malloc (
                                    sizeof (lide_ocl_kernel_type));
    lide_ocl_kernel_set_gpu_setting(context->kernel_trt_stream_cmpt_2, gpu);
    lide_ocl_kernel_set_cSourceFile(context->kernel_trt_stream_cmpt_2, 
                                    "trt_stream_cmpt.cl");
    lide_ocl_kernel_set_cPathAndName(context->kernel_trt_stream_cmpt_2, 
                                    JITTER_OCL, "trt_stream_cmpt.cl");
    
    
    context->gpu = gpu;
    /* GPU memory allocation*/
    /*GPU settings*/
    context->gpu->szGlobalWorkSize = szGlobalWorkSize;
    context->gpu->szLocalWorkSize = szLocalWorkSize;
    context->cmDevDstTrt = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_double) * 
                            context->gpu->szGlobalWorkSize, NULL, 
                            &context->gpu->ciErr1);                      
    context->cmDevDstTrt_local = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_double) * TIE_NUM, 
                            NULL, &context->gpu->ciErr2);                            
                            
    context->gpu->ciErr1 |= context->gpu->ciErr2;
    context->cmDevDstTrt_num = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, sizeof(cl_int) * 1, NULL, 
                            &context->gpu->ciErr2);
    context->gpu->ciErr1 |= context->gpu->ciErr2;	
    context->cmDevPred_scan = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_int) * context->gpu->szGlobalWorkSize, 
                            NULL, &context->gpu->ciErr2);
    context->gpu->ciErr1 |= context->gpu->ciErr2;
    context->cmDevSub_sum = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_int) * context->gpu->szGlobalWorkSize/
                            context->gpu->szLocalWorkSize, NULL, 
                            &context->gpu->ciErr2);
    context->gpu->ciErr1 |= context->gpu->ciErr2;
    
    /*
    if (context->gpu->ciErr1 != CL_SUCCESS)
    {
        fprintf(stderr, "Error in clCreateBuffer, Line %u in file %s !!!\n\n", 
                __LINE__, __FILE__);
        lide_ocl_gpu_cleanup(context->gpu);
        exit(1);
    }
    */
    
    /*clean-up memory*/
    vol = (double *)lide_ocl_util_malloc(
            sizeof(double)*TIE_NUM);        
    for (i = 0; i< TIE_NUM; i++){
        vol[i] = 0.0;
    }
    
    i = 0;                      
    context->gpu->ciErr1 |= clEnqueueWriteBuffer(context->gpu->cqCommandQueue, 
                            context->cmDevDstTrt_local, CL_TRUE, 0, 
                            sizeof(cl_double) * TIE_NUM, vol, 0, NULL, NULL);                        
                            
    
    context->gpu->ciErr1 |= clEnqueueWriteBuffer(context->gpu->cqCommandQueue, 
                            context->cmDevDstTrt_num, CL_TRUE, 0, 
                            sizeof(cl_int) * 1, &i, 0, NULL, NULL);
    /*
    if (context->gpu->ciErr1 != CL_SUCCESS)
    {
        fprintf(stderr, "Error in clwriteBuffer, Line %u in file %s !!!\n\n", 
                __LINE__, __FILE__);
        lide_ocl_gpu_cleanup(context->gpu);
        exit(1);
    }
    */
    
    
    if (context->gpu->ciErr1 != CL_SUCCESS)
    {
        fprintf(stderr, "error in trt init.\n");
        fprintf(stderr, "Error %d in buffer init, Line %u in file %s !!!\n\n", 
                context->gpu->ciErr1, __LINE__, __FILE__);
        lide_ocl_gpu_cleanup(context->gpu);
        context->mode = LIDE_OCL_TRT_MODE_ERROR;
        exit(1);
    }
    
    free(vol);
    /* First kernel*/	
    lide_ocl_kernel_load_source (context->kernel_trt, "");		
    lide_ocl_kernel_create_program(context->kernel_trt, 1);	
    lide_ocl_kernel_build_program (context->kernel_trt, 0, 
                                    NULL, NULL, NULL, NULL);	
    lide_ocl_kernel_create_kernel (context->kernel_trt, "TRT");

    lide_ocl_kernel_load_source (context->kernel_trt_prefix_sum_sub, "");		
    lide_ocl_kernel_create_program(context->kernel_trt_prefix_sum_sub, 1);	
    lide_ocl_kernel_build_program (context->kernel_trt_prefix_sum_sub, 0, 
                                    NULL, NULL, NULL, NULL);	
    lide_ocl_kernel_create_kernel (context->kernel_trt_prefix_sum_sub, 
                                    "prefix_sum_sub");

    lide_ocl_kernel_load_source (context->kernel_trt_prefix_sum_sum, "");		
    lide_ocl_kernel_create_program(context->kernel_trt_prefix_sum_sum, 1);	
    lide_ocl_kernel_build_program (context->kernel_trt_prefix_sum_sum, 0, 
                                    NULL, NULL, NULL, NULL);	
    lide_ocl_kernel_create_kernel (context->kernel_trt_prefix_sum_sum, 
                                    "prefix_sum_sub_sum");

    lide_ocl_kernel_load_source (context->kernel_trt_prefix_offset_sum, "");		
    lide_ocl_kernel_create_program(context->kernel_trt_prefix_offset_sum, 1);	
    lide_ocl_kernel_build_program (context->kernel_trt_prefix_offset_sum, 0, 
                                    NULL, NULL, NULL, NULL);	
    lide_ocl_kernel_create_kernel (context->kernel_trt_prefix_offset_sum, 
                                    "prefix_offset_sum");    
    
    lide_ocl_kernel_load_source (context->kernel_trt_stream_cmpt, "");		
    lide_ocl_kernel_create_program(context->kernel_trt_stream_cmpt, 1);	
    lide_ocl_kernel_build_program (context->kernel_trt_stream_cmpt, 0, 
                                    NULL, NULL, NULL, NULL);	
    lide_ocl_kernel_create_kernel (context->kernel_trt_stream_cmpt, 
                                    "stream_cmpt");
    
    lide_ocl_kernel_load_source (context->kernel_trt_stream_cmpt_2, "");		
    lide_ocl_kernel_create_program(context->kernel_trt_stream_cmpt_2, 1);	
    lide_ocl_kernel_build_program (context->kernel_trt_stream_cmpt_2, 0, 
                                    NULL, NULL, NULL, NULL);	
    lide_ocl_kernel_create_kernel (context->kernel_trt_stream_cmpt_2, 
                                    "stream_cmpt_2");
    
   
    /*Argument Settings*/
    context->gpu->ciErr1 |= clSetKernelArg(context->kernel_trt->ckKernel, 4, 
                            sizeof(int), 
                            (void*)&context->gpu->szGlobalWorkSize);
    
    context->gpu->ciErr1 |= clSetKernelArg(
                            context->kernel_trt_prefix_sum_sub->ckKernel, 3, 
                            sizeof(cl_int), (void*)&context->Ws);
    
    context->SubElements = context->gpu->szGlobalWorkSize/
                            context->gpu->szLocalWorkSize;
    context->gpu->ciErr1 |= clSetKernelArg(
                            context->kernel_trt_prefix_sum_sum->ckKernel, 1, 
                            sizeof(cl_int), (void*)&context->SubElements);
    
    context->gpu->ciErr1 |= clSetKernelArg(
                            context->kernel_trt_prefix_offset_sum->ckKernel, 
                            2, sizeof(cl_int), 
                            (void*)&context->gpu->szGlobalWorkSize);
    
    context->gpu->ciErr1 |= clSetKernelArg(
                            context->kernel_trt_stream_cmpt->ckKernel, 5, 
                            sizeof(cl_uint), (void*)&context->Ws);
    context->gpu->ciErr1 |= clSetKernelArg(
                            context->kernel_trt_stream_cmpt_2->ckKernel, 5, 
                            sizeof(cl_uint), (void*)&context->Ws);
    
    if (context->gpu->ciErr1 != CL_SUCCESS)
    {
        fprintf(stderr, "error in trt init.\n");
        fprintf(stderr, "Error %d in argument settings, Line %u in file %s \
                !!!\n\n", context->gpu->ciErr1, __LINE__, __FILE__);
        lide_ocl_gpu_cleanup(context->gpu);
        context->mode = LIDE_OCL_TRT_MODE_ERROR;
        exit(1);
    }
    context->EXP_2_c1 = (int)LOG2((double)(context->gpu->szLocalWorkSize));
    /* Performance var*/
    context->t_trt = 0.0;
    context->counter_trt = 0;
    for (i = 0; i < 6; i++)
        context->t_k_trt[i] = 0.0;
    context->counter_trt_k = 0;
    
    return context;
}

boolean lide_ocl_trt_enable(
        lide_ocl_trt_context_type *context) {
    boolean result = FALSE;
    switch (context->mode) {
    case LIDE_OCL_TRT_MODE_ERROR:
        result = FALSE;
        break;
    case LIDE_OCL_TRT_MODE_PROCESS:
        result = (lide_ocl_fifo_population(context->in2trt) >= 1) &&
                (lide_ocl_fifo_population(context->volth) >= 1) && 
                (lide_ocl_fifo_population(context->trstate) >= 1) && 
                ((lide_ocl_fifo_population(context->trtime) < 
                lide_ocl_fifo_capacity(context->trtime)));
        break;
    default:
        result = FALSE;
        break;
    }
    return result;
}

void lide_ocl_trt_invoke(lide_ocl_trt_context_type *context) {
    cl_mem input, trstate, medium;
    size_t iNumElements;
    size_t SubElements;

    iNumElements = TIE_NUM;
    SubElements = context->gpu->szGlobalWorkSize/
                    context->gpu->szLocalWorkSize;
    switch (context->mode) {
    case LIDE_OCL_TRT_MODE_ERROR:
        fprintf(stderr, "lide_ocl_trt: error invalid status.\n");
        context->mode = LIDE_OCL_TRT_MODE_ERROR;
        break;
    case LIDE_OCL_TRT_MODE_PROCESS:
        context->counter_trt = StartCounter();
        /* get input data*/
        lide_ocl_fifo_read(context->in2trt, &input);
        lide_ocl_fifo_read(context->trstate, &trstate);
        lide_ocl_fifo_read(context->volth, &medium);
    

        /* State transition*/
        /* Set the Argument values*/
        context->gpu->ciErr1 = clSetKernelArg(context->kernel_trt->ckKernel, 
                                0, sizeof(cl_mem), (void*)&input);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_trt->ckKernel, 
                                1, sizeof(cl_mem), (void*)&trstate);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_trt->ckKernel, 
                                2, sizeof(cl_mem), (void*)&medium);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_trt->ckKernel, 
                                3, sizeof(cl_mem), 
                                (void*)&context->cmDevDstTrt);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clSetKernelArg, Line %u in file %s \
                    !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */    
        
        /* Launch kernel*/
        clFinish (context->gpu->cqCommandQueue);
   
        context->counter_trt_k = StartCounter();
        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_trt->ckKernel, 1, NULL, 
                                &context->gpu->szGlobalWorkSize, 
                                &context->gpu->szLocalWorkSize, 0, NULL, NULL);
        clFinish (context->gpu->cqCommandQueue);
        context->t_k_trt[0] += GetCounter(context->counter_trt_k);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u in file \
                    %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        
        /*TRT compact and concatenate*/
        /*Prefix sum*/
        /* prefix sub sum*/
		context->EXP_2 = context->EXP_2_c1;
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_prefix_sum_sub->ckKernel, 
                                0, sizeof(cl_mem), (void*)&trstate);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_prefix_sum_sub->ckKernel, 
                                1, sizeof(cl_mem), 
                                (void*)&context->cmDevPred_scan);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_prefix_sum_sub->ckKernel, 
                                2, sizeof(cl_mem), 
                                (void*)&context->cmDevSub_sum);

        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_prefix_sum_sub->ckKernel, 
                                4, sizeof(cl_int), (void*)&context->EXP_2);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error %d in clSetKernelArg, Line %u in file %s \
                    !!!\n\n", context->gpu->ciErr1, __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }	
        */        
        /* Launch kernel*/
        clFinish (context->gpu->cqCommandQueue);
        context->counter_trt_k = StartCounter();

        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_trt_prefix_sum_sub->ckKernel, 
                                1, NULL, &context->gpu->szGlobalWorkSize, 
                                &context->gpu->szLocalWorkSize, 0, NULL, NULL);
        clFinish (context->gpu->cqCommandQueue);
        context->t_k_trt[1] += GetCounter(context->counter_trt_k);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u in file \
                    %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        
        /* Summation of subsum*/
        context->EXP_2 = (int)LOG2((double)(context->gpu->szGlobalWorkSize/
                                context->gpu->szLocalWorkSize));
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_prefix_sum_sum->ckKernel, 
                                0, sizeof(cl_mem), 
                                (void*)&context->cmDevSub_sum);
        
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_prefix_sum_sum->ckKernel, 
                                2, sizeof(cl_int), (void*)&context->EXP_2);
        
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error %d in clSetKernelArg, Line %u in file %s \
                    !!!\n\n", context->gpu->ciErr1, __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */        
        /* Launch kernel*/
        clFinish (context->gpu->cqCommandQueue);
        
        context->counter_trt_k = StartCounter();
        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_trt_prefix_sum_sum->ckKernel, 
                                1, NULL, &SubElements, &SubElements, 0, 
                                NULL, NULL);
   
        clFinish (context->gpu->cqCommandQueue);
        context->t_k_trt[2] += GetCounter(context->counter_trt_k);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u in file \
                    %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        /* Update Prefix sum*/
        context->gpu->ciErr1 |= clSetKernelArg(
                            context->kernel_trt_prefix_offset_sum->ckKernel, 
                            0, sizeof(cl_mem), 
                            (void*)&context->cmDevPred_scan);
        context->gpu->ciErr1 |= clSetKernelArg(
                            context->kernel_trt_prefix_offset_sum->ckKernel, 
                            1, sizeof(cl_mem), (void*)&context->cmDevSub_sum);
        
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error %d in clSetKernelArg, Line %u in file %s \
                    !!!\n\n", context->gpu->ciErr1, __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }	
        */     
        
        /* Launch kernel*/
        clFinish (context->gpu->cqCommandQueue);
        
        context->counter_trt_k = StartCounter();
        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                            context->gpu->cqCommandQueue, 
                            context->kernel_trt_prefix_offset_sum->ckKernel, 
                            1, NULL, &context->gpu->szGlobalWorkSize, 
                            &context->gpu->szLocalWorkSize, 0, NULL, NULL);
        clFinish (context->gpu->cqCommandQueue);
        context->t_k_trt[3] += GetCounter(context->counter_trt_k);
        
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u in file \
                    %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }   
        */
        /*stream compact*/
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt->ckKernel, 0, 
                                sizeof(cl_mem), (void*)&context->cmDevDstTrt);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt->ckKernel, 1, 
                                sizeof(cl_mem), (void*)&trstate);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt->ckKernel, 2, 
                                sizeof(cl_mem), 
                                (void*)&context->cmDevPred_scan);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt->ckKernel, 3, 
                                sizeof(cl_mem), 
                                (void*)&context->cmDevDstTrt_local);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt->ckKernel, 4, 
                                sizeof(cl_mem), 
                                (void*)&context->cmDevDstTrt_num);
        
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt->ckKernel, 6, 
                                sizeof(cl_uint), (void*)&context->index);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clSetKernelArg, Line %u in file %s \
                    !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }	
        */        
        /* Launch kernel*/
        clFinish (context->gpu->cqCommandQueue);
        
        context->counter_trt_k = StartCounter();
        
        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_trt_stream_cmpt->ckKernel, 1, 
                                NULL, &context->gpu->szGlobalWorkSize, 
                                &context->gpu->szLocalWorkSize, 0, NULL, NULL);
        
        clFinish (context->gpu->cqCommandQueue);
        context->t_k_trt[4] += GetCounter(context->counter_trt_k);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u in file \
                    %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt_2->ckKernel, 
                                0, sizeof(cl_mem), 
                                (void*)&context->cmDevDstTrt);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt_2->ckKernel, 
                                1, sizeof(cl_mem), (void*)&trstate);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt_2->ckKernel, 
                                2, sizeof(cl_mem), 
                                (void*)&context->cmDevPred_scan);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt_2->ckKernel, 
                                3, sizeof(cl_mem), 
                                (void*)&context->cmDevDstTrt_local);
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt_2->ckKernel, 
                                4, sizeof(cl_mem), 
                                (void*)&context->cmDevDstTrt_num);
        
        context->gpu->ciErr1 |= clSetKernelArg(
                                context->kernel_trt_stream_cmpt_2->ckKernel, 
                                6, sizeof(cl_uint), (void*)&context->index);
        
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clSetKernelArg, Line %u in file %s \
                    !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        /* Launch kernel*/
        //clFinish (context->gpu->cqCommandQueue);        
        context->counter_trt_k = StartCounter();
        context->gpu->ciErr1 = clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_trt_stream_cmpt_2->ckKernel, 
                                1, NULL, &context->gpu->szGlobalWorkSize, 
                                &context->gpu->szLocalWorkSize, 0, NULL, NULL);
        clFinish (context->gpu->cqCommandQueue);
        context->t_k_trt[5] += GetCounter(context->counter_trt_k);
        
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u in file \
                    %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        //clFinish (context->gpu->cqCommandQueue);
        
        
        context->index += 1;
        
        clFinish (context->gpu->cqCommandQueue);
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "error in trt, Num: %d\n", context->index);
            fprintf(stderr, "Error %d in TRT, Line %u in file %s !!!\n\n", 
                context->gpu->ciErr1, __LINE__, __FILE__);
            context->mode = LIDE_OCL_TRT_MODE_ERROR;
            context->index = context->index+1;
            context->t_trt += GetCounter(context->counter_trt);
            break;
            //lide_ocl_gpu_cleanup(context->gpu);
        }
        
        lide_ocl_fifo_write(context->trtime, &context->cmDevDstTrt_local);
        lide_ocl_fifo_write(context->trtnum2re, &context->cmDevDstTrt_num);
        context->t_trt += GetCounter(context->counter_trt);
        break;
    default:
        context->mode = LIDE_OCL_TRT_MODE_PROCESS;
        break;
    }

    return;
}

void lide_ocl_trt_terminate(
        lide_ocl_trt_context_type *context) {
    int i;
    
    printf("time in TRT:%.6lf\n", context->t_trt); 
    for(i = 0; i<6; i++){
        printf("subtime in TRT: ID = %d; Time:%.6lf\n", i, 
                context->t_k_trt[i]);
    }
    
    clReleaseMemObject(context->cmDevDstTrt);
    clReleaseMemObject(context->cmDevDstTrt_local);
    clReleaseMemObject(context->cmDevDstTrt_num);
    clReleaseMemObject(context->cmDevPred_scan);
    clReleaseMemObject(context->cmDevSub_sum);
    
    
    clReleaseKernel(context->kernel_trt->ckKernel);
    clReleaseProgram(context->kernel_trt->cpProgram);

    clReleaseKernel(context->kernel_trt_prefix_sum_sub->ckKernel);
    clReleaseProgram(context->kernel_trt_prefix_sum_sub->cpProgram);
    
    clReleaseKernel(context->kernel_trt_prefix_sum_sum->ckKernel);
    clReleaseProgram(context->kernel_trt_prefix_sum_sum->cpProgram);
    
    clReleaseKernel(context->kernel_trt_prefix_offset_sum->ckKernel);
    clReleaseProgram(context->kernel_trt_prefix_offset_sum->cpProgram);
    
    clReleaseKernel(context->kernel_trt_stream_cmpt->ckKernel);
    clReleaseProgram(context->kernel_trt_stream_cmpt->cpProgram);
    
    free(context->kernel_trt);
    free(context);
}



