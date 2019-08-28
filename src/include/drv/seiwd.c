/* ***********************************************************************************
 */
/* seiwd() */
/* Main SEI watchdog routine. */
/* ***********************************************************************************
 */

inline int seiwd(float input_sensors[], int max_var[]) {
  int watchdog_byte;

  watchdog_byte = 0;

  /* Compare the STS sensor inputs */
  if (input_sensors[0] > max_var[0])
    watchdog_byte |= 0x1; /* Bit 0 */
  if (input_sensors[1] > max_var[0])
    watchdog_byte |= 0x2; /* Bit 1 */
  if (input_sensors[2] > max_var[0])
    watchdog_byte |= 0x4; /* Bit 2 */

  /* Compare the POS V sensor inputs */
  if (input_sensors[3] > max_var[1])
    watchdog_byte |= 0x8; /* Bit 3 */
  if (input_sensors[4] > max_var[1])
    watchdog_byte |= 0x10; /* Bit 4 */
  if (input_sensors[5] > max_var[1])
    watchdog_byte |= 0x20; /* Bit 5 */
  if (input_sensors[6] > max_var[1])
    watchdog_byte |= 0x40; /* Bit 6 */

  /* Compare the POS H sensor inputs */
  if (input_sensors[7] > max_var[2])
    watchdog_byte |= 0x80; /* Bit 7 */
  if (input_sensors[8] > max_var[2])
    watchdog_byte |= 0x100; /* Bit 8 */
  if (input_sensors[9] > max_var[2])
    watchdog_byte |= 0x200; /* Bit 9 */
  if (input_sensors[10] > max_var[2])
    watchdog_byte |= 0x400; /* Bit 10 */

  /* Compare the GEO V sensor inputs */
  if (input_sensors[11] > max_var[3])
    watchdog_byte |= 0x10000; /* Bit 16 */
  if (input_sensors[12] > max_var[3])
    watchdog_byte |= 0x20000; /* Bit 17 */
  if (input_sensors[13] > max_var[3])
    watchdog_byte |= 0x40000; /* Bit 18 */
  if (input_sensors[14] > max_var[3])
    watchdog_byte |= 0x80000; /* Bit 19 */

  /* Compare the GEO H sensor inputs */
  if (input_sensors[15] > max_var[4])
    watchdog_byte |= 0x100000; /* Bit 20 */
  if (input_sensors[16] > max_var[4])
    watchdog_byte |= 0x200000; /* Bit 21 */
  if (input_sensors[17] > max_var[4])
    watchdog_byte |= 0x400000; /* Bit 22 */
  if (input_sensors[18] > max_var[4])
    watchdog_byte |= 0x800000; /* Bit 23 */

  return (watchdog_byte);
}

inline void seiwd1(int cycle, float rawInput[], float filtInput[],
                   SEI_WATCHDOG *wd) {
  int ii, kk, jj;

  // Get ABS of all input values
  for (ii = 0; ii < 19; ii++) {
    if (rawInput[ii] < 0)
      rawInput[ii] *= -1;
    if (filtInput[ii] < 0)
      filtInput[ii] *= -1;
  }
  if (!wd->trip) {
    wd->status[0] = 0;
    wd->status[1] = 0;
    wd->status[2] = 0;
    for (ii = 0; ii < 3; ii++) {
      if ((rawInput[ii] > 32000) || (rawInput[ii] == 0)) {
        wd->senCount[ii]++;
      }
      if (filtInput[ii] > wd->filtMax[ii]) {
        wd->filtMax[ii] = filtInput[ii];
      }
      kk = ii + 3;
      if (filtInput[ii] > wd->tripSetF[0]) {
        wd->status[0] |= (0x1 << kk);
      }
      if (cycle == 0) {
        wd->filtMaxHold[ii] = wd->filtMax[ii];
        wd->filtMax[ii] = 0;
        if (wd->senCount[ii] > wd->tripSetR[0]) {
          wd->status[0] |= (0x1 << ii);
        }
        wd->senCountHold[ii] = wd->senCount[ii];
        wd->senCount[ii] = 0;
      }
    }
    for (jj = 0; jj < 16; jj++) {
      ii = jj + 3;
      if ((rawInput[ii] > 32000) || (rawInput[ii] == 0)) {
        wd->senCount[ii]++;
      }
      if (filtInput[ii] > wd->filtMax[ii]) {
        wd->filtMax[ii] = filtInput[ii];
      }
      kk = jj / 4 + 1;
      if (filtInput[ii] > wd->tripSetF[kk]) {
        wd->status[2] |= (0x1 << jj);
      }
      if (cycle == 0) {
        wd->filtMaxHold[ii] = wd->filtMax[ii];
        wd->filtMax[ii] = 0;
        if (wd->senCount[ii] > wd->tripSetR[kk]) {
          wd->status[1] |= (0x1 << jj);
        }
        wd->senCountHold[ii] = wd->senCount[ii];
        wd->senCount[ii] = 0;
      }
    }
  }
  if ((wd->status[0]) || (wd->status[1]) || (wd->status[2]))
    wd->trip = 1;
  else
    wd->trip = 0;
}
