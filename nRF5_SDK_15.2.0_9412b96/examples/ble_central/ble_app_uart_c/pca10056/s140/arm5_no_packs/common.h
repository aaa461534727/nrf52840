#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/*********************************************************************
 * @fn      Util_convertBdAddr2Str
 *
 * @brief   Convert Bluetooth address to string. Only needed when
 *          LCD display is used.
 *
 * @param   pAddr - BD address
 *
 * @return  BD address as a string
 */
extern char *Util_convertBdAddr2Str(uint8_t *pAddr);

/*********************************************************************
 * @fn      Util_convertHex2Str
 *
 * @brief   将HEX转成String
 *
 * @param   Hex - Hex
 *          len - Hex len
 *
 * @return  Hex as a string
 */
extern char *Util_convertHex2Str(uint8_t *Hex, uint16_t Len);

#endif // COMMON_H
