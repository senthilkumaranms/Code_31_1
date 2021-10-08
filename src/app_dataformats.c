#include "app_dataformats.h"
#include "app_sensor.h"
#include "ruuvi_endpoints.h"
#include "ruuvi_endpoint_3.h"
#include "ruuvi_endpoint_5.h"
#include "ruuvi_endpoint_8.h"
#include "ruuvi_endpoint_fa.h"
#include "ruuvi_interface_aes.h"
#include "ruuvi_interface_communication_ble_advertising.h"
#include "ruuvi_interface_communication_radio.h"
#include "ruuvi_task_adc.h"

#include <math.h>

#ifdef CEEDLING
#   define TESTABLE_STATIC
#else
#   define TESTABLE_STATIC static
#endif

#ifndef APP_FA_KEY
#define APP_FA_KEY {00, 11, 22, 33, 44, 55, 66, 77, 88, 99, 11, 12, 13, 14, 15, 16}
#endif
static const uint8_t ep_fa_key[RE_FA_CIPHERTEXT_LENGTH] = APP_FA_KEY;

uint32_t app_data_encrypt (const uint8_t * const cleartext,
                           uint8_t * const ciphertext,
                           const size_t data_size,
                           const uint8_t * const key,
                           const size_t key_size)
{
    rd_status_t err_code = RD_SUCCESS;
    uint32_t ret_code = 0;

    if (16U != key_size)
    {
        err_code |= RD_ERROR_INVALID_LENGTH;
    }
    else
    {
        err_code |= ri_aes_ecb_128_encrypt (cleartext,
                                            ciphertext,
                                            key,
                                            data_size);
    }

    if (RD_SUCCESS != err_code)
    {
        ret_code = 1;
    }

    RD_ERROR_CHECK (err_code, ~RD_ERROR_FATAL);
    return ret_code;
}

/**
 * @brief Return next dataformat to send
 *
 * @param[in] formats Enabled formats.
 * @param[in] state Current state of dataformat picker.
 * @return    Next dataformat to use in app.
 */
app_dataformat_t app_dataformat_next (const app_dataformats_t formats,
                                      const app_dataformat_t state)
{
    return DF_INVALID;
}

TESTABLE_STATIC rd_status_t
encode_to_3 (uint8_t * const output,
             size_t * const output_length,
             const rd_sensor_data_t * const data)
{
    rd_status_t err_code = RD_SUCCESS;
    re_status_t enc_code = RE_SUCCESS;
    re_3_data_t ep_data = {0};
    ep_data.accelerationx_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_X_FIELD);
    ep_data.accelerationy_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_Y_FIELD);
    ep_data.accelerationz_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_Z_FIELD);
    ep_data.humidity_rh       = rd_sensor_data_parse (data, RD_SENSOR_HUMI_FIELD);
    ep_data.pressure_pa       = rd_sensor_data_parse (data, RD_SENSOR_PRES_FIELD);
    ep_data.temperature_c     = rd_sensor_data_parse (data, RD_SENSOR_TEMP_FIELD);
    err_code |= rt_adc_vdd_get (&ep_data.battery_v);
    enc_code |= re_3_encode (output, &ep_data, RD_FLOAT_INVALID);

    if (RE_SUCCESS != enc_code)
    {
        err_code |= RD_ERROR_INTERNAL;
    }

    *output_length = RE_3_DATA_LENGTH;
    return err_code;
}

