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
if( !X_ENABLED && in[0] > pLocalEpics->pde.LSC_X_HIGH)
{
	X_ENABLED=1;
}
else if(X_ENABLED && in[0]<pLocalEpics->pde.LSC_X_LOW)	
{
	X_ENABLED=0;
}
if(!Y_ENABLED && in[1] > pLocalEpics->pde.LSC_Y_HIGH)
{
        Y_ENABLED=1;
}
else if(Y_ENABLED && in[1]<pLocalEpics->pde.LSC_Y_LOW)
{
        Y_ENABLED=0;
}

out[0] = X_ENABLED ? in[0] : 0;
out[1] = Y_ENABLED ? in[1] : 0;
lastx = in[0];
lasty = in[1];

}
