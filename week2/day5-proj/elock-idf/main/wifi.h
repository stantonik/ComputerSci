/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : wifi
 * @created     : Tuesday Oct 08, 2024 09:56:13 CEST
 */

#ifndef WIFI_H
#define WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*          INCLUDES          */
/******************************/
#include "esp_err.h"

/******************************/
/*      Macro Definitions     */
/******************************/
#define SSID "Stanley's e-lock"
#define PASSWORD "ouiouioui"
#define CHANNEL 6

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t wifi_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* WIFI_H */

