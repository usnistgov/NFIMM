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

namespace NFIMM {

/**
 * @param mps needed to update the runtime log
 */

InfoHeader::InfoHeader( std::shared_ptr<MetadataParameters> &mps )
           : _params(mps) {}

/** Load each data element into single container. */
void InfoHeader::headerAsVector() {

  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biSize[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biWidth[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biHeight[i] ); }
  for( int i=0; i<2; i++ ) { _vecEntireHeader.push_back( _biPlanes[i] ); }
  for( int i=0; i<2; i++ ) { _vecEntireHeader.push_back( _biBitCount[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biCompression[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biSizeImage[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biXPelsPerMeter[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biYPelsPerMeter[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biClrUsed[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _biClrImportant[i] ); }
}

/**
 * Immediately follows the BMP File Header. NFIMM only supports header that
 * contains exactly 40 bytes.
 *
 *  Specified to contain the following values:\n
 *   bytes 0-3:    size of this header - must == 40\n
 *   bytes 4-7:    image width\n
 *   bytes 8-11:   image height\n
 *   bytes 12-13:  count of planes - must == 1\n
 *   bytes 14-15:  bits/pixel 1,4,8,16,24 or 32\n
 *   bytes 16-19:  compression type\n
 *   bytes 20-23:  image size, may be 0 if not compressed\n
 *   bytes 24-27:  horizontal sample rate pixels/meter\n
 *   bytes 28-31:  vertical sample rate pixels/meter\n
 *   bytes 32-35:  num entries in color map that are used\n
 *   bytes 36-39:  num significant colors
 *
 * Reads the entire header and saves accordingly.
 *
 * If the size of the header is not equal to 40 bytes return immediately and
 * forgo reading the rest of the header.
 */
void InfoHeader::read()
{
  NFIMM::next4bytes( _biSize );
  // Validate that the header == 40 bytes
  NFIMM::expressFourBytesAsUINT32( _actual.headerCountBytes, _biSize, false );
  if( _actual.headerCountBytes == BMP::NUM_BYTES_DIB_BITMAPINFOHEADER )
  {}
  else {
    std::string err{"ERROR: INFOHEADER size not == 40 bytes, is "};
    err += std::to_string( _actual.headerCountBytes );
    _params->loggit( err );
    throw Miscue( err );
  }

  NFIMM::next4bytes( _biWidth );
  NFIMM::expressFourBytesAsUINT32( _actual.width, _biWidth, false );
  // Row size (i.e. the width of the image) is "padded" to align on 4-byte
  // boundary. If modulo remainder > 0, then to align on the boundary,
  // increment the width. The calculated width X height == INFOHEADER's
  // biSizeImage.
  // As a result, the FILEHEADER's bfSize is the sum of the biSizeImage and
  // the bfOffBytes.
  if( _actual.width % 4 > 0 )
    _actual.padded_width = _actual.width + 1;
  NFIMM::next4bytes( _biHeight );
  NFIMM::expressFourBytesAsUINT32( _actual.height, _biHeight, false );
  NFIMM::nextLengthBytes( 2, _biPlanes );
  NFIMM::expressTwoBytesAsUINT16( _actual.count_planes, _biPlanes, false );
  NFIMM::nextLengthBytes( 2, _biBitCount );
  NFIMM::expressTwoBytesAsUINT16( _actual.bit_depth, _biBitCount, false );

  NFIMM::next4bytes( _biCompression );
  NFIMM::expressFourBytesAsUINT32( _actual.compression_type, _biCompression, false );

  NFIMM::next4bytes( _biSizeImage );
  NFIMM::expressFourBytesAsUINT32( _actual.size_image, _biSizeImage, false );

  NFIMM::next4bytes( _biXPelsPerMeter );
  NFIMM::expressFourBytesAsUINT32( _actual.horizontal_ppmm, _biXPelsPerMeter, false );
  NFIMM::convertPPMMtoPPI( _actual.horizontal_ppmm, _actual.horizontal_ppi );

  NFIMM::next4bytes( _biYPelsPerMeter );
  NFIMM::expressFourBytesAsUINT32( _actual.vertical_ppmm, _biYPelsPerMeter, false );
  NFIMM::convertPPMMtoPPI( _actual.vertical_ppmm, _actual.vertical_ppi );

  NFIMM::next4bytes( _biClrUsed );
  NFIMM::expressFourBytesAsUINT32( _actual.colors_used, _biClrUsed, false );
  NFIMM::next4bytes( _biClrImportant );
  NFIMM::expressFourBytesAsUINT32( _actual.colors_important, _biClrImportant, false );
}


/**
 * @param step which step in the process: read, update, write.
 * @return the InfoHeader bytes as a concatenated string
 */
std::string InfoHeader::to_s( const std::string &step ) {
  std::string s{};
  s.append( step );
  s.append( " INFOHEADER actuals:\n" );
  s.append( "  Count bytes:  " + std::to_string(_actual.headerCountBytes) + "\n" );
  s.append( "  File width:   " + std::to_string(_actual.width) + "\n");
  s.append( "  Padded width: " + std::to_string(_actual.padded_width) + "\n");
  s.append( "  File height:  " + std::to_string(_actual.height) + "\n");
  s.append( "  Count planes (==1): " + std::to_string(_actual.count_planes) + "\n");
  s.append( "  Px bit depth: " + std::to_string(_actual.bit_depth) + "\n");
  s.append( "  Compr type: " + std::to_string(_actual.compression_type) + "\n");
  s.append( "  Size image: " + std::to_string(_actual.size_image) + "\n");
  s.append( "  Horiz PPI:  " + std::to_string(_actual.horizontal_ppi) + "\n");
  s.append( "  Vert PPI :  " + std::to_string(_actual.vertical_ppi) + "\n");
  s.append( "  Horiz PPMM: " + std::to_string(_actual.horizontal_ppmm) + "\n");
  s.append( "  Vert PPMM : " + std::to_string(_actual.vertical_ppmm) + "\n");
  s.append( "  Colors used : " + std::to_string(_actual.colors_used) + "\n");
  s.append( "  Colors important : " + std::to_string(_actual.colors_important) + "\n");
  return s;
}

/**
 * For future reference to dump the header as string of hex.
 *
 * @param step which step in the process: read, update, write.
 * @return the InfoHeader bytes as a concatenated string of hex
 */
std::string InfoHeader::to_s_hex() {
  std::string s{"InfoHeader bytes: "};
  char hex[3];
  s.append(": 0x");
  for( int i=0; i< BMP::NUM_BYTES_DIB_BITMAPINFOHEADER; i++ ) {
    // sprintf_s( hex, "%02X", dataBytes[i] );  // replace this as appropriate
    s.append( hex );
  }
  return s;
}

/**
 * Convert PPI values to PPMM and load into the Info header container.
 * Load the image size value into the Info header container.
 */
void InfoHeader::update()
{
  _actual.horizontal_ppi = _params->destImg.resolution.horiz;
  _actual.vertical_ppi   = _params->destImg.resolution.vert;

  NFIMM::convertPPItoPPMM( _params->destImg.resolution.horiz, _actual.horizontal_ppmm );
  NFIMM::expressUINT32AsFourBytes( _actual.horizontal_ppmm, _biXPelsPerMeter, false );
  NFIMM::convertPPItoPPMM( _params->destImg.resolution.vert, _actual.vertical_ppmm );
  NFIMM::expressUINT32AsFourBytes( _actual.vertical_ppmm, _biYPelsPerMeter, false );

  // In case where bitmap-size was == 0 in the source image, which is ok if
  // image not compressed, go ahead and update with _actual value.
  NFIMM::expressUINT32AsFourBytes( _actual.size_image, _biSizeImage, false );
}


}   // END namespace
