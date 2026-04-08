/**
 * Copyright (c) 2026 by 6Bolt
 *
 * 
 */


#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif


// Enable Device stack
#define CFG_TUD_ENABLED         1

// Legacy RHPORT configuration
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT        0
#endif
// end legacy RHPORT


//------------------------
// DEVICE CONFIGURATION //
//------------------------

// Enable CDC class
#define CFG_TUD_CDC             1

// Set CDC FIFO buffer sizes
//#define CFG_TUD_CDC_RX_BUFSIZE  (64)
//#define CFG_TUD_CDC_TX_BUFSIZE  (64)
//#define CFG_TUD_CDC_EP_BUFSIZE  (64)

// CDC FIFO size of TX and RX
#define CFG_TUD_CDC_RX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_CDC_TX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)

// CDC Endpoint transfer buffer size, more is faster
#define CFG_TUD_CDC_EP_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  64
#endif


#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */
