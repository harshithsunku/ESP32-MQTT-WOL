#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include "esp_system.h"

/**
 * @brief Print comprehensive device information including:
 * - Chip information (model, revision, cores, features)
 * - Flash information
 * - MAC addresses
 * - Memory information
 * - Partition information
 * - System information
 * - Security information
 */
void device_info_print_all(void);

/**
 * @brief Print only chip information
 */
void device_info_print_chip(void);

/**
 * @brief Print only memory information
 */
void device_info_print_memory(void);

/**
 * @brief Print only MAC addresses
 */
void device_info_print_mac_addresses(void);

/**
 * @brief Print detailed hardware features and capabilities
 */
void device_info_print_hardware_features(void);

/**
 * @brief Print peripheral and GPIO information
 */
void device_info_print_peripherals(void);

/**
 * @brief Print clock and timing information
 */
void device_info_print_clocks(void);

#endif // DEVICE_INFO_H
