/* See Project CHIP LICENSE file for licensing information. */

#include <platform/logging/LogV.h>

#include <lib/core/CHIPConfig.h>
#include <lib/support/EnforceFormat.h>
#include <lib/support/logging/Constants.h>
#include <stdio.h>

#define ENABLE_CONSOLE_COLOR_PRINT  0

// Forground color
#define __FG_TXT_CLR_RESET__     "\e[0m"
#define __FG_TXT_CLR_BLACK__     "\e[0;30m"
#define __FG_TXT_CLR_RED__       "\e[0;31m"
#define __FG_TXT_CLR_GREEN__     "\e[0;32m"
#define __FG_TXT_CLR_YELLOW__    "\e[0;33m"
#define __FG_TXT_CLR_BLUE__      "\e[0;34m"
#define __FG_TXT_CLR_WHITE__     "\e[0;37m"
// Forground bold color
#define __FG_TXT_CLR_RESET_B__     "\e[1m"
#define __FG_TXT_CLR_BLACK_B__     "\e[1;30m"
#define __FG_TXT_CLR_RED_B__       "\e[1;31m"
#define __FG_TXT_CLR_GREEN_B__     "\e[1;32m"
#define __FG_TXT_CLR_YELLOW_B__    "\e[1;33m"
#define __FG_TXT_CLR_BLUE_B__      "\e[1;34m"
#define __FG_TXT_CLR_WHITE_B__     "\e[1;37m"

// Background color
#define __BG_TXT_CLR_RESET__     "\e[0m"
#define __BG_TXT_CLR_BLACK__     "\e[0;40m"
#define __BG_TXT_CLR_RED__       "\e[0;41m"
#define __BG_TXT_CLR_GREEN__     "\e[0;42m"
#define __BG_TXT_CLR_YELLOW__    "\e[0;43m"
#define __BG_TXT_CLR_BLUE__      "\e[0;44m"
#define __BG_TXT_CLR_WHITE__     "\e[0;47m"

int os_get_boot_arg_int(const char *name, int defval);

namespace chip {
namespace Logging {
namespace Platform {

void console_print(const char * module, const char * msg, va_list v, char *clr)
{
    static int color_log = os_get_boot_arg_int("matter.color_log", 0);
    char tag[150] = {0};

    if((color_log == 1) && (clr != nullptr)) {
        snprintf(tag, sizeof(tag), "%s\n MATTER[%s][%d]: ", clr, module, xTaskGetTickCount());
    } else {
        snprintf(tag, sizeof(tag), "\n MATTER[%s][%d]: ", module, xTaskGetTickCount());
    }

    os_printf(tag);
    os_vprintf(msg, v);
    va_end(v);

    if((color_log == 1) && (clr != nullptr))
    {
        os_printf(__FG_TXT_CLR_RESET__);
    }
}

void ENFORCE_FORMAT(3, 0) LogV(const char * module, uint8_t category, const char * msg, va_list v)
{
    switch (category)
    {
        case kLogCategory_Error:
        {
            console_print(module, msg, v, (char*)__FG_TXT_CLR_RED_B__);
            break;
        }

        case kLogCategory_Detail:
        {
            console_print(module, msg, v, (char*)__FG_TXT_CLR_BLUE__);
            break;
        }

        case kLogCategory_Progress:
        default:
        {
            console_print(module, msg, v, (char*)__FG_TXT_CLR_GREEN__);
            break;
        }
    }
}

} // namespace Platform
} // namespace Logging
} // namespace chip
