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
 * @param mps needed to update the runtime log
 * @param chnk the pHYs chunk to parse
 */
Phys::Phys( std::shared_ptr<MetadataParameters> &mps,
            std::shared_ptr<PNG::ChunkLayout> &chnk )
     : _params(mps), _chnk(chnk) {}

/**
 * The chunk object is newly created and the reference to the object is saved
 * to the container of insertion pointers.  This reference occurs before
 * all inserted `tEXt` chunk pointers.
 *
 * @throw Miscue Cannot allocate chunk shared pointer, or chunk's dataBuffer
 */
void Phys::insertChunk()
{
  std::shared_ptr<PNG::ChunkLayout> pchunk;
  try
  {
    pchunk.reset( new PNG::ChunkLayout );
  }
  catch( const std::bad_weak_ptr bwPtr )
  {
      throw Miscue( "pHYs chunk-layout bad_weak_ptr" );
  }

  // Chunk is valid, append the object to container that is iterated
  // upon write to output buffer and update index.
  PNG::s_insertChunkPointers.push_back( pchunk );
  PNG::_insertChunkIndex++;

  // Load the type chars.
  pchunk->typeBytes[0] = 'p';
  pchunk->typeBytes[1] = 'H';
  pchunk->typeBytes[2] = 'Y';
  pchunk->typeBytes[3] = 's';

  _params->loggit( "PNG::Phys insertChunk: " + pchunk->type() );
  _params->loggit( "PNG::Phys _insertChunkIndex: " +
                    std::to_string( PNG::_insertChunkIndex) );

  // Update the chunk's data length.
  NFIMM::expressUINT32AsFourBytes( NUM_BYTES_PHYS_DATA, pchunk->lengthBytes,
                                   true );

  // Build the chunk data part.
  // Retrieve the destination sample-rate/resolution from user-specified
  // metadata parameters object.
  const uint32_t destSampleRate{_params->destImg.resolution.horiz};
  uint32_t sampratemm{destSampleRate};

  // Retrieve the sample-rate/resolution units from user-specified
  // metadata parameters object.
  std::string units = _params->destImg.resolution.unitsStr;
  if( units == "inch" ) {
    _params->loggit( "Resolution update units: 'inch'" );
    NFIMM::convertPPItoPPMM( destSampleRate, sampratemm );
    _params->loggit( "Convert resolution: " + std::to_string( destSampleRate ) +
                     "PPI = " + std::to_string(sampratemm) + "PPMM" );
  }
  pchunk->dataBuffer = new uint8_t[NUM_BYTES_PHYS_DATA];
  if( !pchunk->dataBuffer )
  {
    throw Miscue( "cannot allocate text chunk data buffer: 'pHYs'" );
  }

  // Push the resolution and units to the data-part.
  uint8_t resolutionBytes[4];
  NFIMM::expressUINT32AsFourBytes( sampratemm, resolutionBytes, true );
  for( int i=0; i<9; i++ ) {
    // horizontal:
    for( int j=0; j<4; j++ ) {
      pchunk->dataBuffer[i] = resolutionBytes[j];  i++;
    }
    // vertical:
    for( int j=0; j<4; j++ ) {
      pchunk->dataBuffer[i] = resolutionBytes[j];  i++;
    }
    // units: ALWAYS updated to units = meter
    pchunk->dataBuffer[i] = BYTE_PHYS_UNITS;
  }

  // Concat the type- and data-parts of the chunk to calculate the CRC.
  uint8_t cat[PNG::NUM_BYTES_CHUNK_TYPE+NUM_BYTES_PHYS_DATA];
  int ix{0};
  for( ix=0; ix<PNG::NUM_BYTES_CHUNK_TYPE; ix++ ) {
    cat[ix] = pchunk->typeBytes[ix];
  }
  for( ix=PNG::NUM_BYTES_CHUNK_TYPE;
       ix<PNG::NUM_BYTES_CHUNK_TYPE+NUM_BYTES_PHYS_DATA;
       ix++ ) {
    cat[ix] = pchunk->dataBuffer[ix-PNG::NUM_BYTES_CHUNK_TYPE];
  }

  uint32_t crc_calculated{0};
  crc_calculated = CRCforPNG::calc( cat,
    PNG::NUM_BYTES_CHUNK_TYPE+NUM_BYTES_PHYS_DATA );

  // Update the CRC-bytes.
  NFIMM::expressUINT32AsFourBytes( crc_calculated,
                                   pchunk->crcBytes,
                                   true );

  // Concatenate the 4-parts into a single buffer
  pchunk->concatenate4parts();

  _params->loggit( "pHYs whole chunk: " + pchunk->wholeChunkStr() );
  _params->loggit( "pHYs CRC calculated = 0x" + pchunk->crc() );
  // Increment the count
  _params->pngWriteImageInfo.countInsertChunks++;

}   // END insertChunk()


