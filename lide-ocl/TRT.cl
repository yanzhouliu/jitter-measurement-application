#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void TRT(__global float* a, __global int* b, __global float* th, __global double* c, int iNumElements)
{
    // get index into global data array
    int iGID = get_global_id(0);
	
    // bound check (equivalent to the limit on a 'for' loop for standard/serial C code
    if (iGID >= iNumElements)
    {   
        return; 
    }
    
    // add the vector elements
	c[iGID] = 0.0;
	if( (b[iGID] == 1) && (iGID < iNumElements-1)){
		c[iGID] = (double)(1.0 + (double)iGID + fabs((a[iGID] - *th)/(a[iGID] - a[iGID+1])));
	}

}
