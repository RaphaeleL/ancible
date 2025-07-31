#ifndef ANCIBLE_STATE_H
#define ANCIBLE_STATE_H

#include "../modules/module.h"

/**
 * Initialize the state system
 * 
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int state_init(void);

/**
 * Save task result to state file
 * 
 * @param host_name Host name
 * @param task_name Task name
 * @param result Module result
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int state_save_result(const char *host_name, const char *task_name, const module_result_t *result);

/**
 * Clean up the state system
 */
void state_cleanup(void);

#endif /* ANCIBLE_STATE_H */