TESTABLE_STATIC rd_status_t
encode_to_5 (uint8_t * const output,
             size_t * const output_length,
             const rd_sensor_data_t * const data)
{
    static uint16_t ep_5_measurement_count = 0;
    rd_status_t err_code = RD_SUCCESS;
    re_status_t enc_code = RE_SUCCESS;
    re_5_data_t ep_data = {0};
    ep_5_measurement_count++;
    ep_5_measurement_count %= RE_5_SEQCTR_MAX;
    ep_data.accelerationx_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_X_FIELD);
    ep_data.accelerationy_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_Y_FIELD);
    ep_data.accelerationz_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_Z_FIELD);
    ep_data.humidity_rh       = rd_sensor_data_parse (data, RD_SENSOR_HUMI_FIELD);
    ep_data.pressure_pa       = rd_sensor_data_parse (data, RD_SENSOR_PRES_FIELD);
    ep_data.temperature_c     = rd_sensor_data_parse (data, RD_SENSOR_TEMP_FIELD);
    ep_data.measurement_count = ep_5_measurement_count;
    uint8_t mvtctr = (uint8_t) (app_sensor_event_count_get() % (RE_5_MVTCTR_MAX + 1));
    ep_data.movement_count    = mvtctr;
    err_code |= ri_radio_address_get (&ep_data.address);
    err_code |= ri_adv_tx_power_get (&ep_data.tx_power);
    err_code |= rt_adc_vdd_get (&ep_data.battery_v);
    enc_code |= re_5_encode (output, &ep_data);

    if (RE_SUCCESS != enc_code)
    {
        err_code |= RD_ERROR_INTERNAL;
    }

    *output_length = RE_5_DATA_LENGTH;
    return err_code;
}

TESTABLE_STATIC rd_status_t
encode_to_8 (uint8_t * const output,
             size_t * const output_length,
             const rd_sensor_data_t * const data)
{
    return RD_ERROR_NOT_IMPLEMENTED;
}

TESTABLE_STATIC rd_status_t
encode_to_fa (uint8_t * const output,
              size_t * const output_length,
              const rd_sensor_data_t * const data)
{
    static uint8_t ep_fa_measurement_count = 0;
    rd_status_t err_code = RD_SUCCESS;
    re_status_t enc_code = RE_SUCCESS;
    re_fa_data_t ep_data = {0};
    ep_fa_measurement_count++;
    ep_fa_measurement_count %= 0xFFU;
    ep_data.accelerationx_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_X_FIELD);
    ep_data.accelerationy_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_Y_FIELD);
    ep_data.accelerationz_g   = rd_sensor_data_parse (data, RD_SENSOR_ACC_Z_FIELD);
    ep_data.humidity_rh       = rd_sensor_data_parse (data, RD_SENSOR_HUMI_FIELD);
    ep_data.pressure_pa       = rd_sensor_data_parse (data, RD_SENSOR_PRES_FIELD);
    ep_data.temperature_c     = rd_sensor_data_parse (data, RD_SENSOR_TEMP_FIELD);
    ep_data.message_counter   = ep_fa_measurement_count;
    err_code |= rt_adc_vdd_get (&ep_data.battery_v);
    err_code |= ri_radio_address_get (&ep_data.address);
    enc_code |= re_fa_encode (output,
                              &ep_data,
                              &app_data_encrypt,
                              ep_fa_key,
                              RE_FA_CIPHERTEXT_LENGTH); //!< Cipher length == key lenght

    if (RE_SUCCESS != enc_code)
    {
        err_code |= RD_ERROR_INTERNAL;
    }

    *output_length = RE_FA_DATA_LENGTH;
    return err_code;
}

/**
 * @brief Encode data into given buffer with given format.
 *
 * A call to this function will increment measurement sequence counter
 * where applicable. Sensors are read to get latest data from board.
 *
 * @param[out] output Buffer to which data is encoded.
 * @param[in,out] output_length Input: Size of output buffer.
 *                              Output: Size of encoded data.
 * @param[in] format Format to encode data into.
 *
 */
rd_status_t app_dataformat_encode (uint8_t * const output,
                                   size_t * const output_length,
                                   const app_dataformat_t format)
{
    rd_status_t err_code = RD_SUCCESS;
    rd_sensor_data_t data = {0};
    data.fields = app_sensor_available_data();
    float data_values[rd_sensor_data_fieldcount (&data)];
    data.data = data_values;
    app_sensor_get (&data);

    switch (format)
    {
        case DF_3:
            encode_to_3 (output, output_length, &data);
            break;

        case DF_5:
            encode_to_5 (output, output_length, &data);
            break;

        case DF_FA:
            encode_to_fa (output, output_length, &data);
            break;

        default:
            err_code |= RD_ERROR_NOT_IMPLEMENTED;
    }

    return err_code;
}
