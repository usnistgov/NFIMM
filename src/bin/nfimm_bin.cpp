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
#include <iostream>

#include "CLI11.hpp"
#include "nfimm.h"
#include "nfimm_bin.h"


void procArgs( CLI::App &, CmdLineOptions & );


int main(int argc, char** argv) 
{

  CLI::App app{"Modify image metadata only; image-data not modified."};

  // If cmd line has zero switches, force -h.
  if( argc==1 )
  {
    char hlp[8] = "--help\0";
    argv[argc++] = hlp;
    // Line below prevents CLI11 error: 'The following argument was not expected:'
    // when binary is run with zero params, ie, argc=1.
    std::cout << std::endl;
  }

  procArgs( app, opts );

  // This is what the macro CLI11_PARSE expands to:
  // CLI11_PARSE(app, argc, argv);
  try
  {
    app.parse( argc, argv );
  }
  catch( const CLI::ParseError &e )   // also catches help switch
  {
    app.exit(e);    // prints help menu to console
    return -1;
  }

  if( opts.prVer ) {
    std::cout << "*** Call NFIMM::printVersion() ***" << std::endl;
    std::cout << NFIMM::printVersion() << std::endl;
    return(0);
  }

  if( opts.flagVerbose )
    opts.printOptions();  


  // Declare the pointers to metadata params and metadata modifier objects.
  // NFIMM is the base class for supported compression type's derived class.
  std::shared_ptr<NFIMM::MetadataParameters> mp;
  std::unique_ptr<NFIMM::NFIMM> nfimm_mp;
  try
  {
    // Create the metadata
    mp.reset( new NFIMM::MetadataParameters( opts.imageFormat ) );
    mp->srcImg.resolution.horiz = opts.srcSampleRate;
    mp->srcImg.resolution.vert = opts.srcSampleRate;
    mp->set_srcImgSampleRateUnits( opts.sampleRateUnits );
    mp->destImg.resolution.horiz = opts.tgtSampleRate;
    mp->destImg.resolution.vert = opts.tgtSampleRate;
    mp->set_destImgSampleRateUnits( opts.sampleRateUnits );
  }
  catch( const NFIMM::Miscue &e )
  {
    std::cout << "NFIMM user caught exception: " << e.what() << std::endl;
    exit(0);
  }

  try
  {
    if( opts.imageFormat == "bmp" )
    {
      nfimm_mp.reset( new NFIMM::BMP( mp ) );
    }
    else  // must be png
    {
      if( opts.vecPngTextChunk[0] == "" )
      {
        std::cout << "\nImage format is PNG and png-text-chunk cannot be empty!"
                  << std::endl;
        exit(0);
      }
      mp->destImg.textChunk = opts.vecPngTextChunk;
      nfimm_mp.reset( new NFIMM::PNG( mp ) );
    }

    std::vector<uint8_t> vecSourceImage, vecDestImage;

    {
      std::fstream strm;
      strm.open( opts.srcImgPath, std::ios::in|std::ios::binary );

      vecSourceImage.clear();
      std::vector<uint8_t> tmp(
        (std::istreambuf_iterator<char>(strm)), std::istreambuf_iterator<char>() );
      strm.close();
      vecSourceImage = tmp;
    }

    nfimm_mp->readImageFileIntoBuffer( vecSourceImage );
    nfimm_mp->modify();
    nfimm_mp->writeImageBufferToFile( opts.tgtImgPath );

    if( opts.flagVerbose )
    {
      std::cout << "START RUNTIME Metadata LOG:" << std::endl;
      for( std::string s : mp->log ) { std::cout << s << std::endl; }
      std::cout << "START USER-SPECIFIED Metadata Paramaters:" << std::endl;
      std::cout << mp->to_s() << std::endl;
      std::cout << "GENERATED IMAGE: " << opts.tgtImgPath << std::endl;
    }
  }
  catch( const NFIMM::Miscue &e )
  {
    std::cout << "NFIMM user caught exception: " << e.what() << std::endl;
    exit(0);
  }

}

/** @brief Process the command-line options
 *
 * @param app CLI-application object reference
 * @param opts command-line options object reference
 */
void
procArgs( CLI::App &app, CmdLineOptions &opts )
{

  app.add_option( "-a, --src-samp-rate", opts.srcSampleRate, "Source imagery sample rate" );
  app.add_option( "-b, --tgt-samp-rate", opts.tgtSampleRate, "Target imagery sample rate" );

  app.add_option( "-c, --samp-rate-units", opts.sampleRateUnits, "[ inch | meter | other ]" );

  app.add_option( "-e, --png-text-chunk", opts.vecPngTextChunk, "list of 'tEXt' chunks in format 'keyword:text'" );

  app.add_option( "-m, --img-fmt", opts.imageFormat, "Image compression format [ bmp | png ], default is 'png'" );

  app.add_option( "-s, --src-img-path", opts.srcImgPath, "Source image PATH (absolute or relative)" )
    ->check(CLI::ExistingFile);
  app.add_option( "-t, --tgt-img-path", opts.tgtImgPath, "Target image PATH (absolute or relative)" );

  app.add_flag( "-v,--version", opts.prVer, "Print versions and exit" )
    ->multi_option_policy()
    ->ignore_case();

  app.add_flag( "-z,--verbose", opts.flagVerbose, "Print target file PATH" )
    ->multi_option_policy()
    ->ignore_case();

  app.get_formatter()->column_width(20);
}
