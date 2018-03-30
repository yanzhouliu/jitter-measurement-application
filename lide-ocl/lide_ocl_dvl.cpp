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

#include <math.h>
#include <time.h>
#include <windows.h>
#include "lide_ocl_fifo.h"
#include "lide_ocl_dvl.h"
#include "lide_ocl_util.h"
#include "lide_ocl_gpu.h"
#include "lide_ocl_kernel.h"
#include "lide_ocl_jitter_util.h"

/* NOTE: residue and num could be removed later */

/*******************************************************************************
INNER PRODUCT STRUCTURE DEFINITION
*******************************************************************************/


struct _lide_ocl_dvl_context_struct { 
#include "lide_ocl_actor_context_type_common.h"

    /* Actor Parameters*/
    size_t szGlobalWorkSize;
    unsigned int Ws; /*Window Size*/
    unsigned int NUM; 
    double time;
    int Residue; 
    int index; 
    double Op; /*Overlap*/
    int Tp; /*Threshold*/
    double *vol;/*Voltage sequence*/
    double high,low,medium;
    int mark;
    unsigned int dir;
    unsigned int inc;
    unsigned int length;
    unsigned int level;
    unsigned int col;
    
    
    /* Var for sort-selection*/
    unsigned int PM_num;
    unsigned int num_select;
    FILE *fpr;
    /* Input ports. */
    lide_ocl_fifo_pointer x;


    /* Output port. */
    lide_ocl_fifo_pointer volth;
    lide_ocl_fifo_pointer mid2trt;
    lide_ocl_fifo_pointer in2trt;
    lide_ocl_fifo_pointer in2str;
    
    /* GPU setting*/
    lide_ocl_gpu_pointer gpu;
    lide_ocl_kernel_pointer kernel_sort;
    lide_ocl_kernel_pointer kernel_h2l;
    lide_ocl_kernel_pointer kernel_select;
    lide_ocl_kernel_pointer kernel_select_first;
    /* Kernel Settings*/
    /* Host Buffer*/
    void *srcX;
    /* GPU buffer*/
    cl_mem cmDevSrcInput, cmDevSrcIn, cmDevSrcOut, cmDevSrcM, cmDevSrcH, 
            cmDevSrcL, cmDevIndex;
  
	/*Performance var*/
    double t_dvl;
    __int64 counter_dvl;
};


/*******************************************************************************
IMPLEMENTATIONS OF INTERFACE FUNCTIONS.
*******************************************************************************/

