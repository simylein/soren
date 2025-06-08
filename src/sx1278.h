#pragma once

void sx1278_init(void);

void sx1278_sleep(void);
void sx1278_standby(void);

void sx1278_frequency(uint32_t freq_hz);
void sx1278_tx_power(uint8_t power_dbm);
void sx1278_coding_rate(uint8_t cr);
void sx1278_bandwidth(uint32_t bandwidth_hz);
void sx1278_spreading_factor(uint8_t sf);
void sx1278_checksum(bool crc);

void sx1278_send(uint8_t (*data)[256], uint8_t length, uint16_t timeout_ms);
void sx1278_recv(uint8_t (*data)[256], uint8_t *length, uint16_t timeout_ms);
