#ifndef USB_CONFIG_H
#define USB_CONFIG_H

// Общие настройки USB
#define USBD_DBGPRINTF(...)
#define USBD_EP_NUM          4 // Количество эндпоинтов (EP0 + EP_AUDIO)

// Настройки буферов CherryUSB для RP2040
#define USB_OUT_EP_MP_NUM    2
#define USB_IN_EP_MP_NUM     2

// Специфичные настройки Audio Class

#define IN_CHANNEL_NUM       6     // количество каналов АЦП
#define OUT_CHANNEL_NUM      2     // количество каналов ЦАП

#define AUDIO_IN_MAX_FREQ    48000
#define AUDIO_OUT_MAX_FREQ   96000

#define UAC_BIT_RESOLUTION   16
#define USING_FEEDBACK       1


#define USBD_VID             0xDEAD        //0xffff
#define USBD_PID            (0xBEAF - 43)  //0xffff


#define CONFIG_USB_PRINTF(...) // заглушка. Можно перенаправить логи в printf, если нужно
#define CONFIG_USB_ALIGN_SIZE   4
#define USB_NOCACHE_RAM_SECTION
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN  1024
#define CONFIG_USBDEV_MAX_BUS   1




#endif /* USB_CONFIG_H */
