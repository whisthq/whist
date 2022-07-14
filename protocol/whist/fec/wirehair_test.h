/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file wirehair_test.h
 * @brief contains codes that help unstand the performance/overhead/recoverablity
 * of wirehair
 * @note the codes can also be reused if in futhure we integrate raptor
 */

#pragma once

/**
 * @brief                          entry for automatic tests for wirehair, mainly aim for
 * correctness
 * @returns                        0 on success
 */
int wirehair_auto_test(void);

/* @brief                          entry for manual tests for wirehair, which printsout info for
 * human inspection.
 * @returns                        0 on success
 */
int wirehair_manual_test(void);
