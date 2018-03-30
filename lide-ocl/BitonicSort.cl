#pragma OPENCL EXTENSION cl_khr_fp64 : enable
/*  BitonicSortD: bitonic sort for double data type */
/*
   in: input
   out: output
   inc: help find compare pair
   dir: help find ascend or descend
*/
__kernel void BitonicSortD(__global double *in, __global double *out, 
                            const uint inc, const uint dir)
{
    unsigned int iGID = get_global_id(0);
    unsigned int iLID = get_local_id(0);
    unsigned int jGID = iGID ^ inc;
    bool small, swchID;
    bool rev = ((iGID & dir) != 0);
    double x0, x1;
    
    x0 = in[iGID];
    x1 = in[jGID];
    small = (x1 < x0) || (x1 == x0 && jGID < iGID);
    swchID = small ^ (jGID < iGID) ^ rev;
    
    out[iGID] = swchID ? x1 : x0;
}

__kernel void BitonicSortF(__global float *in, __global float *out, 
                            const uint inc, const uint dir)
{
    unsigned int iGID = get_global_id(0);
    unsigned int iLID = get_local_id(0);
    unsigned int jGID = iGID ^ inc;
    bool small, swchID;
    bool rev = ((iGID & dir) != 0);
    float x0, x1;
    
    x0 = in[iGID];
    x1 = in[jGID];
//    barrier (CLK_GLOBAL_MEM_FENCE);
    small = (x1 < x0) || (x1 == x0 && jGID < iGID);
    swchID = small ^ (jGID < iGID) ^ rev;
  //  barrier (CLK_GLOBAL_MEM_FENCE);
    out[iGID] = swchID ? x1 : x0;
}