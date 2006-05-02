#ifndef FM_FIR_HEADER_INCLUDED
#define FM_FIR_HEADER_INCLUDED

#define FIR_TAPS	512


typedef struct PP_FIR{
	int input;
	float gain;
	float limit;
	float offset;
	int limitSw;
	int offsetSw;
	int clearHistory;
	int onOffSw;
	int status;
	double output;
	int dsFiltNum;
	int cmpFiltNum;
	int usFiltNum;
}PP_FIR;

#if 0
float firCoeff[FIR_TAPS] = {
 9.38728953e-001, -4.47367542e-002, -5.88502680e-002, -7.40053485e-002, -8.95581758e-002,
-1.04812322e-001, -1.18983146e-001, -1.31205820e-001, -1.40697189e-001, -1.46645863e-001,
-1.48457737e-001, -1.45601117e-001, -1.37842992e-001, -1.25107495e-001, -1.07634560e-001,
-8.58856487e-002, -6.05883561e-002, -3.26944391e-002, -3.31285452e-003,  2.62946939e-002,
 5.48531762e-002,  8.10642363e-002,  1.03760055e-001,  1.21901651e-001,  1.34692688e-001,
 1.41599008e-001,  1.42386276e-001,  1.37156870e-001,  1.26301382e-001,  1.10535809e-001,
 9.07854096e-002,  6.82031852e-002,  4.40255784e-002,  1.95602542e-002, -3.94226873e-003,
-2.53319083e-002, -4.36378727e-002, -5.81296695e-002, -6.83311577e-002, -7.40755227e-002,
-7.54598858e-002, -7.28702959e-002, -6.69000806e-002, -5.83313105e-002, -4.80484196e-002,
-3.69779210e-002, -2.60252999e-002, -1.59897474e-002, -7.54111242e-003, -1.13669099e-003,
 2.95892964e-003,  4.72752668e-003,  4.34515646e-003,  2.18949175e-003, -1.22500365e-003,
-5.28611461e-003, -9.34118496e-003, -1.27667997e-002, -1.50041900e-002, -1.56333213e-002,
-1.43802957e-002, -1.11678094e-002, -6.09690840e-003,  5.45381806e-004,  8.32038610e-003,
 1.66759689e-002,  2.49839644e-002,  3.26107930e-002,  3.89463338e-002,  4.34827088e-002,
 4.58269475e-002,  4.57611504e-002,  4.32360957e-002,  3.83929857e-002,  3.15403391e-002,
 2.31319384e-002,  1.37337130e-002,  3.96821395e-003, -5.52002403e-003, -1.41322353e-002,
-2.13436433e-002, -2.67588645e-002, -3.01274478e-002, -3.13672431e-002, -3.05639960e-002,
-2.79549502e-002, -2.39136890e-002, -1.89007700e-002, -1.34392054e-002, -8.04995615e-003,
-3.22579126e-003,  6.26510257e-004,  3.20565616e-003,  4.34836282e-003,  4.03532473e-003,
 2.38929762e-003, -3.35465790e-004, -3.78621545e-003, -7.53640253e-003, -1.11357548e-002,
-1.41421150e-002, -1.61697344e-002, -1.69188428e-002, -1.62050007e-002, -1.39771565e-002,
-1.03180195e-002, -5.44466665e-003,  3.19664911e-004,  6.56239089e-003,  1.28280050e-002,
 1.86461702e-002,  2.35804924e-002,  2.72579716e-002,  2.94045797e-002,  2.98654920e-002,
 2.86169965e-002,  2.57695634e-002,  2.15537664e-002,  1.63040150e-002,  1.04250286e-002,
 4.36102817e-003, -1.44448179e-003, -6.58413684e-003, -1.07195131e-002, -1.36097092e-002,
-1.51258379e-002, -1.52635167e-002, -1.41345024e-002, -1.19593402e-002, -9.03932973e-003,
-5.73277889e-003, -2.41700561e-003,  5.42479358e-004,  2.82748321e-003,  4.19349223e-003,
 4.49178345e-003,  3.68154852e-003,  1.83236545e-003, -8.82760555e-004, -4.20432336e-003,
-7.81034309e-003, -1.13459102e-002, -1.44559779e-002, -1.68166573e-002, -1.81655169e-002,
-1.83236618e-002, -1.72135371e-002, -1.48641943e-002, -1.14110942e-002, -7.08248758e-003,
-2.18202148e-003,  2.93955556e-003,  7.91201410e-003,  1.23794258e-002,  1.60275298e-002,
 1.86121785e-002,  1.99764906e-002,  2.00652004e-002,  1.89255006e-002,  1.67032157e-002,
 1.36269122e-002,  9.98782691e-003,  6.11268600e-003,  2.33493495e-003, -1.03435265e-003,
-3.73345044e-003, -5.57133476e-003, -6.44329293e-003, -6.33810190e-003, -5.33672931e-003,
-3.60392107e-003, -1.37010532e-003,  1.08874744e-003,  3.48013789e-003,  5.51901826e-003,
 6.95793985e-003,  7.60584924e-003,  7.34929633e-003,  6.15925789e-003,  4.09664179e-003,
 1.30472703e-003, -2.00237347e-003, -5.55920244e-003, -9.07300243e-003, -1.22486453e-002,
-1.48161578e-002, -1.65527929e-002, -1.73038071e-002, -1.69946892e-002, -1.56375966e-002,
-1.33304573e-002, -1.02466010e-002, -6.62185770e-003, -2.73064540e-003,  1.13394262e-003,
 4.68962283e-003,  7.68451389e-003,  9.92221331e-003,  1.12739687e-002,  1.16911153e-002,
 1.12055658e-002,  9.92609394e-003,  8.02696727e-003,  5.73020790e-003,  3.28603861e-003,
 9.48023327e-004, -1.04832124e-003, -2.50813505e-003, -3.29347223e-003, -3.33523346e-003,
-2.63927387e-003, -1.28332120e-003,  5.88424919e-004,  2.78233327e-003,  5.06977792e-003,
 7.21235058e-003,  8.97990266e-003,  1.01753696e-002,  1.06496728e-002,  1.03177788e-002,
 9.16480507e-003,  7.24828945e-003,  4.69309532e-003,  1.67996101e-003, -1.56928848e-003,
-4.81216472e-003, -7.80533395e-003, -1.03264042e-002, -1.21924176e-002, -1.32750278e-002,
-1.35116803e-002, -1.29086478e-002, -1.15415334e-002, -9.54509607e-003, -7.10354241e-003,
-4.43065595e-003, -1.75378650e-003,  7.09115260e-004,  2.76582413e-003,  4.26839952e-003,
 5.12366285e-003,  5.30145525e-003,  4.83518838e-003,  3.81770322e-003,  2.39152773e-003,
 7.34680423e-004, -9.56081799e-004, -2.48391696e-003, -3.67011442e-003, -4.37035708e-003,
-4.48840442e-003, -3.98399343e-003, -2.87754227e-003, -1.24665835e-003,  7.79266066e-004,
 3.03287202e-003,  5.32173795e-003,  7.44793730e-003,  9.22330242e-003,  1.04884168e-002,
 1.11244470e-002,  1.10655620e-002,  1.03027665e-002,  8.88596529e-003,  6.91812831e-003,
 4.54687501e-003,  1.95089671e-003, -6.75405096e-004, -3.13864347e-003, -5.26304533e-003,
-6.90491300e-003, -7.96450651e-003, -8.39376086e-003, -8.19903913e-003, -7.43999983e-003,
-6.22192431e-003, -4.68689047e-003, -2.99811669e-003, -1.32738399e-003,  1.63440358e-004,
 1.33374466e-003,  2.07956179e-003,  2.33944384e-003,  2.10197060e-003,  1.40390857e-003,
 3.27678955e-004, -1.00771887e-003, -2.45788443e-003, -3.86623748e-003, -5.07934695e-003,
-5.95991686e-003, -6.40045337e-003, -6.33205239e-003, -5.73125908e-003, -4.62226752e-003,
-3.07408673e-003, -1.19599564e-003,  8.74479538e-004,  2.98060606e-003,  4.96374336e-003,
 6.67308649e-003,  7.98249246e-003,  8.79721459e-003,  9.06489596e-003,  8.77689077e-003,
 7.97014543e-003,  6.72249026e-003,  5.14523039e-003,  3.37374592e-003,  1.55396846e-003,
-1.68764945e-004, -1.66393487e-003, -2.82536187e-003, -3.58065654e-003, -3.89659543e-003,
-3.78019468e-003, -3.27885149e-003, -2.47235124e-003, -1.46830879e-003, -3.87628799e-004,
 6.42621478e-004,  1.50535145e-003,  2.10014898e-003,  2.35564530e-003,  2.23317622e-003,
 1.73165504e-003,  8.86725245e-004, -2.32861986e-004, -1.53073681e-003, -2.89313612e-003,
-4.19769075e-003, -5.32596814e-003, -6.17295802e-003, -6.65681946e-003, -6.72625060e-003,
-6.36362334e-003, -5.58833167e-003, -4.45208448e-003, -3.03765730e-003, -1.44777714e-003,
 1.99892387e-004,  1.78672969e-003,  3.19974639e-003,  4.34393259e-003,  5.14758656e-003,
 5.56935613e-003,  5.60003085e-003,  5.26278939e-003,  4.61045681e-003,  3.71978634e-003,
 2.68458343e-003,  1.60665463e-003,  5.86643082e-004, -2.84644918e-004, -9.34814676e-004,
-1.31468683e-003, -1.40363597e-003, -1.20869924e-003, -7.65442366e-004, -1.31646091e-004,
 6.15913929e-004,  1.39138148e-003,  2.10538042e-003,  2.67576627e-003,  3.03280313e-003,
 3.12774101e-003,  2.93490808e-003,  2.45596490e-003,  1.71755713e-003,  7.70710082e-004,
-3.15492756e-004, -1.45849920e-003, -2.57073990e-003, -3.56634210e-003, -4.36907363e-003,
-4.91860709e-003, -5.17537735e-003, -5.12385402e-003, -4.77309373e-003, -4.15554902e-003,
-3.32412125e-003, -2.34640350e-003, -1.30039283e-003, -2.65151308e-004,  6.82356109e-004,
 1.47813196e-003,  2.07113068e-003,  2.43275610e-003,  2.55288791e-003,  2.44533891e-003,
 2.14109711e-003,  1.68841523e-003,  1.14580228e-003,  5.77007193e-004,  4.61735096e-005,
-3.90454686e-004, -6.85906337e-004, -8.10738810e-004, -7.51128729e-004, -5.13281300e-004,
-1.20124133e-004,  3.90753644e-004,  9.68916771e-004,  1.56009825e-003,  2.10435310e-003,
 2.55026082e-003,  2.84914215e-003,  2.97052080e-003,  2.89386868e-003,  2.62016089e-003,
 2.16381009e-003,  1.55618639e-003,  8.40860214e-004,  6.80235742e-005, -7.04996562e-004,
-1.42608938e-003, -2.04330861e-003, -2.51901396e-003, -2.82345692e-003, -2.94434827e-003,
-2.88311775e-003, -2.65456932e-003, -2.28920477e-003, -1.82135308e-003, -1.29881134e-003,
-7.62410199e-004, -2.60771848e-004,  1.72346411e-004,  5.03323813e-004,  7.15786808e-004,
 8.01462164e-004,  7.65241371e-004,  6.25132332e-004,  4.03302562e-004,  1.35299616e-004,
-1.48894473e-004, -4.10293578e-004, -6.21542954e-004, -7.54312130e-004, -7.92279690e-004,
-7.27934425e-004, -5.60642580e-004, -3.06619925e-004,  1.99317814e-005,  3.85190224e-004,
 7.66307678e-004,  1.12444005e-003,  1.43638382e-003,  1.67178139e-003,  1.81425680e-003,
 1.85189545e-003,  1.77990648e-003,  1.60838659e-003,  1.34526520e-003,  1.01802826e-003,
 6.44088886e-004,  2.57156396e-004, -1.21430464e-004, -4.63985151e-004, -7.50227192e-004,
-9.67268839e-004, -1.10006024e-003, -1.15437135e-003, -1.12405948e-003, -1.03126653e-003,
-8.79874177e-004, -6.99345978e-004, -5.00943447e-004, -3.09499773e-004, -1.39569180e-004,
-2.74145135e-006,  8.68101407e-005,  1.33346241e-004,  1.27068471e-004,  8.51246883e-005,
 5.70772238e-006, -8.93875107e-005, -1.93224879e-004, -2.90243887e-004, -3.66508330e-004,
-4.20061526e-004, -4.32322874e-004, -4.15590006e-004, -3.50975514e-004, -2.61191079e-004,
-1.34174443e-004,  4.07052780e-006,  1.53712623e-004,  2.96213047e-004,  4.20759723e-004,
 5.25318502e-004,  5.91805608e-004,  6.34236447e-004,  6.31415393e-004,  6.04761638e-004,
 5.38205471e-004,  4.50952798e-004,  3.43398579e-004,  2.22956102e-004,  1.11561991e-004,
-5.26187351e-006, -8.84551994e-005, -1.76343929e-004, -2.19924355e-004, -2.68363828e-004,
-2.72762479e-004, -2.74662980e-004, -2.46406513e-004, -2.07609715e-004, -1.77683049e-004,
-1.28427556e-004, -1.35479102e-004};
#endif

#endif
