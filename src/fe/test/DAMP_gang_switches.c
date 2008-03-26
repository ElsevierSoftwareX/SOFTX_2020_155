// Sample code
// Gang FM1 in DAMP_V1 and DAMP_V2

static const fmask = 0x10; // FM1 request bit, use ezcaread on SW1R to get bits

inline void
DAMP_gang_switches(double *ins , int nins, double *outs, int nouts)
{

  unsigned int v1 = dsp_ptr->inputs[DAMP_V1].opSwitchE;
  unsigned int v2 = dsp_ptr->inputs[DAMP_V2].opSwitchE;
  unsigned int sw = (unsigned int)ins[2];

  if (sw) { // turn filter on
  	dsp_ptr->inputs[DAMP_V1].opSwitchE |= fmask;
  	dsp_ptr->inputs[DAMP_V2].opSwitchE |= fmask;
  } else { // clear
  	dsp_ptr->inputs[DAMP_V1].opSwitchE &= ~fmask;
  	dsp_ptr->inputs[DAMP_V2].opSwitchE &= ~fmask;
  }

  // pass signals through
  outs[0] = ins[0];
  outs[1] = ins[1];
}
