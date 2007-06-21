int X_ENABLED=0;
int Y_ENABLED=0;
int xcount=0;
int ycount=0;
float lastx;
float lasty;
int totalx=0;
int totaly=0;
float slopex;
float slopey;
 
void LSC_transthr(double *in, int inSize, double *out, int outSize)
{
out[4] = 0;
if( !X_ENABLED && in[0] > pLocalEpics->pde.LSC_X_HIGH)
{
	X_ENABLED=1;
//	slopex = in[0]-lastx;
//	xcount = 0;
//	totalx=0;
}
else if(X_ENABLED && in[0]<pLocalEpics->pde.LSC_X_LOW)	
{
	X_ENABLED=0;
//	out[4] = totalx;
}
if(!Y_ENABLED && in[1] > pLocalEpics->pde.LSC_Y_HIGH)
{
        Y_ENABLED=1;
//	slopey = in[0]-lasty;
//	ycount = 0;
//	totaly=0;
}
else if(Y_ENABLED && in[1]<pLocalEpics->pde.LSC_Y_LOW)
{
        Y_ENABLED=0;
//	out[4]=totaly;
}

out[2] = 0;
out[3] = 0;
/*
if(X_ENABLED)
{
	totalx++;
	if(xcount < pLocalEpics->pde.LSC_IMP_LEG)
	{
        	xcount++;
	        out[2] = pLocalEpics->pde.LSC_IMP_MAG*slopex;
	}
}
if(Y_ENABLED)
{
        totaly++;
        if(ycount < pLocalEpics->pde.LSC_IMP_LEG)
        {
                ycount++;
                out[3] = pLocalEpics->pde.LSC_IMP_MAG*slopey;
        }
}

*/

out[0] = X_ENABLED ? in[0] : 0;
out[1] = Y_ENABLED ? in[1] : 0;
lastx = in[0];
lasty = in[1];

}
