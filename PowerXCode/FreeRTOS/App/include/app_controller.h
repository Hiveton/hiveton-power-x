#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

typedef enum
{
    APP_MODE_IDLE = 0,
    APP_MODE_MONITOR,
    APP_MODE_NEGOTIATE_PD,
    APP_MODE_NEGOTIATE_LEGACY,
    APP_MODE_CABLE_INFO,
    APP_MODE_ERROR
} app_mode_t;

void app_controller_init(void);
app_mode_t app_controller_get_mode(void);
void app_controller_set_mode(app_mode_t mode);

#endif /* APP_CONTROLLER_H */
