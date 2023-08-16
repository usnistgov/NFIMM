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

#include "png/png.h"

namespace NFIMM {

/**
 * Validates the PNG signature required for all valid PNG images. Update the
 * source image buffer read cursor to = 8.
 * @param mps metatdata container for logging
 * @param buf container for source image bytes
 * @throw Miscue If signature is invalid
 */
Signature::Signature( std::shared_ptr<MetadataParameters> &mps, std::vector<uint8_t> &buf )
{
  for( int i=0; i<NUM_BYTES_SIGNATURE; i++ ) {
    dataBytes.push_back( buf[i] );
  }

  // Validate.
  if( dataBytes != defined )
  {
    std::string msg{"ERROR: Signature validation FAILED: " + to_s()};
    throw Miscue( msg );
  }
  mps->loggit( "Signature validation OK! : " + to_s() );
  PNG::s_r_cursor = 8;
}

/**
 * @return the signature bytes as a concatenated string
 */
std::string Signature::to_s() {
  char hex[3];
  std::string s{"0x"};
  for( int i=0; i< NUM_BYTES_SIGNATURE; i++ ) {
    sprintf( hex, "%02X", dataBytes[i] );
    s.append( hex );
  }
  return s;
}

}   // END namespace
