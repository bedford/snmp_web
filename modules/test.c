#include <stdlib.h>
#include <stdio.h>

#include "modem.h"

int main(void)
{
    modem_t *modem = modem_create();
    if (modem->connected(modem)) {
        return -1;
    }

    char sca_code[32] = {0};
    if (modem->get_sca(modem, sca_code)) {
        return -1;
    }

    if (modem->set_mode(modem, SMS_MODE_PDU)) {
        return -1;
    }

    char *string = "当前温度：+023.4度";
    char *phone_num = "13828891024";
    if (modem->send_sms(modem, phone_num, string)) {
        return -1;
    }

    return 0;
}
