#ifndef DAQD_RUN_NUMBER_STRUCTS_H
#define DAQD_RUN_NUMBER_STRUCTS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct daqd_run_number_req_v1 {
    short version;
    short hash_size;
    char  hash[64];
} daqd_run_number_req_v1_t;

typedef struct daqd_run_number_resp_v1 {
    short version;
    short padding;
    int number;
} daqd_run_number_resp_v1_t;

#ifdef __cplusplus
}
#endif

#endif /* DAQD_RUN_NUMBER_STRUCTS_H */