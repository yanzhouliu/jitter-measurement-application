  #pragma OPENCL EXTENSION cl_khr_fp64 : enable
 // OpenCL Kernel Function for element by element vector addition
__kernel void prefix_sum_old(__global int* x, __global int* result, unsigned int length, unsigned int EXP_2)
{
	
   // get index into global data array
    unsigned int iGID = get_global_id(0);
	unsigned int offset, len, i;
	unsigned int index1;
	unsigned int index2;
	//check boundary
	offset = 0;
	len = 0;
	if (iGID<length){
		result[iGID] = x[iGID];		
	}else{
		result[iGID] = 0;
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
	//Up-sweep
	for(i = 1; i<=EXP_2; i = i+1){
		len = 1<<(EXP_2-i);
		offset = 1<<i;
		barrier(CLK_GLOBAL_MEM_FENCE);
		if(iGID<len){
			index1 = iGID*offset + offset/2 -1;
			index2 = iGID*offset + offset -1;
			result[index2] = result[index1] + result[index2];
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
	for(i = EXP_2-1; i>0; i = i-1){
		len = (1<<(EXP_2-i))-1;
		offset = 1<<i;
		barrier(CLK_GLOBAL_MEM_FENCE);
		if(iGID<len){
			index1 = iGID*offset + offset -1;
			index2 = iGID*offset + offset -1 + offset/2;
			result[index2] = result[index1] + result[index2];
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}



__kernel void prefix_sum_sub(__global int* x, __global int* result, __global int* sub_sum, int length, int EXP_2)
{
	
   // get index into global data array
    int iGID = get_global_id(0);
    int iLID = get_local_id(0);
    int GID = get_group_id(0);
    int GSIZE = get_local_size(0);
	int offset, len, i;
	int index1;
	int index2;
	//check boundary
	offset = 0;
	len = 0;
	if (iGID<length){
		result[iGID] = x[iGID];		
	}else{
		result[iGID] = 0;
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
	//Up-sweep
	for(i = 1; i<=EXP_2; i = i+1){
		len = 1<<(EXP_2-i);
		offset = 1<<i;
        
		if(iLID<len){
			index1 = iLID*offset + offset/2 -1 + GID*GSIZE;
			index2 = iLID*offset + offset -1 + GID*GSIZE;
			result[index2] = result[index1] + result[index2];
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}

	for(i = EXP_2-1; i>0; i = i-1){
		len = (1<<(EXP_2-i))-1;
		offset = 1<<i;

		if(iLID<len){
			index1 = iLID*offset + offset -1 + GID*GSIZE;
			index2 = iLID*offset + offset -1 + offset/2 + GID*GSIZE;
			result[index2] = result[index1] + result[index2];
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}

    if (iLID == 0){
        sub_sum[GID] = result[GSIZE*GID+GSIZE-1];
    }
}


/*One group. local size = global size*/
__kernel void prefix_sum_sub_sum(__global int* result, int length, int EXP_2)
{
	
   // get index into global data array
    int iGID = get_global_id(0);

	int offset, len, i;
	int index1;
	int index2;
	//check boundary
	offset = 0;
	len = 0;
	barrier(CLK_GLOBAL_MEM_FENCE);
	//Up-sweep
	for(i = 1; i<=EXP_2; i = i+1){
		len = 1<<(EXP_2-i);
		offset = 1<<i;
		barrier(CLK_GLOBAL_MEM_FENCE);
		if(iGID<len){
			index1 = iGID*offset + offset/2 -1;
			index2 = iGID*offset + offset -1;
			result[index2] = result[index1] + result[index2];
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
	for(i = EXP_2-1; i>0; i = i-1){
		len = (1<<(EXP_2-i))-1;
		offset = 1<<i;
		barrier(CLK_GLOBAL_MEM_FENCE);
		if(iGID<len){
			index1 = iGID*offset + offset -1;
			index2 = iGID*offset + offset -1 + offset/2;
			result[index2] = result[index1] + result[index2];
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
  
}



__kernel void prefix_offset_sum(__global int* result, __global int* sub_sum, int length)
{
    int iGID = get_global_id(0);
    int GID = get_group_id(0);    
    if (iGID >= length || GID == 0){
        return;
    } else{
        result[iGID] = result[iGID] + sub_sum[GID-1];
    }
}


__kernel void stream_cmpt_accumulated(__global double* x, __global int* pred, __global int* pred_scan, 
					__global double* cmpt, __global int* length, unsigned int Ws, unsigned int ws_index)
{
	// get index into global data array
    int iGID = get_global_id(0);
	int globalsize = get_global_size(0);
	int index = pred_scan[iGID]-1+length[0];
    double offset = (double)Ws*ws_index;
	if(iGID == (globalsize-1)){
		length[0] = length[0] + pred_scan[iGID];
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
	if(pred[iGID] == 1){
		cmpt[index] = x[iGID] + offset;
	}
}	

__kernel void stream_cmpt(__global double* x, __global int* pred, __global int* pred_scan, 
					__global double* cmpt, __global int* length, unsigned int Ws, unsigned int ws_index)
{
	// get index into global data array
    int iGID = get_global_id(0);
	int globalsize = get_global_size(0);
	int index = pred_scan[iGID]-1;
    double offset = (double)Ws*ws_index;

	if(pred[iGID] == 1){
		cmpt[index] = x[iGID];
	}

}	


__kernel void stream_cmpt_2(__global double* x, __global int* pred, __global int* pred_scan, 
					__global double* cmpt, __global int* length, unsigned int Ws, unsigned int ws_index){
    int iGID = get_global_id(0);
    int globalsize = get_global_size(0);
    if(iGID == (globalsize-1)){
		length[0] = pred_scan[iGID];
	}
}	