lide_ocl_dvl_context_type *lide_ocl_dvl_new(
        lide_ocl_fifo_pointer x, lide_ocl_fifo_pointer volth, 
        lide_ocl_fifo_pointer in2str, lide_ocl_fifo_pointer in2trt, 
        lide_ocl_fifo_pointer mid2trt, lide_ocl_gpu_pointer gpu, 
        int szGlobalWorkSize, int szLocalWorkSize, int Ws, float Op, int Tp,
        int NUM, int Residue) { 

    lide_ocl_dvl_context_type *context = NULL;

    context = (lide_ocl_dvl_context_type *)lide_ocl_util_malloc(
                                            sizeof(lide_ocl_dvl_context_type));
    
    if (context == NULL){
        fprintf(stderr, "allocation of dvl context failed\n");
    }
    
    context->mode = LIDE_OCL_DVL_MODE_PROCESS;
    context->enable =
            (lide_ocl_actor_enable_function_type)lide_ocl_dvl_enable;
    context->invoke = 
            (lide_ocl_actor_invoke_function_type)lide_ocl_dvl_invoke;
    /* Variable */
    context->Ws = Ws;	
    context->NUM = NUM;	
    context->Residue = Residue;	
    context->index = 0;	
    context->time = 0;	
    context->Op = Op;
    context->Tp = Tp;
    context->high = 0.0;
    context->low = 0.0;
    context->medium = 0.0;
    context->mark = 0;
    
    /* Var for sort-selection*/
    context->PM_num = PM_NUM;
    context->num_select = Sort_NUM;
    
    context->szGlobalWorkSize = Sort_NUM;
    context->level = (unsigned int)(LOG2((double)context->szGlobalWorkSize));
    /* FIFO */
    context->x = x;
    context->volth = volth;
    context->in2trt = in2trt;
    context->mid2trt = mid2trt;
    context->in2str = in2str;
    
    /* GPU construction*/
    context->kernel_sort = (lide_ocl_kernel_pointer) lide_ocl_util_malloc (
                            sizeof (lide_ocl_kernel_type));
    context->kernel_h2l = (lide_ocl_kernel_pointer) lide_ocl_util_malloc (
                            sizeof (lide_ocl_kernel_type));
    context->kernel_select = (lide_ocl_kernel_pointer) lide_ocl_util_malloc (
                            sizeof (lide_ocl_kernel_type));
    context->kernel_select_first = (lide_ocl_kernel_pointer) lide_ocl_util_malloc (
                            sizeof (lide_ocl_kernel_type));

    
    lide_ocl_kernel_set_gpu_setting(context->kernel_sort, gpu);
    lide_ocl_kernel_set_gpu_setting(context->kernel_h2l, gpu);
    lide_ocl_kernel_set_gpu_setting(context->kernel_select, gpu);
    lide_ocl_kernel_set_gpu_setting(context->kernel_select_first, gpu);
    
    lide_ocl_kernel_set_cSourceFile(context->kernel_sort, 
                                    "BitonicSort.cl");
    lide_ocl_kernel_set_cSourceFile(context->kernel_h2l, 
                                    "h2l.cl");
    lide_ocl_kernel_set_cSourceFile(context->kernel_select, 
                                    "sort_select.cl");                                
    lide_ocl_kernel_set_cSourceFile(context->kernel_select_first, 
                                    "sort_select.cl");                                 
                                    
    lide_ocl_kernel_set_cPathAndName(context->kernel_sort, JITTER_OCL, 
                                    "BitonicSort.cl");	                                    
    lide_ocl_kernel_set_cPathAndName(context->kernel_h2l, JITTER_OCL, 
                                    "h2l.cl");
    lide_ocl_kernel_set_cPathAndName(context->kernel_select, JITTER_OCL, 
                                    "sort_select.cl");	
    lide_ocl_kernel_set_cPathAndName(context->kernel_select_first, JITTER_OCL, 
                                    "sort_select.cl");	
    
    context->gpu = gpu;
    /* GPU memory allocation*/
    //GPU settings
    context->gpu->szGlobalWorkSize = szGlobalWorkSize;
    context->gpu->szLocalWorkSize = szLocalWorkSize;
    context->cmDevSrcInput = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_float) * context->gpu->szGlobalWorkSize, 
                            NULL, &context->gpu->ciErr1);
    context->cmDevSrcIn = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_float) * Sort_NUM, 
                            NULL, &context->gpu->ciErr2);
    context->gpu->ciErr1 |= context->gpu->ciErr2; 
    
    context->cmDevIndex = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_uint) * Sort_NUM, 
                            NULL, &context->gpu->ciErr2);
    context->gpu->ciErr1 |= context->gpu->ciErr2; 
    
    context->cmDevSrcOut = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, 
                            sizeof(cl_float) * Sort_NUM, 
                            NULL, &context->gpu->ciErr2);
    context->gpu->ciErr1 |= context->gpu->ciErr2; 
    context->cmDevSrcM = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, sizeof(cl_float) * 1, NULL, 
                            &context->gpu->ciErr2);
     context->gpu->ciErr1 |= context->gpu->ciErr2;   
    context->cmDevSrcL = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, sizeof(cl_float) * 1, NULL, 
                            &context->gpu->ciErr2);
     context->gpu->ciErr1 |= context->gpu->ciErr2;   
    context->cmDevSrcH = clCreateBuffer(context->gpu->cxGPUContext, 
                            CL_MEM_READ_WRITE, sizeof(cl_float) * 1, NULL, 
                            &context->gpu->ciErr2);
     context->gpu->ciErr1 |= context->gpu->ciErr2;   

    if (context->gpu->ciErr1 != CL_SUCCESS)
    {
        fprintf(stderr, "Error in clCreateBuffer, Line %u in file %s !!!\n\n", 
                __LINE__, __FILE__);
        lide_ocl_gpu_cleanup(context->gpu);
        context->mode = LIDE_OCL_DVL_MODE_ERROR;
        exit(1);
    }
    /* First kernel*/	
    lide_ocl_kernel_load_source (context->kernel_sort, "");		
    lide_ocl_kernel_create_program(context->kernel_sort, 1);
    lide_ocl_kernel_build_program (context->kernel_sort, 0, NULL, NULL, NULL, 
                                    NULL);	
    /* Create the kernel */
    lide_ocl_kernel_create_kernel (context->kernel_sort, "BitonicSortF");
    
    /* Second kernel*/	
    lide_ocl_kernel_load_source (context->kernel_h2l, "");
    lide_ocl_kernel_create_program(context->kernel_h2l, 1);	
    lide_ocl_kernel_build_program (context->kernel_h2l, 0, NULL, NULL, NULL, 
                                    NULL);
    /* Create the kernel */
    lide_ocl_kernel_create_kernel (context->kernel_h2l, "h2l");
    
    /* Third kernel*/	
    lide_ocl_kernel_load_source (context->kernel_select, "");
    lide_ocl_kernel_create_program(context->kernel_select, 1);	
    lide_ocl_kernel_build_program (context->kernel_select, 0, NULL, NULL, NULL, 
                                    NULL);
    /* Create the kernel */
    lide_ocl_kernel_create_kernel (context->kernel_select, "sort_select");
    
    
    /* Forth kernel*/	
    lide_ocl_kernel_load_source (context->kernel_select_first, "");
    lide_ocl_kernel_create_program(context->kernel_select_first, 1);	
    lide_ocl_kernel_build_program (context->kernel_select_first, 0, NULL, NULL, NULL, 
                                    NULL);
    /* Create the kernel */
    lide_ocl_kernel_create_kernel (context->kernel_select_first, "sort_select_first");
