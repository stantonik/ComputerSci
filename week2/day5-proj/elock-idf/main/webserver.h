/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : websocket
 * @created     : Tuesday Oct 08, 2024 10:08:42 CEST
 */

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*          INCLUDES          */
/******************************/
#include "esp_err.h"
#include "esp_http_server.h"

/******************************/
/*      Macro Definitions     */
/******************************/

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/

/******************************/
/*     Global Variables       */
/******************************/
extern httpd_handle_t server;
extern int fd;

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t ws_init();
extern void ws_async_send(const char *key, const char *val);
extern esp_err_t ws_set_callback(void (*)(const char *));

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* WEBSOCKET_H */
