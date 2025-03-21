/*

    CHIEDERE COSA USARE COME INTESTAZIONE LIBRERIA ARDUINO

     

     */

#include "Arduino_LoveButton.h"

Arduino_LoveButton_Class Arduino_Love;

int ctsurdEventLinkIndex = 0;
int ctsuwrEventLinkIndex = 0;

volatile uint16_t sensCounter;
volatile uint16_t refCounter;

volatile boolean touch;

/**
 * Handler for the CTSU WRITE Interrupt
 *
 * @param void
 * @return void
 */
void Arduino_LoveButton_Class::CTSUWR_handler()
{
    // we need this interrupt to trigger the CTSU to go to state 3.
    Arduino_LoveButton_Class::resetEventLinkInterrupt(ctsuwrEventLinkIndex);
    R_CTSU->CTSUMCH0 = CTSUMCH0_LOVE;
    R_CTSU->CTSUSO1 = 0x0F00;
}

/**
 * Handler for the CTSU READ Interrupt
 *
 * @param void
 * @return void
 */
void Arduino_LoveButton_Class::CTSURD_handler()
{
    Arduino_LoveButton_Class::resetEventLinkInterrupt(ctsurdEventLinkIndex);
    sensCounter = R_CTSU->CTSUSC;
    // Must read CTSURC even if we don't use it in order for the unit to move on
    refCounter = R_CTSU->CTSURC;
    touch = ((sensCounter - refCounter) > Arduino_Love.threshold);
    startCTSUmeasure();
}

/**
 * Start the CTSU Measure Process
 *
 * @param void
 * @return void
 */
void Arduino_LoveButton_Class::startCTSUmeasure()
{
    R_CTSU->CTSUMCH0 = CTSUMCH0_LOVE;                          // select pin TS00 for Minima, or TS27 for WiFi
    R_CTSU->CTSUCR0 = (CTSU_START_MEASURE | CTSU_EXT_TRIGGER); // software start measurement wait for trigger
}

/**
 * Attach an Event to a Interrupt
 *
 * @param eventCode : Event Code that must be linked to the interrupt
 * @param func : callback function
 * @return index of the interrupt that was set by the function
 */
int Arduino_LoveButton_Class::createEventLinkInterrupt(uint8_t eventCode, Irq_f func /*= nullptr*/)
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
void Arduino_LoveButton_Class::resetEventLinkInterrupt(int eventLinkIndex)
{
    if ((eventLinkIndex >= 0) && (eventLinkIndex < NUMBER_OF_ILC_SLOTS))
    {
        R_ICU->IELSR[eventLinkIndex] &= ~(R_ICU_IELSR_IR_Msk);
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
    return (sensCounter - refCounter);

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
    return touch;

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
        // Figure 41.9 "Initial Setting Flow"
        // Step 1: Discharge the external LPF capacitor connected to the MCU by using the TSCAP pin as
        // the I/O port function and driving it low for the specified time
        R_PFS->PORT[TPS_PORT].PIN[TPS_PIN].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PDR_Pos);
        // Capacitor used on PIN7 pin could change so just apply a random delay log enough for every settings
        delay(50);

        // Step 2: Set the associated pin to TSn
        R_PFS->PORT[LOVE_PORT].PIN[LOVE_PIN].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos) | (12 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);
        // set the pin to the peripheral function by setting the Port Mode Control bit for the I/O port (PMR)
        R_PFS->PORT[TPS_PORT].PIN[TPS_PIN].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos) | (12 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);

        // Step 3: Enable the module clock by setting the MSTPC3 bit in the Module Stop Control Register C (MSTPCRC) to 0
        R_MSTP->MSTPCRC &= ~(1 << R_MSTP_MSTPCRC_MSTPC3_Pos);

        // Step 4: Set the CTSU power supply operating mode and capacity adjustment.
        R_CTSU->CTSUCR1 = (CTSU_POWER_OFF | CTSU_CAP_SW_OFF | CTSU_NORMAL_MODE | CTSU_CLK_PCLKB | CTSU_SELF_SINGLE)  ; // all 0's work for now

        // Step 5: Use the CTSUCR1.CTSUCLK[1:0] and CTSUSO1.CTSUSDPA[4:0] bits to set the base clock
        R_CTSU->CTSUSO1 = 0x0F00;

        // Step 6: Supply power to the CTSU and connect the LPF capacitor to the TSCAP pin
        // Write 1 to the CTSUCR1.CTSUPON bit and 1 to the CTSUCR1.CTSUCSW bit at the same time.
        R_CTSU->CTSUCR1 = (CTSU_POWER_ON | CTSU_CAP_SW_ON);

        // Step 7: After data is written, wait until charging of the external LPF capacitor connected to the
        // TSCAP pin stabilizes.
        delay(100);

        // setup other registers:
        R_CTSU->CTSUSDPRS = (CTSU_PULSE_COUNT | CTSU_62_PULSES | CTSU_HP_FILTER_OFF); 
        R_CTSU->CTSUSST = CTSU_SENSOR_STAB;                 // datasheet recommended value
        R_CTSU->CTSUCHAC[CTSUCHAC_IDX] = CTSUCHAC_VALUE;    // enable pin TS00 
        R_CTSU->CTSUDCLKC = CTSU_CTSUDCLKC_REC;             // datasheet recommended value

        R_CTSU->CTSUMCH0 = CTSUMCH0_LOVE; // select pin based on board

        ctsurdEventLinkIndex = Arduino_LoveButton_Class::createEventLinkInterrupt(CTSURD, Arduino_LoveButton_Class::CTSURD_handler);
        ctsuwrEventLinkIndex = Arduino_LoveButton_Class::createEventLinkInterrupt(CTSUWR, Arduino_LoveButton_Class::CTSUWR_handler);
        // Enable Event Link Controller 
        R_MSTP->MSTPCRC &= ~(1 << R_MSTP_MSTPCRC_MSTPC14_Pos);
        // Association between the ELSRn registers and peripheral functions : for CTSU is ELSR18
        // Association between event signal names set in ELSRn.ELS bits and signal numbers : for AGTO interrupt is 0x1E
        R_ELC->ELSR[CTSU_ELSR_EVENT].HA = AGT0_EVENT; 
        // enable ELC
        R_ELC->ELCR = (1 << R_ELC_ELCR_ELCON_Pos);

        startCTSUmeasure();
    }
}
