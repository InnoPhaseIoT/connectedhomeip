

/* Enable Dimmer Switch for Level Control and
 * Colour Control functionality. */
#define ENABLE_DIMMER_SWITCH 1

#if ENABLE_DIMMER_SWITCH
/* Enable Dimmer Switch for Level Control */
#define ENABLE_LEVEL_CONTROL 1

/* Enable Dimmer Switch for Colour Control */
#define ENABLE_COLOUR_CONTROL 1
#endif /* ENABLE_DIMMER_SWITCH */

#define LED_PIN 14

#define IDENTIFY_TIMER_DELAY_MS  1000

#define SWITCH_PIN 18

/* 0 - For random functionality method.
 * 1 - For ADC fnctionality method. */
#define SWITCH_USING_ADC 0

#define SWITCH_ENDPOINT_ID 2

/* Maximum number of positions that a switch can support,
 * can be configured as per requirement */
#define SWITCH_MAX_POSITIONS 4

/* Type of switch, can be configured as per requirement */
#define SWITCH_TYPE_MOMENTARY static_cast<uint32_t>(Switch::Feature::kMomentarySwitch)

/* Periodic timeout for reading the switch position using a software timer in ms. */
#define SWITCH_POS_PERIODIC_TIME_OUT_MS 10000

/* The ADC PIN can read up to 1V, and value will be (4096 * input_voltage).
 * The ADC value can vary, so defined minimum and maximum ranges to
 * calculate the current position of the switch.
 * These ranges can be configured according to requirements. */
#define ADC_VALUE_0_25_V_MIN 920
#define ADC_VALUE_0_25_V_MAX 1024
#define ADC_VALUE_0_5_V_MAX 2048
#define ADC_VALUE_0_75_V_MAX 3072
#define ADC_VALUE_1_V_MAX 4096

/* Maximum number of positions that a switch can support,
 * can be configured as per requirement */
#define SWITCH_MAX_POSITIONS 4

/* Type of switch, can be configured as per requirement */
#define SWITCH_TYPE_MOMENTARY static_cast<uint32_t>(Switch::Feature::kMomentarySwitch)

/* 4 switch positions have been considered, can be configured
 * by setting value of SWITCH_MAX_POSITIONS according to requirements. */
enum Switch_Position
{
    SWITCH_POS_0,
    SWITCH_POS_1,
    SWITCH_POS_2,
    SWITCH_POS_3,
    SWITCH_POS_4,
};

