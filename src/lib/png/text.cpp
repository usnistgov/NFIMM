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

#include <iomanip>
#include <sstream>
#include <sys/stat.h>


namespace NFIMM {


/**
 * @param mps needed to update the runtime log
 */
Text::Text( std::shared_ptr<MetadataParameters> &mps ) : _params(mps)
{
  std::ostringstream ss;
  ss << std::boolalpha << PNG::s_pHYsChunkExists;
  _params->loggit( "Text ctor Existing 'pHYs': " + ss.str() );
}

/**
 * All generated chunk's pointers are inserted into the array of insertion-
 * chunk pointers.
 *
 * A Comment is added to reflect the source image 'pHYs' chunk whether
 * it exists or not. If it does exist, include source image resolution in a new
 * Comment; if not exist, insert metadata dest resolution in a new Comment.
 *
 * A Software keyword:value pair is always inserted that contains the version
 * of this NFIMM library.
 *
 * @throw Miscue Cannot allocate chunk shared pointer, or chunk's dataBuffer,
 *  rethrow exception for UTCtime error
 */
void Text::insertChunks()
{
  std::shared_ptr<PNG::ChunkLayout> tchunk;

  std::ostringstream ss;
  ss << std::boolalpha << PNG::s_pHYsChunkExists;
  _params->loggit( "Source image contains 'pHYs' chunk: " + ss.str() );

  if( PNG::s_pHYsChunkExists )
  {
    std::string s1{"Comment:NFIMM updated pHYs resolution from "};
    s1.append( std::to_string( _params->srcImg.existingPhysResolution ) );
    s1.append( "PPMM to " );
    s1.append( std::to_string( _params->destImg.resolution.horiz ) );
    s1.append( _params->get_imgSampleRateUnits("dest") );
    _params->destImg.textChunk.push_back( s1 );
  }
  else
  {
    std::string s1{"Comment:NFIMM inserted pHYs resolution as "};
    s1.append( std::to_string( _params->destImg.resolution.horiz ) );
    s1.append( _params->get_imgSampleRateUnits("dest") );
    _params->destImg.textChunk.push_back( s1 );
  }

  std::string sv = "Software:header mod by " + printVersion();
  _params->destImg.textChunk.push_back( sv );

  // Continue with the rest of the tEXt chunk metadata...
  std::vector<std::string> tokens{};
  std::string token{};

  // Iterate through the vector that contains the list of tEXt chunk strings.
  // The format of the chunk's string is: 'keyword:text'.
  for( auto &txt : _params->destImg.textChunk ) {
    // The tEXt strings are delimited by ':'.
    std::istringstream tokenStream(txt);
    tokens.clear();
    while( std::getline(tokenStream, token, ':') ) {
      tokens.push_back(token);
    }
    for( auto &tval : tokens ) {
      _params->loggit( "TOKEN=> " + tval );
    }

    // Verify keyword against list of valid keywords.
    // Non-valid keywords are ignored; user must determine correctness of
    // destination image metadata (update) by inspection.
    for( auto &keywd : _textKeywords ) {
      if( tokens[0] == keywd ) {
        _params->loggit( "KEYPAIR=> " + tokens[0] + ":" + tokens[1] );
        _params->loggit( "valid keywd: " + tokens[0] + ", size: " +
                          std::to_string( tokens[0].size() ) );
        _params->loggit( "keywd text : " + tokens[1] + ", size: " +
                          std::to_string( tokens[1].size() ) );

        try
        {
          tchunk.reset( new PNG::ChunkLayout );
        }
        catch( const std::bad_weak_ptr bwPtr )
        {
           throw Miscue( "tEXt chunk-layout bad_weak_ptr" );
        }

        tchunk->typeBytes[0] = 't';
        tchunk->typeBytes[1] = 'E';
        tchunk->typeBytes[2] = 'X';
        tchunk->typeBytes[3] = 't';
        _params->loggit( "Load TYPE: '" + tchunk->type() + "'" );

        // Support variables.
        // dataBuffer array index.
        int db_idx{0};
        size_t dataBufSize{0};

        // Calculate the data-part buffer size based on the keyword.
        if( tokens[0] == "Creation Time") {
          dataBufSize = tokens[0].size() + NUM_BYTES_UTC + 1;
        }
        else
          dataBufSize = tokens[0].size() + tokens[1].size() + 1;

        // Update the chunk's data length.
        NFIMM::expressUINT32AsFourBytes( dataBufSize, tchunk->lengthBytes,
                                         true );
        _params->loggit( "tEXT dataBufferSize: " +
                          std::to_string( tchunk->length() ) );

        // Build the chunk data part by pushing the keyword, null-separator,
        // and text.
        tchunk->dataBuffer = new (std::nothrow) uint8_t[dataBufSize];
        if( !tchunk->dataBuffer )
        {
          throw Miscue( "cannot allocate tEXt chunk data buffer: '" +
                         tokens[1] + "'" );
        }

        if( tokens[0] == "Creation Time") {
          for( size_t i=0; i<tokens[0].size(); i++ ) {   // push keyword
            tchunk->dataBuffer[i] = tokens[0][i]; db_idx++;
          }
          // Insert null separator.
          tchunk->dataBuffer[db_idx] = NULL_SEPARATOR; db_idx++;
          // Determine the source for the creation time: now, file, user.
          UTCtime utct;
          try
          {
            if( tokens[1] == "now" ) {
              getUTCtime( &utct ); }
            else if( tokens[1] == "file" ) {
              _params->
                loggit( "Src file for timestamp: " + _params->srcImg.path );
              getFiletime( _params->srcImg.path, &utct ); }
            else {
              std::string msg{"Invalid file creation-time parameter: " +
                               tokens[1]};
              _params->loggit( msg );
              throw Miscue( msg );
            }
          }
          catch( const Miscue &e )
          {
            delete [] tchunk->dataBuffer;
            throw e;
          }

          for( size_t i=0; i<2; i++ ) {   // push year
            tchunk->dataBuffer[db_idx] = utct.yearBytes[i]; db_idx++;
          }
          tchunk->dataBuffer[db_idx] = utct.mon; db_idx++;
          tchunk->dataBuffer[db_idx] = utct.day; db_idx++;
          tchunk->dataBuffer[db_idx] = utct.hr; db_idx++;
          tchunk->dataBuffer[db_idx] = utct.min; db_idx++;
          tchunk->dataBuffer[db_idx] = utct.sec; db_idx++;
        }
        else {
          for( size_t i=0; i<tokens[0].size(); i++ ) {   // push keyword
            tchunk->dataBuffer[i] = tokens[0][i]; db_idx++;
          }
          // Insert null separator.
          tchunk->dataBuffer[db_idx] = NULL_SEPARATOR; db_idx++;
          for( size_t i=0; i<tokens[1].size(); i++ ) {   // push text
            tchunk->dataBuffer[db_idx] = tokens[1][i]; db_idx++;
          }
        }
        _params->loggit( "tEXt dataBuffer: 0x" + tchunk->data() );

        // Concat the type- and data-parts of the chunk to calculate the CRC.
        uint8_t *cat = new uint8_t[PNG::NUM_BYTES_CHUNK_TYPE+dataBufSize];
        if( !cat )
        {
          delete [] tchunk->dataBuffer;
          throw Miscue( "cannot allocate tEXt chunk concatenation buffer" );
        }
        uint32_t ix{0};
        for( ix=0; ix<PNG::NUM_BYTES_CHUNK_TYPE; ix++ ) {
          cat[ix] = tchunk->typeBytes[ix];
        }
        for( ix=PNG::NUM_BYTES_CHUNK_TYPE;
             ix<PNG::NUM_BYTES_CHUNK_TYPE+dataBufSize;
             ix++ ) {
          cat[ix] = tchunk->dataBuffer[ix-PNG::NUM_BYTES_CHUNK_TYPE];
        }

        std::string s{};
        char hex[3];

        for( uint32_t j=0; j<(PNG::NUM_BYTES_CHUNK_TYPE+dataBufSize); j++ ) {
          sprintf( hex, "%02X", cat[j] );
          s.append( hex );
        }
        _params->loggit( "(concatenate) CRC tEXt dataBuffer: 0x" + s );

        // Calculate the CRC.
        uint32_t crc_calculated{0};
        crc_calculated = CRCforPNG::calc(
          cat,
          PNG::NUM_BYTES_CHUNK_TYPE+static_cast<int>(dataBufSize) );
        // Done with concatenation, so delete
        delete [] cat;

        // Update the chunk CRC-bytes.
        NFIMM::expressUINT32AsFourBytes( crc_calculated,
                                         tchunk->crcBytes,
                                         true );
        _params->loggit( "tEXt CRC calculated = 0x" + tchunk->crc() );

        // Concatenate the 4-parts into a single buffer
        tchunk->concatenate4parts();

        // Chunk is valid, append the object to container that is iterated
        // upon write to output buffer and update index.
        PNG::s_insertChunkPointers.push_back( tchunk );
        PNG::_insertChunkIndex++;

        // Increment the count
        _params->pngWriteImageInfo.countInsertChunks++;

        break;  // done with list of valid keywords, build next chunk.
      }
    }
  }

  /* A Comment is always added to reflect the source image 'pHYs' chunk
  whether it exists or not, and, a Software keyword:value pair is always
  added that contains the version of this NFIMM library.
  Because these two were pushed into the metadata params text-chunk vector
  to take advantage of the loop that generates the `tEXt` chunk, they must
  be popped off the vector. Before these 2 were popped, they were duplicated
  every time through the list of source images to process. For example, if
  the list of source images contained 5 images, the 5th destination image
  would have 4 "extra" each of the Software and Comment chunks. */
  // Just in case, only pop the vector safely.
  if( _params->destImg.textChunk.size() >= 2 )
  {
    _params->destImg.textChunk.pop_back();
    _params->destImg.textChunk.pop_back();
  }
  _params->loggit( "_params->destImg.textChunk.size(): " +
    std::to_string( _params->destImg.textChunk.size() ) );
}   // END insertChunks()

/**
 * Adjust year since 1900 and add 1 to the month for the actual month, e.g.,
 * if month is July, gmtime() returns 6 for the month, so add to make July
 * the 7th month.
 *
 * @param path to date/time object
 * @param utmp pointer to date/time object
 * @throw gmtime returned NULL
 */
void Text::getFiletime( const std::string &path, UTCtime *utmp )
{
  struct stat attrib;
  stat(path.c_str(), &attrib);
  tm *gtmp = gmtime(&(attrib.st_mtime));
  // gtmp = NULL;
  if( gtmp == NULL ) {
    throw Miscue( "Get FILE gmtime error: " + path );
  }
  utmp->setYear( gtmp->tm_year+1900 );
  utmp->mon = static_cast<uint8_t>(gtmp->tm_mon+1);
  utmp->day = static_cast<uint8_t>(gtmp->tm_mday);
  utmp->hr  = static_cast<uint8_t>(gtmp->tm_hour);
  utmp->min = static_cast<uint8_t>(gtmp->tm_min);
  utmp->sec = static_cast<uint8_t>(gtmp->tm_sec);

}

/**
 * Adjust year since 1900 and add 1 to the month for the actual month, e.g.,
 * if month is July, gmtime() returns 6 for the month, so add to make July
 * the 7th month.
 *
 * @param utmp pointer to date/time object
 * @throw gmtime returned NULL
 */
void Text::getUTCtime( Text::UTCtime *utmp )
{
  time_t t;
  t = time(NULL);
  tm *gtmp = gmtime(&t);
   if( gtmp == NULL ) {
    throw Miscue( "Get UTC gmtime error" );
  }
  utmp->setYear( gtmp->tm_year+1900 );
  utmp->mon = static_cast<uint8_t>(gtmp->tm_mon+1);
  utmp->day = static_cast<uint8_t>(gtmp->tm_mday);
  utmp->hr  = static_cast<uint8_t>(gtmp->tm_hour);
  utmp->min = static_cast<uint8_t>(gtmp->tm_min);
  utmp->sec = static_cast<uint8_t>(gtmp->tm_sec);
}

}   // END namespace
