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

#include <algorithm>
#include <iostream>


namespace NFIMM {

/**
 * Check compression is either bmp or png.
 * @throw Miscue Non-supported compression-type
*/
MetadataParameters::MetadataParameters( const std::string &imgFormat )
                    : compression(imgFormat) {

  // Convert compression to lower-case for comparison to ensure is supported
  std::transform( compression.begin(),
                  compression.end(),
                  compression.begin(),
                  static_cast<int(*)(int)>(std::tolower) );

  if( (compression != "bmp") && (compression != "png") )
    throw Miscue(
        "Non-supported image compression: '" + compression + "'" );
  else
  {
    srcImg.compression  = compression;
    destImg.compression = compression;
  }
}

/**
 * @param s message to log
 */
void MetadataParameters::loggit( const std::string &s ) {
  log.push_back( s );
}

/**
 * @param img either "src" or "dest"
 * @return "PPI" or "PPMM" or ""
 */
std::string MetadataParameters::get_imgSampleRateUnits( const std::string &img ) {
  if( img == "dest" ) {
    if( destImg.resolution.unitsStr == "inch" )
      return "PPI";
    else if( destImg.resolution.unitsStr == "meter" )
      return "PPMM";
    else
      return "";
  }
  else if( img == "src" ) {
    if( srcImg.resolution.unitsStr == "inch" )
      return "PPI";
    else if( srcImg.resolution.unitsStr == "meter" )
      return "PPMM";
    else
      return "";
  }
  return "";
}

/**
 * @param rate [ "inch" | "meter" | "other" ]
 */
void MetadataParameters::set_destImgSampleRateUnits( const std::string &rate ) {
  destImg.resolution.unitsStr = rate;
  if( rate == "meter" )
    destImg.resolution.units = 1;
  else
    destImg.resolution.units = 0;
}

/**
 * @param rate [ "inch" | "meter" | "other" ]
 */
void MetadataParameters::set_srcImgSampleRateUnits( const std::string &rate ) {
  srcImg.resolution.unitsStr = rate;
  if( rate == "meter" )
    srcImg.resolution.units = 1;
  else
    srcImg.resolution.units = 0;
}


/** @return all required and optional metadata parameters */
std::string MetadataParameters::to_s() {
  std::string s{"Modification Metadata:\n"};
  s.append( " * Source compression format: " );
  s.append( srcImg.compression + "\n" );
  s.append( " * Source image sample-rate " );
  s.append( "(" + srcImg.resolution.unitsStr + ")\n" );
  s.append( "   Horiz: " );
  s.append( std::to_string( srcImg.resolution.horiz ) + "\n" );
  s.append( "   Vert:  " );
  s.append( std::to_string( srcImg.resolution.vert ) + "\n" );
  s.append( " * Destination compression format: " );
  s.append( destImg.compression + "\n" );
  s.append( " * Destination image sample-rate " );
  s.append( "(" + destImg.resolution.unitsStr + ")\n" );
  s.append( "   Horiz: " );
  s.append( std::to_string( destImg.resolution.horiz ) + "\n" );
  s.append( "   Vert:  " );
  s.append( std::to_string( destImg.resolution.vert ) + "\n" );

  if( compression == "png" )
  {
    s.append( " * Destination custom text:\n" );
    for( auto &txt : destImg.textChunk )
    {
      s.append( "   " + txt + "\n" );
    }
  }
  return s;
}

}   // END namespace