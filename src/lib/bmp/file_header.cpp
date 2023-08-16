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
FileHeader::FileHeader( std::shared_ptr<MetadataParameters> &mps )
           : _params(mps) {}

/** Load each data element into single container. */
void FileHeader::headerAsVector() {

  for( int i=0; i<BMP::NUM_BYTES_BM_IDENTIFIER; i++ )
     { _vecEntireHeader.push_back(_bfType[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _bfSize[i] ); }
  for( int i=0; i<2; i++ ) { _vecEntireHeader.push_back( _bfReserved1[i] ); }
  for( int i=0; i<2; i++ ) { _vecEntireHeader.push_back( _bfReserved2[i] ); }
  for( int i=0; i<4; i++ ) { _vecEntireHeader.push_back( _bfOffBits[i] ); }
}

/** Specified to contain the following values:\n
 *   bytes 0 and 1: 'BM' for Windows and is most common, most likely supported\n
 *             by linux flavors.  Other valid identifiers:\n
 *            'BA', 'CI', 'CP', 'IC', and 'PT' however only 'BM' is supported.\n
 *   bytes 2-5:   size of file in bytes\n
 *   bytes 6-9:   reserved, usually all zeros\n
 *   bytes 10-13: start offset of image pixels data
 *
 * Reads the entire header and saves accordingly.
 *
 * If the BMP identifier in the first two-bytes is not equal to 'BM' return
 * immediately and forgo reading the rest of the header.
 */
void FileHeader::read()
{
  _params->loggit( "FileHeader _readBuffer size: " +
                    std::to_string( NFIMM::s_readBuffer.size() ) );
  NFIMM::nextLengthBytes( BMP::NUM_BYTES_BM_IDENTIFIER, _bfType );
  // Validate BMP identifier.
  for( int i=0; i<BMP::NUM_BYTES_BM_IDENTIFIER; i++ ) {
    if( _bfType[i] == _fileType[i] )
      continue;
    else {
      std::string err{"ERROR: First 2-bytes of file header not 'BM'"};
      _params->loggit( err );
      throw Miscue( err );
    }
  }

  // Read file size
  NFIMM::next4bytes( _bfSize );
  NFIMM::expressFourBytesAsUINT32( _actual.fileSize, _bfSize, false );
  // Read reserved
  NFIMM::nextLengthBytes( 2, _bfReserved1 );
  NFIMM::nextLengthBytes( 2, _bfReserved2 );
  // Read pixel offset
  NFIMM::next4bytes( _bfOffBits );
  NFIMM::expressFourBytesAsUINT32( _actual.offsetToPixelData, _bfOffBits, false );

  _actual.calculated_size_image = _actual.fileSize - _actual.offsetToPixelData;
}

/**
 * @param step which step in the process: read, update, write.
 * @return the FileHeader metadata as a concatenated string
 */
std::string FileHeader::to_s( const std::string &step ) {
  std::string s{};
  s.append( step );
  s.append( " FILEHEADER actuals:\n" );
  s.append( "  File size: " + std::to_string(_actual.fileSize) + "\n");
  s.append( "  Offset to pixel data: "
            + std::to_string(_actual.offsetToPixelData) + "\n");
  s.append( "  Calculated size image (Filesize minus OffsetToPixelData): "
            + std::to_string(_actual.calculated_size_image) + "\n");
  return s;
}

/**
 * For future reference to dump the header as string of hex.
 *
 * @param step which step in the process: read, update, write.
 * @return the InfoHeader bytes as a concatenated string of hex
 */
std::string FileHeader::to_s_hex() {
  std::string s{"FileHeader bytes: "};
  char hex[3];
  s.append(": 0x");
  for( int i=0; i< BMP::NUM_BYTES_BITMAPFILEHEADER; i++ ) {
    // sprintf_s( hex, "%02X", dataBytes[i] );  // replace this as appropriate
    s.append( hex );
  }
  return s;
}

}   // END namespace
