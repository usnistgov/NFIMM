/*******************************************************************************
License:
This software was developed at the National Institute of Standards and
Technology (NIST) by employees of the Federal Government in the course
of their official duties. Pursuant to title 17 Section 105 of the
United States Code, this software is not subject to copyright protection
and is in the public domain. NIST assumes no responsibility  whatsoever for
its use by other parties, and makes no guarantees, expressed or implied,
about its quality, reliability, or any other characteristic.

This software has been determined to be outside the scope of the EAR
(see Part 734.3 of the EAR for exact details) as it has been created solely
by employees of the U.S. Government; it is freely distributed with no
licensing requirements; and it is considered public domain. Therefore,
it is permissible to distribute this software as a free download from the
internet.

Disclaimer:
This software was developed to promote biometric standards and biometric
technology testing for the Federal Government in accordance with the USA
PATRIOT Act and the Enhanced Border Security and Visa Entry Reform Act.
Specific hardware and software products identified in this software were used
in order to perform the software development.  In no case does such
identification imply recommendation or endorsement by the National Institute
of Standards and Technology, nor does it imply that the products and equipment
identified are necessarily the best available for the purpose.
*******************************************************************************/
#include "png/crc_public_code.h"

#include <iomanip>
#include <sstream>

namespace CRCforPNG {

/** @brief Table of Cyclic Redundancy Checksums for all 8-bit bytes */
uint32_t crc_table[256];

/** Calc table only once. */
void buildCRCtable()
{
  static bool crc_table_generated{false};
  if( crc_table_generated == true ) return;

  uint32_t c;

  for( int n=0; n<256; n++ )
  {
    c = (uint32_t)n;
    for( int k=0; k<8; k++ ) {
      if( c & 1 ) {
        c = 0xedb88320L ^ ( c >> 1 );
      } 
      else {
        c = c >> 1;
      }
    }
    crc_table[n] = c;
  }
  crc_table_generated = true;
}


/**
 * The CRC should be initialized to all 1’s, and the transmitted value is
 * the 1’s complement of the final running CRC.
 *
 * @param buf pointer to the array of bytes on which to calculate the CRC
 * @param len length of current array (buf)
 * @return 1's complement CRC
 */
uint32_t calc( uint8_t *buf, int len )
{
  return updateCRC( 0xffffffffL, buf, len ) ^ 0xffffffffL;
}


/**
 * @return the CRC table with column header on top.
 */
std::string to_s_CRCtable()
{
  buildCRCtable();

  std::stringstream ss{};
  ss << "CRC_TABLE\n  N  dec(CRC(N))          hex(CRC(N))\n";

  for( int n=0; n<256; n++ ) {
    ss << std::dec << std::setw(3)  << n << "  ";
    ss << std::dec << std::setw(12) << crc_table[n] << "  ";
    ss << std::hex << std::setw(8)  << "0x" << crc_table[n] << "\n";
  }
  return ss.str();
}


/**
 * The value of CRC should have been initialized to all 1's, and the
 * transmitted value is the 1's complement of the final running CRC.
 * 
 * @param crc current value of the CRC
 * @param buf pointer to the array of bytes on which to calculate the CRC
 * @param len length of current array (buf)
 * @return the updated CRC
 */
uint32_t updateCRC( const uint32_t crc, uint8_t *buf, int len )
{
  uint32_t c = crc;

  buildCRCtable();

  for( int n=0; n<len; n++ ) {
    c = crc_table[ ( c ^ buf[n] ) & 0xff ] ^ ( c >> 8 );
  }
  return c;
}

}   // END namespace
