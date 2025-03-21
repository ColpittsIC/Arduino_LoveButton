/*
    Arduino Love Button Library

    Copyright (c) 2023 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef ARDUINO_LOVEBUTTON_H
#define ARDUINO_LOVEBUTTON_H

#if !defined(ARDUINO_UNOR4_WIFI) && !defined(ARDUINO_UNOR4_MINIMA)
#error Library only working on UNO-R4 BOARDS
#endif

#include "Arduino.h"
#include "IRQManager.h"

/*Interrupt Management*/ 
#define NUMBER_OF_ILC_SLOTS 32

#define TPS_PORT    1
#define TPS_PIN     12

#ifdef ARDUINO_UNOR4_WIFI
#define CTSUMCH0_LOVE  0x1B //TS27
#define LOVE_PORT 1
#define LOVE_PIN 13 //Capacitive button connected to pin P113
//There are 5 CTSUCHAC registers to cover all the 33 CTSU Pin available 
//In Arduino UNO R4_WIFI the capacitive pin is connected on pin TS27 -> CTSUCHAC[3]
#define CTSUCHAC_IDX 3 //TS27 is on CTSUCHAC[3] = (1 << 3)
#define CTSUCHAC_VALUE (1<<3)
#endif

#ifdef ARDUINO_UNOR4_MINIMA
#define CTSUMCH0_LOVE  0 //TS00
#define LOVE_PORT 2
#define LOVE_PIN 4 //Capacitive button connected to pin P204
//There are 5 CTSUCHAC registers to cover all the 33 CTSU Pin available 
//In Arduino UNO R4_WIFI the capacitive pin is connected on pin TS00 -> CTSUCHAC[0]
#define CTSUCHAC_IDX 0 //TS00 is on CTSUCHAC[0] = 1
#define CTSUCHAC_VALUE 1
#endif

/*DEFINITION FOR REGISTERS SETTINGS*/
/*CTSUCR1 SETTINGS*/
#define CTSU_POWER_OFF          (0)
#define CTSU_POWER_ON           (0x1)
#define CTSU_CAP_SW_OFF         (0<<1)
#define CTSU_CAP_SW_ON          (1<<1)
#define CTSU_NORMAL_MODE        (0<<2)
#define CTSU_LOW_VOLTAGE_MODE   (1<<2)
#define CTSU_CLK_PCLKB          (0<<4)
#define CTSU_CLK_PCLKB_DIV_2    (1<<4)
#define CTSU_CLK_PCLKB_DIV_4    (2<<4)
#define CTSU_SELF_SINGLE        (0<<6)
#define CTSU_SELF_MULTI         (1<<6)
#define CTSU_MUTUAL             (3<<6)

/*CTSUSDPRS SETTINGS*/
#define CTSU_PULSE_COUNT        (3) //This is a recommended value
#define CTSU_510_PULSES         (0<<4)
#define CTSU_126_PULSES         (1<<4)
#define CTSU_62_PULSES          (2<<4) //Recommended value
#define CTSU_HP_FILTER_ON       (0<<6)
#define CTSU_HP_FILTER_OFF      (1<<6)

/*CTSUSST REGISTER*/
#define CTSU_SENSOR_STAB        (0x10) //Recommended Value


/*CTSUDCLKC REGISTER*/
#define CTSU_CTSUDCLKC_REC      (0x30)

/*CTSUCR0 REGISTER*/
#define CTSU_STOP_MEASURE       0x0
#define CTSU_START_MEASURE      0x1
#define CTSU_SOFT_TRIGGER       (0<<1)
#define CTSU_EXT_TRIGGER        (1<<1)
#define CTSU_POWER_SAVE_OFF     (0<<2)
#define CTSU_POWER_SAVE_ON      (1<<2)


/*ELSR*/
#define CTSU_ELSR_EVENT         0x12

/*Interrupt Event Code*/
#define CTSUWR                  0x42
#define CTSURD                  0x43

/*AGT0 Event*/
#define AGT0_EVENT              0x1E
#define CTSU_CTSUWR_EVENT       0x42         


class Arduino_LoveButton_Class
{
public:
    uint16_t threshold;
    Arduino_LoveButton_Class() : threshold(23000) {}

    public:
    void begin();
    bool read_touch();
    uint16_t read_value();
    void setThreshold(uint16_t t);

    private:
    static void startCTSUmeasure();
    static void CTSURD_handler();
    static void CTSUWR_handler();
    static int  createEventLinkInterrupt(uint8_t eventCode, Irq_f func = nullptr);
    static void resetEventLinkInterrupt(int eventLinkIndex);

};






extern Arduino_LoveButton_Class Arduino_Love;







#endif // ARDUINO_LOVEBUTTON_H