/*******************************************************************************
Besides are related to optimization
*******************************************************************************/    
    /* Argument settings*/
    context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 4, 
                            sizeof(cl_int), 
                            //(void*)&context->gpu->szGlobalWorkSize);
                            (void*)&context->szGlobalWorkSize);
    if (context->gpu->ciErr1 != CL_SUCCESS)
    {
        fprintf(stderr, "Error %d in argument settings, Line %u in file %s \
                !!!\n\n", context->gpu->ciErr1, __LINE__, __FILE__);
        lide_ocl_gpu_cleanup(context->gpu);
        context->mode = LIDE_OCL_DVL_MODE_ERROR;
        exit(1);
    }

	/*Performance var*/
    context->t_dvl = 0.0;
    context->counter_dvl = 0;
    return context;
}

boolean lide_ocl_dvl_enable(
        lide_ocl_dvl_context_type *context) {
    boolean result = FALSE; 
    switch (context->mode) {
    case LIDE_OCL_DVL_MODE_ERROR:
        result = FALSE;
        break;
    case LIDE_OCL_DVL_MODE_PROCESS:
        result = (lide_ocl_fifo_population(context->x) >= 1) &&
                ((lide_ocl_fifo_population(context->volth) < 
                lide_ocl_fifo_capacity(context->volth))) &&
                ((lide_ocl_fifo_population(context->mid2trt) < 
                lide_ocl_fifo_capacity(context->mid2trt))) &&				
                ((lide_ocl_fifo_population(context->in2str) < 
                lide_ocl_fifo_capacity(context->in2str))) && 
                ((lide_ocl_fifo_population(context->in2trt) < 
                lide_ocl_fifo_capacity(context->in2trt)));
        break;
    default:
            result = FALSE;
            break;
    }  
    return result;
}

