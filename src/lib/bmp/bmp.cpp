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

#include "bmp/bmp.h"

#include <iostream>
#include <sstream>


namespace NFIMM {

/** 
 * Initialize the read-buffer cursor and clear the write-buffer.
*/
BMP::BMP( std::shared_ptr<MetadataParameters> &mps ) : NFIMM(mps)
{
  _params->loggit( "Initialize for BMP modification" );
  s_r_cursor = 0;
  s_writeBuffer.clear();
}

/**
 * Parse the image file's header and update with new parameters.
 * @throw Miscue Invalid image FILE or INFO header, calculated image-size
 *   mismatch
 */
void BMP::modify()
{
  std::unique_ptr<FileHeader> fileHeader(new FileHeader( _params ));
  std::unique_ptr<InfoHeader> infoHeader(new InfoHeader( _params ));
  try
  {
    fileHeader->read();
    _params->loggit( fileHeader->to_s( "READ file header:" ) );
    infoHeader->read();
    _params->loggit( infoHeader->to_s( "READ info header:" ) );
  }
  catch( const Miscue &e )
  {
    throw e;
  }
  
  // Check that File header calculated size image == Info header Size image
  if( fileHeader->_actual.calculated_size_image == infoHeader->_actual.size_image )
  {
    _params->loggit(
      "VALIDATION OK: FILEHEADER calculated size equals INFOHEADER file size." );
    _params->loggit(
      "calc size:   " + std::to_string( fileHeader->_actual.calculated_size_image ) );
    _params->loggit(
      "actual size: " + std::to_string( infoHeader->_actual.size_image ) );
  }
  else if( infoHeader->_actual.size_image == 0 )
  {
    std::string err{"INFOHEADER image-size: "};
    err.append( "calculated size: " +
      std::to_string( fileHeader->_actual.calculated_size_image ) );
    err.append( ", src image header actual size: " +
      std::to_string( infoHeader->_actual.size_image ) );
    err.append( "  where 0 is OK" );
    _params->loggit( err );
    infoHeader->_actual.size_image = fileHeader->_actual.fileSize
                                   - fileHeader->_actual.offsetToPixelData;
  }
  else
  {
    std::string err{"File header calculated image-size ERROR: "};
    err.append( "calc size: " +
      std::to_string( fileHeader->_actual.calculated_size_image ) );
    err.append( ", actual size: " +
      std::to_string( infoHeader->_actual.size_image ) );
    _params->loggit( err );
    throw Miscue( err );
  }
  
  // Read the pixel data; the s_r_cursor is the start point; the size of the
  // image pixel data is full size of file minus size of headers.
  uint32_t countPixelData = fileHeader->_actual.fileSize
                          - NUM_BYTES_BITMAPFILEHEADER
                          - infoHeader->_actual.headerCountBytes;

  std::vector<uint8_t> sourceImagePixels;
  readImagePixels( countPixelData, sourceImagePixels );

  // At this point, replace the file size, width, height, and sample rate.
  fileHeader->headerAsVector();
  infoHeader->update();
  infoHeader->headerAsVector();
  _params->loggit( fileHeader->to_s( "WRITE file header:" ) );
  _params->loggit( infoHeader->to_s( "WRITE info header:" ) );

  s_w_cursor = 0;
  // File header
  xferBytesBetweenBuffers( s_writeBuffer, fileHeader->_vecEntireHeader );
  // Info header
  xferBytesBetweenBuffers( s_writeBuffer, infoHeader->_vecEntireHeader );
  // Pixel data
  xferBytesBetweenBuffers( s_writeBuffer, sourceImagePixels );
}   // END modify()

/**
 * Read the remaining bytes from source image AFTER the two headers. Therefore,
 * the "starting point" for the read is the current read-cursor value.
 * 
 * @param len total num bytes to read
 * @param toVec receiving container for the bytes
 */
void BMP::readImagePixels( const uint32_t len, std::vector<uint8_t> &toVec )
{
  for( uint32_t i=NFIMM::s_r_cursor; i<len+NFIMM::s_r_cursor; i++ ) {
    toVec.push_back( NFIMM::s_readBuffer[i] );
  }
}

/** @return current Metadata Parameters */
std::string BMP::to_s()
{
  std::string s{"BMP: "};
  s.append( _params->to_s() );
  return s;
}

/**
 * @param to buffer to receive bytes
 * @param from buffer to take bytes
 */
void BMP::xferBytesBetweenBuffers( std::vector<uint8_t> &to,
                                   const std::vector<uint8_t> &from )
{
  for( uint8_t byte : from ) { to.push_back( byte ); }
}


}   // END namespace
