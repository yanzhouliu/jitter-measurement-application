// OpenCL Kernel Function for element by element vector addition
__kernel void h2l(__global float* x, __global float* M, __global float* L, 
                    __global float* H, int length)
{
	
    // get index into global data array
	int hindex, lindex, tmp;
    int iGID = get_global_id(0);
	if (iGID == 0){

        lindex = (int)(0.01*length);
        hindex = length - lindex;
    
		*H = x[hindex];
		*L = x[lindex];
		*M = (*H+*L)/2.0;
	}
	

}