/**
 * The `pHYs` chunk contains 9 bytes of data, and the total length of this
 * chunk is 21 bytes:
 *   - LEN:  4 bytes
 *   - pHYs: 4 bytes, string `pHYs`
 *   - DATA: 9 bytes, horiz & vert resolution and units
 *   - CRC:  4 bytes
 * 
 * Parse the DATA part of the chunk, in order of appearance, that has been
 * extracted from the image in the readBuffer.  This includes the horizontal
 * and vertical resolution and units.  Values are saved to the ImageResolution
 * struct.
 */
void Phys::parseChunk()
{
  _params->loggit( "INSIDE Phys::parseChunk()" );
  _params->loggit( "PHYS: wholeChunkStr(): 0x" +
                       _chnk->wholeChunkStr() );
  _params->loggit( "Phys length: " +
                    std::to_string( _chnk->length() ) );
  _params->loggit( "Phys type: '" + _chnk->type() + "'" );
  _params->loggit( "Phys data: 0x" + _chnk->data() );
  _params->loggit( "Phys CRC:  0x" + _chnk->crc() );

  for( uint32_t i=0; i<_chnk->length()+12; i++ ) {
    PNG::s_oneByte = _chnk->wholeChunkBuffer[i];
    _imagepHYs.wholeChunk[i] = PNG::s_oneByte;
  }

  // Support for shifting of 4 bytes to calculate value.
  uint32_t tmp32Val{0};

  // Parse the chunk in order of appearance.
  // Chunk length (of data-part):
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_LENGTH; i++ ) {
    PNG::s_oneByte = _imagepHYs.wholeChunk[i];
    _imagepHYs.lenData[i] = PNG::s_oneByte;
    tmp32Val <<= 8;
    tmp32Val += PNG::s_oneByte;
  }
  _imagepHYs.length = tmp32Val;
  tmp32Val = 0;
  _params->loggit( "pHYs len of data, should == 9: " +
                    std::to_string( _imagepHYs.length ) );

  // Chunk type-name:
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_TYPE; i++ ) {
    PNG::s_oneByte = _imagepHYs.wholeChunk[i+PNG::NUM_BYTES_CHUNK_TYPE];
    _imagepHYs.typeBytes[i] = PNG::s_oneByte;
  }
  // Check type is correct.
  if( _imagepHYs.tostring_type() != "pHYs" ) {
    std::string msg{"ERROR: invalid pHYs name: "};
    msg.append( _imagepHYs.tostring_type() );
    throw Miscue( msg );
  }

  // START Chunk data.
  // Horizontal resolution:
  {
    for( int i=0; i<NUM_BYTES_PHYS_RESOLUTION; i++ ) {
      PNG::s_oneByte = _chnk->dataBuffer[i];
      _imagepHYs.imageResolution.horizontalBytes[i] = PNG::s_oneByte;
      tmp32Val <<= 8;
      tmp32Val += PNG::s_oneByte;
    }
    _imagepHYs.imageResolution.horizontal = tmp32Val;
    tmp32Val = 0;
    _params->loggit( "pHYs " + _imagepHYs.imageResolution.horizBytesHex() );

    // Vertical resolution:
    for( int i=0; i<NUM_BYTES_PHYS_RESOLUTION; i++ ) {
      PNG::s_oneByte = _chnk->dataBuffer[i+NUM_BYTES_PHYS_RESOLUTION];
      _imagepHYs.imageResolution.verticalBytes[i] = PNG::s_oneByte;
      tmp32Val <<= 8;
      tmp32Val += PNG::s_oneByte;
    }
    _imagepHYs.imageResolution.vertical = tmp32Val;
    _params->loggit( "pHYs " + _imagepHYs.imageResolution.vertBytesHex() );
    _params->srcImg.existingPhysResolution = tmp32Val;

    // Units:
    _imagepHYs.imageResolution.units =
      _chnk
        ->dataBuffer[NUM_BYTES_PHYS_RESOLUTION+NUM_BYTES_PHYS_RESOLUTION];
    _params->loggit( "pHYs sample-rate info:\n" + _imagepHYs.imageResolution.to_s() );
  }
  // END Chunk data.

  // Chunk CRC.
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_CRC; i++ ) {
    PNG::s_oneByte = _imagepHYs.wholeChunk[i+8+_imagepHYs.length];
    _imagepHYs.crc[i] = PNG::s_oneByte;
  }
}   // END parseChunk()

