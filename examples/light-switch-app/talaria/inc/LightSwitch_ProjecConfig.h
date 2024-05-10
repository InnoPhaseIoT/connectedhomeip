

/* Enable Dimmer Switch for Level Control and
 * Colour Control functionality. */
#define ENABLE_DIMMER_SWITCH 1

#if ENABLE_DIMMER_SWITCH
/* Enable Dimmer Switch for Level Control */
#define ENABLE_LEVEL_CONTROL 1

/* Enable Dimmer Switch for Colour Control */
#define ENABLE_COLOUR_CONTROL 1
#define COLOR_VALUE_MAX 65279
#endif /* ENABLE_DIMMER_SWITCH */

#define LED_PIN 14

#define IDENTIFY_TIMER_DELAY_MS  1000

#define SWITCH_PIN 18

/* 0 - For random functionality method.
 * 1 - For ADC fnctionality method. */
#define SWITCH_USING_ADC 1

#define SWITCH_ENDPOINT_ID 2

/* Maximum number of positions that a switch can support,
 * can be configured as per requirement */
#define SWITCH_MAX_POSITIONS 4

/* Type of switch, can be configured as per requirement */
#define SWITCH_TYPE_MOMENTARY static_cast<uint32_t>(Switch::Feature::kMomentarySwitch)
#define SWITCH_TYPE_LATCHING static_cast<uint32_t>(Switch::Feature::kLatchingSwitch)

/* Periodic timeout for reading the switch position using a software timer in ms. */
#define SWITCH_POS_PERIODIC_TIME_OUT_MS 1000

/* The ADC PIN can read up to 1V, and value will be (4096 * input_voltage).
 * The ADC value can vary, so defined minimum and maximum ranges to
 * calculate the current position of the switch.
 * These ranges can be configured according to requirements. */
#define ADC_SWITCH_POS_0 0
#define ADC_SWITCH_POS_1 512
#define ADC_SWITCH_POS_2 1024
#define ADC_SWITCH_POS_3 1536
#define ADC_SWITCH_POS_4 2048
#define ADC_SWITCH_POS_5 2560
#define ADC_SWITCH_POS_6 3072
#define ADC_SWITCH_POS_7 3584
#define ADC_SWITCH_POS_8 4095

/* Maximum number of positions that a switch can support,
 * can be configured as per requirement */
#define SWITCH_MAX_POSITIONS 8

/* 4 switch positions have been considered, can be configured
 * by setting value of SWITCH_MAX_POSITIONS according to requirements. */
enum Switch_Position
{
    SWITCH_POS_0,
    SWITCH_POS_1,
    SWITCH_POS_2,
    SWITCH_POS_3,
    SWITCH_POS_4,
    SWITCH_POS_5,
    SWITCH_POS_6,
    SWITCH_POS_7,
    SWITCH_POS_8,
};

