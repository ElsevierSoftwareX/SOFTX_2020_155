#ifndef PARAM_H
#define PARAM_H

typedef struct CHAN_PARAM {
  int dcuid;
  int datarate;
  int acquire;
  int ifoid;
  int rmid;
  int datatype;
  int chnnum;
  int testpoint;
  float gain;
  float slope;
  float offset;
  char units[40];
  char system[40];
} CHAN_PARAM;

int parseConfigFile(char *, unsigned long *, int (*callback)(char *, struct CHAN_PARAM *, void *), int, char *, void *);

int loadDaqConfigFile(DAQ_INFO_BLOCK *, GDS_INFO_BLOCK *, char *, char *, char *);

#endif