/**
 * Update the chunk data's horizontal and vertical bytes and the units-byte per
 * the Metadata Parameters object that is passed from the user of NFIMM.  This
 * "new" chunk is the one written to the destination image (hence replacing the
 * source image chunk).
 * 
 * The `pHYs` chunk is ALWAYS set to use 'meter' as units.  Therefore, if the
 * destination sample rate is specified as PPI, then it is converted to meters.
 * 
 * The CRC is calculated.
 * 
 * The write buffer is built by "transferring bytes" from each of the 4 parts
 * of a chunk (and not the chunk "as a whole").  (This is the implementation
 * for this library).
 * 
 * Only the data[] and crc[] member-objects are updated:
 * - the `_remainingChunkPointers[_idx]` object (over-write source
 *     data and CRC)
 * - the `_imagepHYs` object is left intact with source image info
 * 
 * Since the majority of the chunks are unmodified and passed "as-is" to the
 * write-buffer, the source image bytes (objects) are made directly available
 * to the write-buffer (the destination image). Therefore, to manifest updates
 * to this (and any chunk's) data-buffer, the source object's chunk data buffer
 * and CRC are overwritten.
 * 
 * The CRC for 1000PPI in meters is `E3 91 A4 22` :
 * - The entire chunk is:
 *   `--- LEN --------- pHYs -------- HORIZ ------- VERT --- Units --- CRC`
 * - The entire chunk is:
 *   `00 00 00 09 - 70 48 59 73 - 00 00 99 CA - 00 00 99 CA - 01 - E3 91 A4 22`
 */
void Phys::updateChunk()
{
  _params->loggit( "INSIDE PNG::Phys updateChunk()" );

  // Retrieve the destination sample-rate/resolution from user-specified
  // metadata parameters object.
  const uint32_t destSampleRate{_params->destImg.resolution.horiz};
  uint32_t sampratemm{destSampleRate};

  // Retrieve the sample-rate/resolution units from user-specified
  // metadata parameters object.
  std::string units = _params->destImg.resolution.unitsStr;
  if( units == "inch" ) {
    _params->loggit( "Resolution update units: 'inch'" );
    NFIMM::convertPPItoPPMM( destSampleRate, sampratemm );
    _params->loggit( "Convert destination resolution: " + std::to_string( destSampleRate ) +
                     "PPI = " + std::to_string(sampratemm) + "PPMM" );
  }
  else if( units == "meter" )
    _params->loggit( "Resolution update units: 'meter'" );
  else if( units == "other" )
    _params->loggit( "Resolution update units: 'other'" );
  else {
    std::string msg{"ERROR: invalid pHYs resolution units: "};
    msg.append( units );
    throw Miscue( msg );
  }

  // Update the data[] buffer with resolution and units.
  uint8_t resolutionBytes[4];
  NFIMM::expressUINT32AsFourBytes( sampratemm, resolutionBytes, true );
  NFIMM::expressUINT32AsFourBytes( sampratemm, _chnk->crcBytes, true );

  for( int i=0; i<8; i++ ) {
    // horizontal:
    for( int j=0; j<4; j++ ) {
      _chnk->dataBuffer[i] = resolutionBytes[j];  i++;
    }
    // vertical:
    for( int j=0; j<4; j++ ) {
      _chnk->dataBuffer[i] = resolutionBytes[j];  i++;
    }
    // units: ALWAYS updated to units = meter
    _chnk->dataBuffer[i] = BYTE_PHYS_UNITS;
  }

  // Concat the type- and data-parts of the chunk to calculate the CRC.
  uint8_t cat[PNG::NUM_BYTES_CHUNK_TYPE+NUM_BYTES_PHYS_DATA];
  int ix{0};
  for( ix=0; ix<PNG::NUM_BYTES_CHUNK_TYPE; ix++ ) {
    cat[ix] = _chnk->typeBytes[ix];
  }
  for( ix=PNG::NUM_BYTES_CHUNK_TYPE;
       ix<PNG::NUM_BYTES_CHUNK_TYPE+NUM_BYTES_PHYS_DATA;
       ix++ ) {
    cat[ix] = _chnk->dataBuffer[ix-PNG::NUM_BYTES_CHUNK_TYPE];
  }

  uint32_t crc_calculated{0};
  crc_calculated = CRCforPNG::calc( cat,
    PNG::NUM_BYTES_CHUNK_TYPE+NUM_BYTES_PHYS_DATA );

  // Update the CRC-bytes.
  NFIMM::expressUINT32AsFourBytes( crc_calculated, _chnk->crcBytes, true );
  _params->loggit( "pHYs CRC calculated = 0x" + _chnk->crc() );

  _chnk->concatenate4parts();

  _params->loggit( "PHYS: updated wholeChunkStr(): 0x" +
                   _chnk->wholeChunkStr() );
}   // END updateChunk()



