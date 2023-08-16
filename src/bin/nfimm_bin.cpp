#include <iostream>
#include <string>

#include "nfimm.h"


int main(int argc, char** argv) 
{
  // Create the path to src image
  std::string pathToSrcImage, pathToDestImage;
  // std::cout << "pathToSrcImage: " << pathToSrcImage << std::endl;
  const std::string srcImgCompressionType{"png"};

  // Declare the pointers to metadata params and metadata modifier objects.
  // NFIMM is the base class for PNG and BMP derived classes.
  std::shared_ptr<NFIMM::MetadataParameters> mp;
  std::unique_ptr<NFIMM::NFIMM> nfimm_mp;
  // std::unique_ptr<NFIMM::NFIMM> nfimm_mp = std::make_unique<NFIMM::NFIMM>(mp);
  try
  {
    // START Create the metadata
    mp.reset( new NFIMM::MetadataParameters( srcImgCompressionType ) );
    mp->srcImg.resolution.horiz = 600;
    mp->srcImg.resolution.vert = 600;
    mp->set_srcImgSampleRateUnits( "inch" );
    mp->destImg.resolution.horiz = 500;
    mp->destImg.resolution.vert = 500;
    mp->set_destImgSampleRateUnits( "inch" );
    mp->destImg.textChunk = {};
    // END Create the metadata
  }
  catch( const NFIMM::Miscue &e )
  {
    std::cout << "NFIMM user caught exception: " << e.what() << std::endl;
    exit(0);
  }

  try
  {
    if( srcImgCompressionType == "bmp" )
    {
      std::vector<std::string> vecPngTextChunk{};
      mp->destImg.textChunk = vecPngTextChunk;
      pathToSrcImage = "your-input-image.bmp";
      pathToDestImage = "your-output-image.bmp";
      std::cout << "pathToSrcImage: " << pathToSrcImage << std::endl;
      nfimm_mp.reset( new NFIMM::BMP( mp ) );
    }
    else  // must be png
    {
      // Update the meta params with png tEXt chunk info
      std::vector<std::string> vecPngTextChunk
      { "Comment:for NFIMM pub release",
        // "Software:not used",
        "Author:NIST-ITL(test)",
        "Description:Original image downsampled from 600PPI",
        "Creation Time:file" };
      mp->destImg.textChunk = vecPngTextChunk;

      pathToSrcImage = "your-input-image.bmp";
      pathToDestImage = "your-output-image.bmp";
      std::cout << "pathToSrcImage: " << pathToSrcImage << std::endl;
      nfimm_mp.reset( new NFIMM::PNG( mp ) );
    }


    // Source and destination images API is either path-to-file or vector-of-bytes.
    // TO run this code, either API is commented in/out
    // ++++++++++ START API: path-to-file ++++++++++++++++++++++++++++++++++++++
    // std::cout << nfimm_mp->to_s() << std::endl;
    // nfimm_mp->readImageFileIntoBuffer( pathToSrcImage );
    // nfimm_mp->modify();
    // nfimm_mp->writeImageBufferToFile( pathToDestImage );
    // ++++++++++ END API: path-to-file ++++++++++++++++++++++++++++++++++++++++

    // ++++++++++ START API: vector-of-bytes +++++++++++++++++++++++++++++++++++
    // There are 2 "ways" to handle the destination image:
    //  1) write the buffer to file
    //  2) retrieve the destination image as vector of bytes
    // These 2 ways enable maximum flexibility for the NFIMM-user.
    // Seems reasonable that to test the vector-of-bytes API
    // readImageFileIntoBuffer() call it is sufficient to call the
    // nfimm_mp->writeImageBufferToFile( pathToDestImage ).
    //
    // To test the retrieveWriteImageBuffer( vecDestImage ) call, this code
    // block writes the file to disk.
    // std::cout << nfimm_mp->to_s() << std::endl;
    std::vector<uint8_t> vecSourceImage, vecDestImage;

    {
      std::fstream strm;
      strm.open( pathToSrcImage, std::ios::in|std::ios::binary );

      vecSourceImage.clear();
      std::vector<uint8_t> tmp(
        (std::istreambuf_iterator<char>(strm)), std::istreambuf_iterator<char>() );
      strm.close();
      vecSourceImage = tmp;
    }

    // std::cout << "vecSourceImage.size(): " << vecSourceImage.size() << std::endl;
    nfimm_mp->readImageFileIntoBuffer( vecSourceImage );
    nfimm_mp->modify();
    nfimm_mp->writeImageBufferToFile( pathToDestImage );

    // vecDestImage.clear();
    // nfimm_mp->retrieveWriteImageBuffer( vecDestImage );
    // std::cout << "vecDestImage.size(): " << vecDestImage.size() << std::endl;
    // {
    //   std::fstream strm;
    //   strm.open( pathToDestImage, std::ios::out|std::ios::binary );

    //   for (const auto &e : vecDestImage) strm << e;
    //   strm.close();
    // }
    // ++++++++++ END API: vector-of-bytes +++++++++++++++++++++++++++++++++++++

    std::cout << "START USER-SPECIFIED Metadata Paramaters:" << std::endl;
    std::cout << mp->to_s() << std::endl;
    std::cout << "START RUNTIME Metadata LOG:" << std::endl;
    for( std::string s : mp->log ) { std::cout << s << std::endl; }
    std::cout << "pathToDestImage: " << pathToDestImage << std::endl;
  }
  catch( const NFIMM::Miscue &e )
  {
    std::cout << "NFIMM user caught exception: " << e.what() << std::endl;
    exit(0);
  }

}
