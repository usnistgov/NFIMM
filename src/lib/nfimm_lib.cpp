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

#include "nfimm_lib.h"

#include <iostream>
#include <sys/stat.h>


namespace NFIMM {


/** Initialize the metadata params container for receipt of user info and
 * logging. */
NFIMM::NFIMM( std::shared_ptr<MetadataParameters> &mps ) : _params(mps) {}

/** 1 byte -> [0-255] or [0x00-0xFF].
 *
 * @param val to parse into bytes
 * @param toBytes array of individual bytes
 * @param endian where true = big, false = little
 */
void NFIMM::expressUINT32AsFourBytes( const size_t val,
                                      uint8_t toBytes[],
                                      const bool endian ) {
  uint8_t oneByte;
  // Copy const val for shifting
  size_t tmp = val;

  if( endian ) {
    for( int i=4-1; i>=0; i-- ) { // save bytes in big-endian order
      oneByte = (tmp & 0xff);
      toBytes[i] = oneByte;
      tmp >>= 8;
    }
  }
  else {
    for( int i=0; i<4; i++ ) { // save bytes in little-endian order
      oneByte = (tmp & 0xff);
      toBytes[i] = oneByte;
      tmp >>= 8;
    }
  }
}

/**
 * Reads 4 consecutive bytes from the read-buffer vector; the current index
 * into this vector is maintained by variable static `s_r_cursor`.
 * The `s_r_cursor` is incremented by the number of bytes that were copied,
 * namely 4.
 *
 * @param toBytes[] the 4-bytes read from the image buffer
 */
void NFIMM::next4bytes( uint8_t toBytes[] )
{
  uint32_t val{0};
  for( uint32_t i=s_r_cursor,j=0; j<4; i++,j++ ) {
    toBytes[j] = s_readBuffer[i];
    val <<= 8;
    val += toBytes[j];
  }
  s_r_cursor += 4;
}

/**
 * Reads number of bytes from the source-image buffer specified by the caller
 * into the array. It is guaranteed by the caller that the `toBytes[]` array
 * -size is the exact number of bytes to copy.
 * 
 * Note that the source-image buffer has already been loaded and that the
 * cursor/index into buffer is current, that is, the cursor is initially set
 * by the caller; for all subsequent calls.  The `s_r_cursor` is incremented by
 * the number of bytes that were copied.
 * 
 * @param len next number of bytes parsed from the source-image buffer
 * @param toBytes[] buffer to load from the source-image buffer
 */
void NFIMM::nextLengthBytes( const uint32_t len, uint8_t toBytes[] )
{
  for( uint32_t i=s_r_cursor,j=0; j<len; i++,j++ ) {
    toBytes[j] = s_readBuffer[i];
  }
  s_r_cursor += len;
}

/**
 * @return title and current version number
 */
std::string printVersion()
{
  std::string s{ "NFIMMv" };
  s.append( NFIMM_VERSION );
  return s;
}

/**
 * Clear the read-buffer (vector) and read the image-bytes into it.
 *
 * @param path to source image
 * @throw Miscue If file cannot be opened or is zero size
 */
void NFIMM::readImageFileIntoBuffer( const std::string &path )
{
  std::fstream strm;
  strm.open( path, std::ios::in|std::ios::binary );
  if( !strm )
    throw Miscue( "CANNOT open file: '" + path + "'" );

  std::vector<uint8_t> contents(
    (std::istreambuf_iterator<char>(strm)), std::istreambuf_iterator<char>() );
  s_readBuffer.clear();
  s_readBuffer = contents;
  strm.close();
}

/**
 * Clear the read-buffer (vector) and transfer the image-bytes into it.
 *
 * @param vec IN : source image
 */
void NFIMM::readImageFileIntoBuffer( const std::vector<uint8_t> vec )
{
  s_readBuffer.clear();
  s_readBuffer = vec;
}

/**
 * Transfers the image write-buffer bytes into the vector and then clears
 * the write-buffer.
 *
 * @param vec OUT : destination image
 */
void NFIMM::retrieveWriteImageBuffer( std::vector<uint8_t> &vec )
{
  vec = s_writeBuffer;
  s_writeBuffer.clear();
}

/**
 * Save the Write-buffer to file.
 *
 * @param path to destination image
 * @throw Miscue If file cannot be opened or is zero size
 */
void NFIMM::writeImageBufferToFile( const std::string &path )
{
  std::fstream strm;
  strm.open( path, std::ios::out|std::ios::binary );
  if( !strm )
    throw Miscue( "CANNOT open output image file: '" + path + "'" );

  for (const auto &e : s_writeBuffer) strm << e;
  strm.close();
}

/** BMP format uses little-endian. 1 byte -> [0-255] or [0x00-0xFF].
 *
 * @param val OUT the result
 * @param fromBytes array of individual bytes
 * @param endian where true = big, false = little
 */
void NFIMM::expressFourBytesAsUINT32( uint32_t &val,
                                      const uint8_t fromBytes[],
                                      const bool endian ) {
  uint8_t oneByte;
  uint32_t tmp = 0;

  if( !endian ) {
    for( int i=4-1; i>=0; i-- ) { // save bytes in big-endian order
      oneByte = fromBytes[i];
      tmp <<= 8;
      tmp += oneByte;
    }
    val = tmp;
  }
  else {
    for( int i=0; i<4; i++ ) { // save bytes in little-endian order
      // oneByte = (tmp & 0xff);
      // toBytes[i] = oneByte;
      tmp >>= 8;
    }
  }
}

/** BMP format uses little-endian. 1 byte -> [0-255] or [0x00-0xFF].
 *
 * @param val OUT the result
 * @param fromBytes array of individual bytes
 * @param endian where true = big, false = little
 */
void NFIMM::expressTwoBytesAsUINT16( uint16_t &val,
                                      const uint8_t fromBytes[],
                                      const bool endian ) {
  uint8_t oneByte;
  uint16_t tmp = 0;

  if( !endian ) {
    for( int i=2-1; i>=0; i-- ) { // save bytes in big-endian order
      oneByte = fromBytes[i];
      tmp <<= 8;
      tmp += oneByte;
    }
    val = tmp;
  }
  else {
    for( int i=0; i<2; i++ ) { // save bytes in little-endian order
      // oneByte = (tmp & 0xff);
      // toBytes[i] = oneByte;
      tmp >>= 8;
    }
  }
}

/** 0.0254in == 1mm
 * @param ppmm sample rate in pixels per millimeter
 * @param ppi sample rate in pixels per inch
 */
void NFIMM::convertPPMMtoPPI( const uint32_t ppmm, uint32_t &ppi )
{
  const double inch_per_mm{0.0254};
  ppi = static_cast<uint32_t>(ppmm * inch_per_mm);
}

/** 39.37mm == 1in
 * @param ppi sample rate in pixels per inch
 * @param ppmm sample rate in pixels per millimeter
 */
void NFIMM::convertPPItoPPMM( const uint32_t ppi, uint32_t &ppmm )
{
  const double inch_per_mm{0.0254};
  ppmm = static_cast<uint32_t>(ppi / inch_per_mm);
}

}   // END namespace