/******************************************************************************/
/* struct ImagepHYs methods implementations */

/** @return return data in hex format */
std::string
Phys::ImagepHYs::tostring_data() {
  char hex[3];
  std::string s{};
  for( int i=0; i<NUM_BYTES_PHYS_DATA; i++ ) {
    sprintf( hex, "%02X", data[i] );
    s.append( hex );
  }
  return s;
}

/** @return string chunk's type name */
std::string
Phys::ImagepHYs::tostring_type() {
  std::stringstream ss;
  for( int i=0; i<PNG::NUM_BYTES_CHUNK_TYPE; i++ ) { ss << typeBytes[i]; }
    return ss.str();
}

/** @return full, single string with all data */
std::string
Phys::ImagepHYs::to_s() {
  char hex[3];
  std::string s{"pHYs data:\n"};

  std::string sw{" * whole chunk: 0x"};
  for( int i=0; i<NUM_BYTES_CHUNK_PHYS_TOTAL; i++ ) {
    sprintf( hex, "%02X", wholeChunk[i] );
    sw.append( hex );
  }

  s.append( sw + "\n" );
  s.append( imageResolution.to_s() + "\n" );
  return s;
}

/******************************************************************************/
/* struct ImageResolution methods implementations */

/** @return horiz resolution as 4-bytes strung together */
std::string
Phys::ImageResolution::horizBytesHex() {
  char hex[3];
  std::string s{"Horiz: 0x"};
  for( int i=0; i< NUM_BYTES_PHYS_RESOLUTION; i++ ) {
    sprintf( hex, "%02X", horizontalBytes[i] );
    s.append( hex );
  }
  return s;
}

/** @return full, single string with all data */
std::string
Phys::ImageResolution::to_s() {
  std::string s{};
  s.append( resolution_to_s() );
  s.append( units_to_s() );
  return s;
}

/** @return units-byte and corresponding text */
std::string
Phys::ImageResolution::units_to_s() {
  char hex[3];
  sprintf( hex, "%02X", units );
  std::string s{" * Units: 0x"};
  s.append( hex );
  std::string t{};
  if( units == 1 )
    t = " (meters)";
  else
    t = " (other)";
  s.append( t );
  return s;
}

/** @return vert resolution as 4-bytes strung together */
std::string
Phys::ImageResolution::vertBytesHex() {
  char hex[3];
  std::string s{"Vert: 0x"};
  for( int i=0; i< NUM_BYTES_PHYS_RESOLUTION; i++ ) {
    sprintf( hex, "%02X", verticalBytes[i] );
    s.append( hex );
  }
  return s;
}

/** @return horiz and vert resolutions of image */
std::string
Phys::ImageResolution::resolution_to_s() {
  std::string s{" * Resolution: "};
  s.append( std::to_string(horizontal) );
  s.append( " horiz, " );
  s.append( std::to_string(vertical) );
  s.append( " vert\n" );
  return s;
}

}   // END namespace
