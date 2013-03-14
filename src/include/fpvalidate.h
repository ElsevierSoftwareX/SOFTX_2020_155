#define fpvalidate(a) a
/*
INLINE float 
fpvalidate(float p) {
 float f = p;

 if (((*((unsigned long *)&f)) & 0x7f800000) == 0x7f800000)
 	return 0.000000000001;
 if(f > 1e20) f = 1e20;
 if(f < -1e20) f = -1e20;
  
 return f;
}
*/
