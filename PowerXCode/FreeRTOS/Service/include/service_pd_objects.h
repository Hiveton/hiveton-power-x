#ifndef SERVICE_PD_OBJECTS_H
#define SERVICE_PD_OBJECTS_H

#include <stdint.h>

typedef enum
{
    PD_OBJECT_TYPE_FIXED = 0,
    PD_OBJECT_TYPE_BATTERY = 1,
    PD_OBJECT_TYPE_VARIABLE = 2,
    PD_OBJECT_TYPE_APDO = 3,
    PD_OBJECT_TYPE_UNSUPPORTED = 255
} pd_object_type_t;

typedef enum
{
    PD_APDO_SUBTYPE_SPR_PPS = 0,
    PD_APDO_SUBTYPE_EPR_AVS = 1,
    PD_APDO_SUBTYPE_SPR_AVS = 2,
    PD_APDO_SUBTYPE_RESERVED = 3
} pd_apdo_subtype_t;

typedef struct
{
    uint32_t raw;
    uint8_t position;
    pd_object_type_t type;
    pd_apdo_subtype_t apdo_subtype;
    union
    {
        struct
        {
            int32_t voltage_mv;
            int32_t current_ma;
            uint8_t epr_capable;
        } fixed;
        struct
        {
            int32_t min_mv;
            int32_t max_mv;
            int32_t power_mw;
        } battery;
        struct
        {
            int32_t min_mv;
            int32_t max_mv;
            int32_t current_ma;
        } variable;
        struct
        {
            int32_t min_mv;
            int32_t max_mv;
            int32_t current_ma;
        } pps;
        struct
        {
            int32_t current_9v_15v_ma;
            int32_t current_15v_20v_ma;
        } spr_avs;
        struct
        {
            int32_t min_mv;
            int32_t max_mv;
            int32_t pdp_w;
        } epr_avs;
    };
} pd_object_t;

uint8_t pd_object_parse_source_pdo(uint8_t position, uint32_t raw, pd_object_t *object);
uint8_t pd_object_is_fixed(const pd_object_t *object);
uint8_t pd_object_is_pps(const pd_object_t *object);
uint8_t pd_object_build_fixed_rdo(const pd_object_t *object, int32_t operating_ma, uint32_t *rdo);
uint8_t pd_object_build_pps_rdo(const pd_object_t *object, int32_t voltage_mv, int32_t operating_ma, uint32_t *rdo);
uint8_t pd_object_build_avs_rdo(const pd_object_t *object, int32_t voltage_mv, int32_t operating_ma, uint32_t *rdo);

#endif /* SERVICE_PD_OBJECTS_H */
