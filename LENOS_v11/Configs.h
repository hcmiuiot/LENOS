#ifndef CONFIGS_H
    #define CONFIGS_H



    #define PIN_STATUS      13
    #define PIN_SIM_RESET   2
    #define PIN_SIM_TX      10
    #define PIN_SIM_RX      11

    #define BAUD_SIM    4800
    #define BAUD_SERIAL 9600

    #define EEP_POS_PSW             10
    #define EEP_POS_TEMP_THRESHOLD  12

    #define TIMEOUT_WAIT_SIM          3000
    #define TIMEOUT_WAIT_SIM_RESPONSE 10000

    #define SMS_COUNT_CHECK_BALANCE 5

    #define DEFAULT_PSW 12345

    #define MAX_CLIENTS 3

    #define THRESHOLD_BALANCE 5000 //VND //VIETTEL ONLYYYY

    #define VERSION F("LENOS 1.1 - FIRMWARE 31.03.19 - CSE IoT CLUB")
    #define DK_OK "Welcome to LENOS!"
    #define DK_WRONG_PSW "Wrong password!"
    #define DK_EMPTY_SLOT "Sorry, there is no more slot!"
    #define NOTICE_LOST_ELECTRICITY "CAUTION! LOSTING ELECTRICITY"
    #define NOTICE_RETURN_ELECTRICITY "YEAH! RETURN of ELECTRICITY"
    #define NOTICE_CANCEL_REGISTER_SUCCESS "Unregister successfully!"
    #define NOTICE_CANCEL_REGISTER_FAIL "Your phone number is not in our list"
    #define NOTICE_CHANGE_PASSWORD_SUCCEED "Successfully change password!"
    #define NOTICE_CHANGE_PASSWORD_FAIL "Wrong password"
    #define NOTICE_DUPLICATE_REGISTER "Your phone number is already in LEOS"
    #define NOTICE_OVER_HEAT "WARNING!!!! LENOS IS OVERHEAT!!!!"
    #define NOTICE_BROKE "Balance is under 5000vnd"

    #define ATCMGF1 F("AT+CMGF=1")
    #define OK F("OK")
    #define RETURN F("\r")
    #define SMS_REQ F(">")
#endif
