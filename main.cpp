#include <iostream>
#include <pmix.h>

#define PMIX_SAFECALL(x)                                                  \
  {                                                                      \
    int err = (x);                                                       \
    if (err != PMIX_SUCCESS) {                                                           \
      fprintf(stderr, "err %d : %s (%s:%d)\n", err, PMIx_Error_string(err), \
                     __FILE__, __LINE__);                                \
    }                                                                    \
  }                                                                      \
  while (0)                                                              \
    ;

int main() {
    pmix_proc_t myproc;
    PMIX_SAFECALL(PMIx_Init(&myproc, NULL, 0));

    /* job-related info is found in our nspace, assigned to the
     * wildcard rank as it doesn't relate to a specific rank. Setup
     * a name to retrieve such values */
    pmix_proc_t proc;
    PMIX_PROC_CONSTRUCT(&proc);
    PMIX_LOAD_PROCID(&proc, myproc.nspace, PMIX_RANK_WILDCARD);

    /* get our universe size */
    pmix_value_t *val = NULL;
    PMIX_SAFECALL(PMIx_Get(&proc, PMIX_UNIV_SIZE, NULL, 0, &val));
    fprintf(stderr, "Client %s:%d universe size %d\n", myproc.nspace, myproc.rank,
            val->data.uint32);
    PMIX_VALUE_RELEASE(val);

    PMIX_SAFECALL(PMIx_Get(&proc, PMIX_JOB_SIZE, NULL, 0, &val));
    uint32_t nprocs = val->data.uint32;
    PMIX_VALUE_RELEASE(val);
    fprintf(stderr, "Client %s:%d num procs %d\n", myproc.nspace, myproc.rank,
            nprocs);

    char* tmp;
    if (0 > asprintf(&tmp, "%s-%d-global", myproc.nspace, myproc.rank)) {
        exit(1);
    }
    pmix_value_t value;
    value.type = PMIX_UINT64;
    value.data.uint64 = 1234 + myproc.rank;
    PMIX_SAFECALL(PMIx_Put(PMIX_GLOBAL, tmp, &value));
    free(tmp);

    PMIX_SAFECALL(PMIx_Commit());

    /* call fence to synchronize with our peers - instruct
     * the fence operation to collect and return all "put"
     * data from our peers */
    pmix_info_t *info;
    PMIX_INFO_CREATE(info, 1);
    bool flag = true;
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);
    PMIX_SAFECALL(PMIx_Fence(&proc, 1, info, 1));
    PMIX_INFO_FREE(info, 1);

    /* check the returned data */
    for (uint32_t n = 0; n < nprocs; n++) {
        proc.rank = PMIX_RANK_UNDEF;
        if (0 > asprintf(&tmp, "%s-%d-global", myproc.nspace, n)) {
            exit(1);
        }
        PMIX_SAFECALL(PMIx_Get(&proc, tmp, NULL, 0, &val));
        if (PMIX_UINT64 != val->type) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned wrong type: %d\n",
                    myproc.nspace, myproc.rank, tmp, val->type);
            exit(1);
        }
        if (val->data.uint64 != 1234 + n) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned wrong value: %s\n",
                    myproc.nspace, myproc.rank, tmp, val->data.string);
            exit(1);
        }
        fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned correct\n", myproc.nspace,
                myproc.rank, tmp);
        PMIX_VALUE_RELEASE(val);
        free(tmp);
    }

    std::cout << "Hello, World!" << std::endl;
    PMIX_SAFECALL(PMIx_Finalize(NULL, 0));
    return 0;
}
