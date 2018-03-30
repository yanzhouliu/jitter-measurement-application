__kernel void sort_select_first(__global unsigned int *index, unsigned int ws, 
                            unsigned int num)
{
	
    // get index into global data array
    unsigned int iGID = get_global_id(0);
    if (iGID < num){
        index[0] = index[num-1]%ws;
    }
    return;

}


__kernel void sort_select(__global float* x, __global float* y, 
                            __global unsigned int *index, unsigned int PM, 
                            unsigned int ws, unsigned int num)
{
	
    // get index into global data array
	unsigned int tmp,tmp3;
    unsigned int iGID = get_global_id(0);
    float tmp1,tmp2;
    if (iGID < num){
        
        tmp3 = index[0];
        tmp = (iGID*PM+tmp3)%ws; 
        y[iGID] = x[tmp];
        index[iGID] = tmp;
    }else{
        y[iGID] = 0;
        index[iGID] = 0;
    }
    return;

}

