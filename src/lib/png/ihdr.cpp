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

// #include "png/crc_public_code.h"
#include "png/png.h"

#include <sstream>

namespace NFIMM {

/**
 * Although this chunk is parsed, none of its bytes are modified; it is always
 * passed as-is to the destination header. Parsed values are output to log
 * for verification/inspection.
 *
 * @param mps needed to update the runtime log
 * @param chnk the IHDR chunk to parse
 */
IhdrX::IhdrX( std::shared_ptr<MetadataParameters> &mps, std::shared_ptr<PNG::ChunkLayout> &chnk )
{
  // mps->loggit( "INSIDE IhdrX::parseChunk(), chunk pointer index: " +
  //     std::to_string(_idx));
  mps->loggit( "IHDR: wholeChunkStr(): 0x" + chnk->wholeChunkStr() );
  mps->loggit( "IHDR length: " + std::to_string( chnk->length() ) );
  mps->loggit( "IHDR type: '"  + chnk->type() + "'" );
  mps->loggit( "IHDR data: 0x" + chnk->data() );
  mps->loggit( "IHDR CRC:  0x" + chnk->crc() );

  for( int i=0; i<NUM_BYTES_CHUNK_IHDR_TOTAL; i++ ) {
    PNG::s_oneByte = chnk->wholeChunkBuffer[i];
    _imageHDR.wholeChunk[i] = PNG::s_oneByte;
  }

  // Support for shifting of 4 bytes to calculate value.
  uint32_t tmp32Val{0};

  // Parse the chunk in order of appearance.
  // Chunk length (of data-part):
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_LENGTH; i++ ) {
    PNG::s_oneByte = _imageHDR.wholeChunk[i];
    _imageHDR.lenData[i] = PNG::s_oneByte;
    tmp32Val <<= 8;
    tmp32Val += PNG::s_oneByte;
  }
  _imageHDR.length = tmp32Val; tmp32Val = 0;
  mps->loggit( "IHDR len of data, should == 13: " +
                       std::to_string( _imageHDR.length ) );

  // Chunk type-name:
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_TYPE; i++ ) {
    PNG::s_oneByte = _imageHDR.wholeChunk[i+PNG::NUM_BYTES_CHUNK_TYPE];
    _imageHDR.typeBytes[i] = PNG::s_oneByte;
  }
  // Check type is correct.
  if( _imageHDR.tostring_type() != "IHDR" ) {
    std::string msg{"ERROR: invalid IHDR name: "};
    msg.append( _imageHDR.tostring_type() );
    throw Miscue( msg );
  }

  // START Chunk data.
  {
    // Get the entire data buffer first, then extract the width, height,
    // and 5 additional info-bytes.
    for( uint32_t i=0; i<_imageHDR.length; i++ ) {
      PNG::s_oneByte = _imageHDR.wholeChunk[i+8];
      _imageHDR.data[i] = PNG::s_oneByte;
    }

    // Width:
    for( int i=0; i<NUM_BYTES_IHDR_WIDTH; i++ ) {
      PNG::s_oneByte = _imageHDR.data[i];
      _imageHDR.imageInfo.dimension.widthBytes[i] = PNG::s_oneByte;
      tmp32Val <<= 8;
      tmp32Val += PNG::s_oneByte;
    }
    _imageHDR.imageInfo.dimension.width = tmp32Val;
    mps->loggit( "IHDR image width: " +
                      std::to_string( _imageHDR.imageInfo.dimension.width ) );
    tmp32Val = 0;
    // Height:
    for( int i=0; i<NUM_BYTES_IHDR_HEIGHT; i++ ) {
      PNG::s_oneByte = _imageHDR.data[i+4];
      _imageHDR.imageInfo.dimension.heightBytes[i] = PNG::s_oneByte;
      tmp32Val <<= 8;
      tmp32Val += PNG::s_oneByte;
    }
    _imageHDR.imageInfo.dimension.height = tmp32Val;
    mps->loggit( "IHDR image height: " +
                      std::to_string( _imageHDR.imageInfo.dimension.height ) );
    // Rest of the (5) bytes:
    _imageHDR.imageInfo.bitDepth          = _imageHDR.data[8];
    _imageHDR.imageInfo.colorType         = _imageHDR.data[9];
    _imageHDR.imageInfo.compressionMethod = _imageHDR.data[10];
    _imageHDR.imageInfo.filterMethod      = _imageHDR.data[11];
    _imageHDR.imageInfo.interlaceMethod   = _imageHDR.data[12];

  }
  // END Chunk data.

  // Chunk CRC.
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_CRC; i++ ) {
    PNG::s_oneByte = _imageHDR.wholeChunk[i+21];
    _imageHDR.crc[i] = PNG::s_oneByte;
  }

}   // END ctor IhdrX


/******************************************************************************/
/* struct ImageHDR methods implementations */

/** @return `IHDR` when source-image IHDR is correct */
std::string
IhdrX::ImageHDR::tostring_type() {
  std::stringstream ss;
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_TYPE; i++ ) { ss << typeBytes[i]; }
  return ss.str();
}


}   // END namespace