void lide_ocl_dvl_invoke(lide_ocl_dvl_context_type *context) {

    unsigned int i,j;

    size_t iNumElements = context->szGlobalWorkSize;
    size_t Sort_local;
    size_t Sort_global = context->szGlobalWorkSize;
    cl_mem in, out, tmp;
    /* For check*/
	/*
    float m, h, l;
    unsigned int *index;
    float *sorted_in;
    FILE *fp;
    index = (unsigned int *)lide_ocl_util_malloc(sizeof(unsigned int) * 
                                                    context->num_select);
    sorted_in = (float *)lide_ocl_util_malloc(sizeof(float) * 
                                                    context->num_select);
    */
    
    switch (context->mode) {
    case LIDE_OCL_DVL_MODE_ERROR:
        fprintf(stderr, "lide_ocl_dvl: error invalid status.\n");
        context->mode = LIDE_OCL_DVL_MODE_ERROR;
        exit(1);
        break;
    case LIDE_OCL_DVL_MODE_PROCESS:
		 context->counter_dvl = StartCounter();
        /* get input data */   
        lide_ocl_fifo_read(context->x, &context->cmDevSrcInput);
        
        /*Copy input data for sorting*/
        /*
        context->gpu->ciErr1 |= clEnqueueCopyBuffer (
                            context->gpu->cqCommandQueue, context->cmDevSrcIn, 
                            context->cmDevSrcInput, 0, 0, 
                            sizeof(cl_float) * context->gpu->szGlobalWorkSize,
                            0, NULL, NULL); 
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueWriteBuffer, Line %u in file \
                    %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        /* Select first one for sorting*/
        /* Select Data for Sorting*/
        Sort_local = 1;
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select_first->ckKernel, 
                                                0, sizeof(cl_mem), 
                                                (void*)&context->cmDevIndex);

        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select_first->ckKernel, 
                                                1, sizeof(cl_uint), 
                                                (void*)&context->Ws);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select_first->ckKernel, 
                                                2, sizeof(cl_uint), 
                                                (void*)&context->num_select);
        /* Launch kernel */
        //printf("%d\t%d\t%d\n",context->PM_num,context->Ws,context->num_select);
        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_select_first->ckKernel, 1, 
                                NULL, &Sort_local, &Sort_local, 0, 
                                NULL, NULL);
        //clFinish (context->gpu->cqCommandQueue);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u \
            in file %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        
        
        /* Select Data for Sorting*/
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select->ckKernel, 
                                                0, sizeof(cl_mem), 
                                                (void*)&context->cmDevSrcInput);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select->ckKernel, 
                                                1, sizeof(cl_mem), 
                                                (void*)&context->cmDevSrcIn);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select->ckKernel, 
                                                2, sizeof(cl_mem), 
                                                (void*)&context->cmDevIndex);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select->ckKernel, 
                                                3, sizeof(cl_uint), 
                                                (void*)&context->PM_num);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select->ckKernel, 
                                                4, sizeof(cl_uint), 
                                                (void*)&context->Ws);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_select->ckKernel, 
                                                5, sizeof(cl_uint), 
                                                (void*)&context->num_select);
        /* Launch kernel */
        //printf("%d\t%d\t%d\n",context->PM_num,context->Ws,context->num_select);
        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_select->ckKernel, 1, 
                                NULL, &context->gpu->szGlobalWorkSize, 
                                &context->gpu->szLocalWorkSize, 0, 
                                NULL, NULL);
        
        //clFinish (context->gpu->cqCommandQueue);
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u \
            in file %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }
        */
        /* Check Here:*/
		/*
        context->gpu->ciErr1 = clEnqueueReadBuffer(context->gpu->cqCommandQueue, 
                                                context->cmDevSrcIn, CL_TRUE, 0, 
                                                sizeof(cl_float) * context->num_select, 
                                                sorted_in, 0, NULL, NULL);
        context->gpu->ciErr1 |= clEnqueueReadBuffer(context->gpu->cqCommandQueue, 
                                                context->cmDevIndex, CL_TRUE, 0, 
                                                sizeof(cl_uint) * context->num_select, 
                                                index, 0, NULL, NULL);                                        
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u \
            in file %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }                                       
        clFinish (context->gpu->cqCommandQueue);
        fp = fopen("sort_in.txt","w");
        
        for(i = 0; i < context->num_select; i++){
            fprintf(fp, "%lf\n", sorted_in[i]);
        }
        fclose(fp);
        
        fp = fopen("sort_index.txt","w");
        for(i = 0; i < context->num_select; i++){
            fprintf(fp, "%u\n", index[i]);
        }
        fclose(fp);
        */         
        /* sort:ascending order */
        clFinish (context->gpu->cqCommandQueue);

        in = context->cmDevSrcIn;
        out = context->cmDevSrcOut;

        for (i = 1; i<= context->level; i++){
            context->col = i;
            context->dir = 1<<context->col;
            context->gpu->ciErr1 |= clSetKernelArg(
                                    context->kernel_sort->ckKernel, 3, 
                                    sizeof(cl_uint), (void*)&context->dir);
            for (j = 1; j <=context->col; j++){
                context->inc = 1 << (context->col-j);
                Sort_local = 2;
                context->gpu->ciErr1 |= clSetKernelArg(
                                        context->kernel_sort->ckKernel, 0, 
                                        sizeof(cl_mem), (void*)&in);
                context->gpu->ciErr1 |= clSetKernelArg(
                                        context->kernel_sort->ckKernel, 1, 
                                        sizeof(cl_mem), (void*)&out);
                context->gpu->ciErr1 |= clSetKernelArg(
                                        context->kernel_sort->ckKernel, 2, 
                                        sizeof(cl_uint), (void*)&context->inc);
                /*
                if (context->gpu->ciErr1 != CL_SUCCESS)
                {
                    fprintf(stderr, "Error in clSetKernelArg, Line %u in file \
                            %s !!!\n\n", __LINE__, __FILE__);
                    lide_ocl_gpu_cleanup(context->gpu);
                }
                */
                /* Launch kernel */
                context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                        context->gpu->cqCommandQueue, 
                                        context->kernel_sort->ckKernel, 1, 
                                        NULL, &Sort_global, &Sort_local, 0, 
                                        NULL, NULL);
                /*
                if (context->gpu->ciErr1 != CL_SUCCESS)
                {
                    fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u \
                    in file %s !!!\n\n", __LINE__, __FILE__);
                    lide_ocl_gpu_cleanup(context->gpu);
                    exit(1);
                }
                */
                tmp = in;
                in = out;
                out = tmp;
            }
        }
        //clFinish (context->gpu->cqCommandQueue);
        
        /* Check Here:*/
		/*
        context->gpu->ciErr1 = clEnqueueReadBuffer(context->gpu->cqCommandQueue, 
                                                in, CL_TRUE, 0, 
                                                sizeof(cl_float) * context->num_select, 
                                                sorted_in, 0, NULL, NULL);
                                       
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clEnqueueNDRangeKernel, Line %u \
            in file %s !!!\n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }                                       
        clFinish (context->gpu->cqCommandQueue);
        fp = fopen("sorted_in.txt","w");
        
        for(i = 0; i < context->num_select; i++){
            fprintf(fp, "%lf\n", sorted_in[i]);
        }
        fclose(fp);
        */
               
        /* Set the Argument values */
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 
                                                0, sizeof(cl_mem), (void*)&in);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 
                                                1, sizeof(cl_mem), 
                                                (void*)&context->cmDevSrcM);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 
                                                2, sizeof(cl_mem), 
                                                (void*)&context->cmDevSrcL);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 
                                                3, sizeof(cl_mem), 
                                                (void*)&context->cmDevSrcH);
        /*                                        
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 
                                                5, sizeof(cl_int), 
                                                (void*)&context->NUM);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 
                                                6, sizeof(cl_int), 
                                                (void*)&context->Residue);
        context->gpu->ciErr1 |= clSetKernelArg(context->kernel_h2l->ckKernel, 
                                                7, sizeof(cl_int), 
                                                (void*)&context->index);
        */
        /*
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in clSetKernelArg, Line %u in file %s !!!\
                    \n\n", __LINE__, __FILE__);
            lide_ocl_gpu_cleanup(context->gpu);
            exit(1);
        }	
        */        
        /* Launch kernel*/
        Sort_local = 1;
        //clFinish (context->gpu->cqCommandQueue);
        context->gpu->ciErr1 |= clEnqueueNDRangeKernel(
                                context->gpu->cqCommandQueue, 
                                context->kernel_h2l->ckKernel, 1, NULL, 
                                //&context->gpu->szGlobalWorkSize, 
                                &Sort_local,
                                &Sort_local, 0, NULL, NULL);
                                //&context->gpu->szLocalWorkSize, 0, NULL, NULL);
        //clFinish (context->gpu->cqCommandQueue);  
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

        /*Synchronous/blocking read of results, and check accumulated errors*/
        // Need to be commented during performance testing
        /*
        context->gpu->ciErr1 = clEnqueueReadBuffer(context->gpu->cqCommandQueue, 
                                                context->cmDevSrcM, CL_TRUE, 0, 
                                                sizeof(cl_float) * 1, &m, 
                                                0, NULL, NULL);
        context->gpu->ciErr1 = clEnqueueReadBuffer(context->gpu->cqCommandQueue, 
                                                context->cmDevSrcL, CL_TRUE, 0, 
                                                sizeof(cl_float) * 1, &l, 
                                                0, NULL, NULL);	
        context->gpu->ciErr1 = clEnqueueReadBuffer(context->gpu->cqCommandQueue, 
                                                context->cmDevSrcH, CL_TRUE, 0, 
                                                sizeof(cl_float) * 1, &h, 
                                                0, NULL, NULL);	

        printf("m:%f  h:%f  l:%f\n", m, h, l);
        */
        clFinish (context->gpu->cqCommandQueue);
        if (context->gpu->ciErr1 != CL_SUCCESS)
        {
            fprintf(stderr, "Error in DVL: NUM:%d\n", context->index);
            fprintf(stderr, "Error %d in DVL, Line %u in file %s !!!\n\n", 
                context->gpu->ciErr1, __LINE__, __FILE__);
            //lide_ocl_gpu_cleanup(context->gpu);
            context->mode = LIDE_OCL_DVL_MODE_ERROR;
            context->index = context->index+1;
            context->t_dvl += GetCounter(context->counter_dvl);
            
            break;
        }
        
        
        lide_ocl_fifo_write(context->volth, &context->cmDevSrcM);
        lide_ocl_fifo_write(context->mid2trt, &context->cmDevSrcM);
        lide_ocl_fifo_write(context->in2str, &context->cmDevSrcInput);
        lide_ocl_fifo_write(context->in2trt, &context->cmDevSrcInput);        
        context->index = context->index+1;
		context->t_dvl += GetCounter(context->counter_dvl);

        break;
    default:
        context->mode = LIDE_OCL_DVL_MODE_PROCESS;
        break;
    }

    return;
}

void lide_ocl_dvl_terminate(
        lide_ocl_dvl_context_type *context) {
    printf("time in DVL:%.6lf\n", context->t_dvl);
    clReleaseMemObject(context->cmDevSrcInput);
    clReleaseMemObject(context->cmDevSrcIn);
    clReleaseMemObject(context->cmDevSrcOut);
    clReleaseMemObject(context->cmDevSrcM);
    clReleaseMemObject(context->cmDevSrcH);
    clReleaseMemObject(context->cmDevSrcL);
    
    clReleaseKernel(context->kernel_sort->ckKernel);
    clReleaseProgram(context->kernel_sort->cpProgram);
    free(context->kernel_sort);
    
    clReleaseKernel(context->kernel_h2l->ckKernel);
    clReleaseProgram(context->kernel_h2l->cpProgram);
    free(context->kernel_h2l);	
    free(context);
}



