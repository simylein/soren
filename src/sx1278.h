#pragma once

#include <stdint.h>

void sx1278_init(void);

int sx1278_reset(void);

int sx1278_sleep(uint32_t timeout_ms);
int sx1278_standby(uint32_t timeout_ms);
int sx1278_tx(uint32_t timeout_ms);
int sx1278_rx(uint32_t timeout_ms);

int sx1278_frequency(uint32_t frequency);
int sx1278_tx_power(uint8_t power);
int sx1278_coding_rate(uint8_t cr);
int sx1278_bandwidth(uint32_t bandwidth);
int sx1278_spreading_factor(uint8_t sf);
int sx1278_checksum(bool crc);
int sx1278_sync_word(uint8_t word);

int sx1278_snr(int8_t *snr);
int sx1278_rssi(int16_t *rssi);

int sx1278_transmit(uint8_t (*data)[256], uint8_t length, uint32_t timeout_ms);
int sx1278_receive(uint8_t (*data)[256], uint8_t *length, uint32_t timeout_ms);
