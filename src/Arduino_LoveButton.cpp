/*

    CHIEDERE COSA USARE COME INTESTAZIONE LIBRERIA ARDUINO

     

     */

#include "Arduino_LoveButton.h"






Arduino_LoveButton_Class Arduino_Love;



namespace LB_NAMESPACE {

    int ctsurdEventLinkIndex = 0;
    int ctsuwrEventLinkIndex = 0;
    
    volatile uint16_t sCounter;
    volatile uint16_t rCounter;
    
    volatile boolean touch;
    
    /**
    * Handler for the CTSU WRITE Interrupt
    *
    * @param void
    * @return void
    */
    void CTSUWR_handler() {
      // we need this interrupt to trigger the CTSU to go to state 3.
      resetEventLink(ctsuwrEventLinkIndex);
      R_CTSU->CTSUMCH0 = CTSUMCH0_LOVE;
      R_CTSU->CTSUSO1 = 0x0F00;
    }
    
    /**
    * Handler for the CTSU READ Interrupt
    *
    * @param void
    * @return void
    */
    void CTSURD_handler() {
      resetEventLink(ctsurdEventLinkIndex);
      sCounter = R_CTSU->CTSUSC;
      // Must read CTSURC even if we don't use it in order for the unit to move on
      rCounter = R_CTSU->CTSURC;      
      touch = ((sCounter - rCounter) > Arduino_Love.threshold);
      startCTSUmeasure();
    }
    
    /**
    * Start the CTSU Measure Process
    *
    * @param void
    * @return void
    */
    void startCTSUmeasure() {
      R_CTSU->CTSUMCH0 = CTSUMCH0_LOVE;  // select pin TS00 for Minima, or TS27 for WiFi
      R_CTSU->CTSUCR0 = (CTSU_START_MEASURE | CTSU_EXT_TRIGGER);   // software start measurement wait for trigger
    }

    /**
    * Attach an Event to a Interrupt
    *
    * @param eventCode : Event Code that must be linked to the interrupt
    * @param func : callback function
    * @return index of the interrupt that was set by the function
    */
    int attachEventLinkInterrupt(uint8_t eventCode, Irq_f func /*= nullptr*/)
    {

        GenericIrqCfg_t generic_cfg;
        generic_cfg.irq = FSP_INVALID_VECTOR;
        generic_cfg.event = (elc_event_t)eventCode;
        generic_cfg.ipl = (12);
        IRQManager::getInstance().addGenericInterrupt(generic_cfg, func);
        return generic_cfg.irq;
    }



    /**
    * Reset Event Link
    *
    * @param eventLinkIndex : Index of the Event to reset 
    * @return void
    */
    void resetEventLink(int eventLinkIndex)
    {
        if ((eventLinkIndex >= 0) && (eventLinkIndex < NUMBER_OF_ILC_SLOTS))
        {
            R_ICU->IELSR[eventLinkIndex] &= ~(R_ICU_IELSR_IR_Msk);
        }
    }
}

/**
* Read a value that is related to the measure on Touch Button
* When something get closer to the touch Button this value go up
*
* @param voud
* @return Measured value
*/
uint16_t Arduino_LoveButton_Class::read_value()
{
    return (LB_NAMESPACE::sCounter - LB_NAMESPACE::rCounter);

}

/**
* The Value measured by the CTSU peripheral is compared with a Threshold and if 
* this value is higher that the threshold the bool value returned by this function is true
*
* @param void
* @return True if the button is touched; false otherwise
*/
bool Arduino_LoveButton_Class::read_touch()
{
    return LB_NAMESPACE::touch;

}

/**
* Set the threshold 
*
* @param t: threshold
* @return void
*/
void Arduino_LoveButton_Class::setThreshold(uint16_t t)
{
    threshold = t;
}

/**
* Initialize the CTSU to use the Arduino Love Button as a Touch Sensor
*
* @param void
* @return void
*/
void Arduino_LoveButton_Class::begin()
{
    static bool hasBegun = false;
    if (!hasBegun)
    {
        hasBegun = true;
        // Follow the flow chart Fig 41.9
        // Step 1: Discharge LPF (set TSCAP as OUTPUT LOW.)
        R_PFS->PORT[TPS_PORT].PIN[TPS_PIN].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PDR_Pos);
        delay(100);

        // Step 2: Setup I/O port PmnPFR registers
        // Set Love pin to TS function
        R_PFS->PORT[LOVE_PORT].PIN[LOVE_PIN].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos) | (12 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);
        // set TSCAP pin to TSCAP function
        R_PFS->PORT[TPS_PORT].PIN[TPS_PIN].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos) | (12 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);

        // Step 3: Enable CTSU in MSTPCRC bit MSTPC3 to 0
        R_MSTP->MSTPCRC &= ~(1 << R_MSTP_MSTPCRC_MSTPC3_Pos);

        // Step 4: Set CTSU Power Supply (CTSUCR1 register)
        R_CTSU->CTSUCR1 = (CTSU_POWER_OFF | CTSU_CAP_SW_OFF | CTSU_NORMAL_MODE | CTSU_CLK_PCLKB | CTSU_SELF_SINGLE)  ; // all 0's work for now

        // Step 5: Set CTSU Base Clock (CTSUCR1 and CTSUSO1 registers) _ Operating Clock/32
        R_CTSU->CTSUSO1 = 0x0F00;

        // Step 6: Power On CTSU (set bits CTSUPON and CTSUCSW in CTSUCR1 at the same time)
        R_CTSU->CTSUCR1 = (CTSU_POWER_ON | CTSU_CAP_SW_ON);

        // Step 7: Wait for stabilization (Whatever that means...)
        delay(100);

        // setup other registers:
        R_CTSU->CTSUSDPRS = (CTSU_PULSE_COUNT | CTSU_62_PULSES | CTSU_HP_FILTER_OFF); 
        R_CTSU->CTSUSST = CTSU_SENSOR_STAB; // data sheet says set value to this only
        R_CTSU->CTSUCHAC[CTSUCHAC_IDX] = CTSUCHAC_VALUE; // enable pin TS00 for measurement
        R_CTSU->CTSUDCLKC = CTSU_CTSUDCLKC_REC;                        // data sheet dictates these settings.

        R_CTSU->CTSUMCH0 = CTSUMCH0_LOVE; // select pin TS00

        LB_NAMESPACE::ctsurdEventLinkIndex = LB_NAMESPACE::attachEventLinkInterrupt(CTSURD, LB_NAMESPACE::CTSURD_handler);
        LB_NAMESPACE::ctsuwrEventLinkIndex = LB_NAMESPACE::attachEventLinkInterrupt(CTSUWR, LB_NAMESPACE::CTSUWR_handler);
        // Enable Event Link Controller in Master Stop Register
        R_MSTP->MSTPCRC &= ~(1 << R_MSTP_MSTPCRC_MSTPC14_Pos);
        // The ELC register for CTSU is ELSR18
        // The event link signal for AGT0 underflow is 0x1E
        R_ELC->ELSR[CTSU_ELSR_EVENT].HA = AGT0_EVENT; //was   CTSU_CTSUWR_EVENT
        // enable ELC
        R_ELC->ELCR = (1 << R_ELC_ELCR_ELCON_Pos);

        LB_NAMESPACE::startCTSUmeasure();
    }
}
