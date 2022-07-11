/*
 * matrix.h
 *
 *  Created on: 6 Oct 2018
 *      Author: gal
 */

#ifndef MATRIX_H_
#define MATRIX_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "keyboard_config.h"
#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t MATRIX_STATE[MATRIX_ROWS][MATRIX_COLS];

/*
 * @brief deinitialize rtc pins
 */
void rtc_matrix_deinit(void);

/*
 * @brief initialize rtc pins
 */
void rtc_matrix_setup(void);

/*
 * @brief initialize matrix
 */
void matrix_setup(void);

/*
 * @brief scan matrix
 */
void scan_matrix(void);

#ifdef __cplusplus
}
#endif
#endif /* MATRIX_H_ */
