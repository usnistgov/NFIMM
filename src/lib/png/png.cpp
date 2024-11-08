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

#include <iostream>
#include <map>
#include <sstream>


namespace NFIMM {

size_t PNG::_insertChunkIndex;
std::vector<std::shared_ptr<PNG::ChunkLayout>> PNG::s_insertChunkPointers;

/** Clear the chunk containers, clear write-buffer. */
PNG::PNG( std::shared_ptr<MetadataParameters> &mps ) : NFIMM{mps}
{
  _params->loggit( "Initialize for PNG modification" );
  s_pHYsChunkExists = false;
  s_writeBuffer.clear();
  s_insertChunkPointers.clear();
  _srcChunkPointers.clear();
}

/**
 * Run the entire process to parse, update, and insert chunks. To reassemble
 * the destination image, transfer bytes to write-buffer.
 *
 * @throw Miscue for any point where process failed
 */
void PNG::modify()
{
  try
  {
    PNG::_insertChunkIndex = 0;  // required for linux
    Signature sig( _params, s_readBuffer );
    // s_r_cursor = 8;
    _params->loggit( ">> Parse all chunks in source");
    parseAllChunks();
    _params->loggit( ">> Process source chunks");
    processExistingChunks();
    insertChunkPhys();
    _params->loggit( ">> Insert custom text");
    insertCustomText();
    _params->loggit( "Chunk INSERT total COUNT: " + std::to_string( _insertChunkIndex ) );
    _params->loggit( ">> Xfer chunks to write buffer");
    xferChunks();
  }
  catch( const Miscue &e ) {
    throw Miscue( e );
  }

}

/**
 * Read all chunks from source-image AFTER the IHDR chunk.  With each chunk,
 * save the pointer to the ChunkLayout object in an array. This array is used
 * when processing chunks and writing destination image.
 * 
 * For each CHUNK:
 * 
 *   1. read 4-bytes : LEN
 *   2. read 4-bytes : type-name
 *   3. read LEN-bytes : chunk data
 *   4. read 4-bytes : CRC
 *   5. call concatenate4parts() to concatenate the 4-parts of the chunk
 *      into a single buffer.
 * 
 * Concatenation is done for convenience:
 * 
 * - IDAT chunks are never modified; the IDAT chunks for the source image are
 *   always the same as the destination chunks
 * - Other chunks may be modified/updated with "new" information:
 * 
 *   - pHYs: resolution
 *   - IHDR: image size
 * 
 * Therefore, in the event that no modification is specified, the whole chunk
 * is ready for write without having to rebuild from each of the parts.
 * 
 * After chunk-read complete, the TYPE is validated against a list of all
 * Critical and Ancillary TYPEs.  If valid, update the "chunk dictionary" where
 * `key` == TYPE and `val` == COUNT.
 * 
 * Count for IDAT chunks is tracked and saved for use in writing destination
 * image.
 * 
 * Check TYPE for IEND to indicate no-more chunks; Chunk IEND is NOT included
 * in the total chunk-count.
 * 
 * ```
 *   if( chunk-type == 'IEND' )
 *     break;
 *   else
 *     next-chunk;
 * ```
 * 
 * After all chunks read, they are ready to be processed.
 * 
 * Updates the COUNT of chunks in the MetadataParameters object used to support
 * WRITE operations.
 * 
 * @param offset starting point for first chunk after Signature in image buffer
 */
void PNG::parseAllChunks( int offset )
{
  /* Keep track of all found chunks and their counts */
  std::map<std::string, uint32_t> chunkDictionary;
  /* Iterator for the chunks-dictionary */
  std::map<std::string, uint32_t>::iterator itr;

  while( true )
  {
    std::shared_ptr<ChunkLayout> currentChunk( new ChunkLayout );

    // Parse the LEN
    next4bytes( currentChunk->lengthBytes );

    // Parse the TYPE
    next4bytes( currentChunk->typeBytes );
    {
      // log all except IDAT
      if( currentChunk->type() != "IDAT" )
      _params->loggit( "*** currentChunk: " +
                        currentChunk->type() + "  len: " +
                        std::to_string( currentChunk->length() ) );
    }

    // Parse the Chunk's DATA
    currentChunk->dataBuffer = new uint8_t[currentChunk->length()];
    nextLengthBytes( currentChunk->length(), currentChunk->dataBuffer );
    // {
    //   // Dump all bytes of the image data to the log; useful for extreme debug.
    //   _params->loggit( "*** currentChunk->dataBuffer(): 0x" + currentChunk->data() );
    // }

    // Parse the Chunk CRC
    next4bytes( currentChunk->crcBytes );
    {
      // Dump all bytes to log; useful for extreme debug.
      // _params->loggit( "*** currentChunk->crc(): 0x" + currentChunk->crc() );
    }

    // Concatenate the 4-parts into a single buffer
    currentChunk->concatenate4parts();

    // Save the chunk pointer
    _srcChunkPointers.push_back( currentChunk );
    // {
    //   // Dump chunk's memory address to log; useful for extreme debug.
    //   std::string loggerStr{};
    //   auto strAddr = "Chunk address: currentChunk: 0x%p";
    //   logAddress( strAddr, currentChunk, loggerStr );
    //   _params->logggit( loggerStr );
    // }
    _countChunk++;

    // Update the map of chunks; first, if key does not exist, insert with
    //   COUNT == 1.
    // If key does exist, increment the count.  This accounts for those chunks
    //   where multiple are allowed:
    //     IDAT, sPLT, iTXt, tEXt, zTXt
    itr = chunkDictionary.find( currentChunk->type() );
    if( itr != chunkDictionary.end() )
      itr->second += 1;
    else
      chunkDictionary.insert( std::pair<std::string,
                              uint32_t>( currentChunk->type(), 1 ) );

    if( currentChunk->type() == "IEND" ) {
      break;   // exit while(true) because reached End of File chunk
    }
    else if( currentChunk->type() == "pHYs" ) {
      s_pHYsChunkExists = true;
    }
  }  // END while(true)

  _params->loggit( "Source image chunk summary, total COUNT = " +
                    std::to_string( _countChunk ) );
  for( itr = chunkDictionary.begin(); itr != chunkDictionary.end(); ++itr) {
    _params->loggit( "Source image chunk type => " + itr->first +
                     "  COUNT =>" + std::to_string( itr-> second ) );
  }

  // Update output for write of dest image.
  _params->pngWriteImageInfo.countSourceChunks = _countChunk;
}

// -----------------------------------------------------------------------------
// Functions to check PNG image chunks after parsing of source image buffer.

/** Insert chunk if it does not exist:
 *
 * - `pHYs`: image resolution and units must always be inserted
 *
 * Note that this function is called AFTER the source image has been parsed
 * into chunks and references to those chunks have been saved to a container.
 *
 * Said array is iterated and checked for chunk-types that must be inserted
 * if they do not exist.  If a required chunk exists, then its content/data
 * has already been updated and is ready for output to the dest image, that is,
 * the destImg buffer is loaded.
 */
void PNG::insertChunkPhys()
{
  // Insert chunk `pHYs` if it does not exist.
  if( !s_pHYsChunkExists ) {
    _params->loggit( "pHYs does not exist, insert it" );
    // Phys ph( _params, 0 );
    Phys ph( _params, _srcChunkPointers[0] );
    ph.insertChunk();
  }
  else {
    _params->loggit( "pHYs does exist, already been updated" );
  }
}

/** For chunks `IHDR` and `pHYs`.
 *
 * For each chunk, its type (by name) is checked for inclusion in the list of
 * all available PNG-spec chunk types; these specified chunks include both the
 * Critical and Ancillary chunks.
 *
 * For `pHYs` chunk, modification is done, if required, by examination of the
 * metadata parameters.
 */
void PNG::processExistingChunks()
{
  for( uint32_t i=0; i<_countChunk; i++ ) {

    bool foundValidChunk{false};
    for( auto chunkType : _allChunkTypes ) {
      if( _srcChunkPointers[i]->type() == chunkType ) {
        foundValidChunk = true;
        break;
      }
    }

    // Throw error if not a valid chunk (which is not likely but possible)
    if( !foundValidChunk ) {
      std::string msg{"IDENTIFIED INvalid chunk: '" +
                       _srcChunkPointers[i]->type() + "'"};
      _params->loggit( msg );
      throw Miscue( msg );
    }

    if( _srcChunkPointers[i]->type() == "IHDR" ) {
      _params->loggit( "Chunk xfer without modification: IHDR" );

      // Constructor parses the chunk data and updates the write-data-buffer.
      IhdrX ih( _params, _srcChunkPointers[i] );
    }
    else if( _srcChunkPointers[i]->type() == "pHYs" ) {
      _params->loggit( "Chunk eligible for modification: pHYs" );

      // Constructor parses the chunk data and updates the write-data-buffer.
      Phys ph( _params, _srcChunkPointers[i] );
      ph.parseChunk();
      ph.updateChunk();
    }
  }
}

// -----------------------------------------------------------------------------
// Functions to build chunks for insertion into destination image write-buffer.

/**
 * Called by the modify() function, instantiates Text object where the
 * Text constructor performs all required functionality.
 */
void PNG::insertCustomText()
{
  // Constructor parses the chunk metadata and builds the chunks for
  // insertion into write-data-buffer.
  Text tx( _params );
  tx.insertChunks();
}

/** @return current metadata parameters */
std::string PNG::to_s()
{
  std::string s{"PNG: "};
  s.append( _params->to_s() );
  return s;
}


/**
 * Calculate the write-buffer size.  This is the same size as the source image
 * if there is no insertion of one or more `tEXt` chunks.  Write-buffer size
 * is calculated by iterating through the array of pointers to the chunks and
 * adding the chunk (data length + LEN + TYPE + CRC) where:
 * - LEN + TYPE + CRC = 12
 * 
 * Lengths of chunks for insertion are added to write-buffer size by iterating
 * through the insertion pointers array.
 * 
 * Save the write-buffer size to the metadata parameters in order to save the
 * buffer to disk later.
 * 
 * Write the signature to the write-buffer.
 * 
 * Then, iterate through the source chunks (array of pointers) and check for
 * chunk type `IDAT`.  `tEXt` chunks are inserted prior to the first
 * occurrence of `IDAT`.  A flag us used to insert ONLY prior to the first
 * `IDAT` and not all of them (since it is permissible for multiple `IDAT`s).
 * 
 * Transfer each chunk one at a tiime to the write buffer from either:
 * - _srcChunkPointers
 * - _insertChunkPointers
 */
void PNG::xferChunks()
{
  uint32_t totalChunks = _params->pngWriteImageInfo.sumChunks();
  _params->
    loggit( "WRITE all chunks, COUNT: " + std::to_string( totalChunks ) );
  _params->
    loggit( "WRITE sourced chunks, COUNT: " +
             std::to_string( _params->pngWriteImageInfo.countSourceChunks ) );
  _params->
    loggit( "WRITE inserted chunks, COUNT: " +
             std::to_string( _params->pngWriteImageInfo.countInsertChunks ) );

  // Update the writeBufferSize based on the lengths of the source image chunks
  //   AND the insertion-chunks .
  uint32_t writeBufferSize{8};   // signature = 8 bytes
  for( uint32_t p=0; p<_countChunk; p++ ) {
    writeBufferSize += _srcChunkPointers[p]->length();
    writeBufferSize += 12;  // LEN + TYPE + CRC
  }
  // _params->loggit( "writeBufferSize source: " +
  //                   std::to_string( writeBufferSize ) );
  for( size_t p=0; p<_insertChunkIndex; p++ ) {
    writeBufferSize += s_insertChunkPointers[p]->length();
    writeBufferSize += 12;  // LEN + TYPE + CRC
  }

  // SIGNATURE
  _params->loggit( "Length of Signature should == 8: " +
                    std::to_string( Signature::s_definedHex.size() ) );
  xferBytesBetweenBuffers( s_writeBuffer, Signature::s_definedHex );

  // Append IHDR - note that IHDR is always the first chunk after the signature
  // per the PNG spec and is passed to the destination image header unchanged.
  // Therefore IDHR is first in the container of src image chunks.
  _params->loggit( "IHDR whole chunk (sourced): " +
                    _srcChunkPointers[0]->wholeChunkStr() );
  xferBytesBetweenBuffers( s_writeBuffer, _srcChunkPointers[0]->wholeChunk() );
  delete [] _srcChunkPointers[0]->wholeChunkBuffer;
  delete [] _srcChunkPointers[0]->dataBuffer;

  // Append pHYs - since this chunk contains the image resolution, this chunk
  // has either been modified from the source or inserted if it did not exist
  // in the source.  Therefore, by design, if the pHYs chunk exists in source,
  // it is updated and the appended to the _writeBuffer.
  // If it does not exist in source, then by design this chunk is first in the
  // container that contains all chunks to insert.
  // Iterate the source chunk container and write to buffer
  for( std::shared_ptr<PNG::ChunkLayout> chnk : _srcChunkPointers )
  {
    if( chnk->type() == "pHYs" )
    {
      s_pHYsChunkExists = true;
      _params->loggit( "pHYs whole chunk (updated): " + chnk->wholeChunkStr() );
      xferBytesBetweenBuffers( s_writeBuffer, chnk->wholeChunk() );
      delete [] chnk->wholeChunkBuffer;
      delete [] chnk->dataBuffer;
      break;
    }
  }
  if( !s_pHYsChunkExists )
  {
    for( std::shared_ptr<PNG::ChunkLayout> chnk : s_insertChunkPointers )
    {
      if( chnk->type() == "pHYs" )
      {
        _params->loggit( "pHYs whole chunk (inserted): " + chnk->wholeChunkStr() );
        xferBytesBetweenBuffers( s_writeBuffer, chnk->wholeChunk() );
        delete [] chnk->wholeChunkBuffer;
        delete [] chnk->dataBuffer;
      }
    }
  }

  // Chunk ordering per the PNG spec calls-out no order-constraint per tEXt.
  // Therefore, all tEXt chunks in the insert-chunk container shall be written
  // to the _writeBuffer just ahead of the IDAT chunks.  (Any tEXt chunks in
  // the source image header container will be written in the order they
  // originally appeared).

  // In the use-case where zero custom tEXt were specified, thie insert
  // container shall contain three chunks: 1-pHYs chunk and 2-tEXt chunks.

  // Write all chunks from the source chunks container to the _writeBuffer
  // except:
  //  * IHDR - already been written to the _writeBuffer
  //  * pHYs - if it exists, the insert-container contains the chunk and has
  //           already been written to the _writeBuffer
  //  * IDAT - these go last but before IEND
  //  * IEND - must go last

  // Iterate the source chunk container and write to buffer
  for( std::shared_ptr<PNG::ChunkLayout> chnk : _srcChunkPointers )
  {
    if( chnk->type() == "IHDR" ) continue;
    if( chnk->type() == "pHYs" ) continue;
    if( chnk->type() == "IDAT" ) continue;
    if( chnk->type() == "IEND" ) continue;

    _params->loggit( "_writeBuffer sourced header chunk: " + chnk->type() );
    _params->loggit( "whole chunk (inserted): " + chnk->wholeChunkStr() );
    xferBytesBetweenBuffers( s_writeBuffer, chnk->wholeChunk() );
    delete [] chnk->wholeChunkBuffer;
    delete [] chnk->dataBuffer;
  }

  // Iterate the insert chunk container and write to buffer
  for( std::shared_ptr<PNG::ChunkLayout> chnk : s_insertChunkPointers )
  {
    // pHYs has already been xferred above
    if( chnk->type() == "pHYs" ) continue;

    _params->loggit( "_writeBuffer header chunk: " + chnk->type() );
    _params->loggit( "whole chunk (inserted): " + chnk->wholeChunkStr() );
    xferBytesBetweenBuffers( s_writeBuffer, chnk->wholeChunk() );
    delete [] chnk->wholeChunkBuffer;
    delete [] chnk->dataBuffer;
  }

  // Iterate the source chunk container and write to buffer
  for( std::shared_ptr<PNG::ChunkLayout> chnk : _srcChunkPointers )
  {
    if( chnk->type() == "IDAT" )
    {
      xferBytesBetweenBuffers( s_writeBuffer, chnk->wholeChunk() );
      delete [] chnk->wholeChunkBuffer;
      delete [] chnk->dataBuffer;
    }
    if( chnk->type() == "IEND" )
    {
      xferBytesBetweenBuffers( s_writeBuffer, chnk->wholeChunk() );
      delete [] chnk->wholeChunkBuffer;
      delete [] chnk->dataBuffer;
    }
  }
}

/**
 * Transfer bytes from one buffer to another. Maintain the cursor of the
 * receiving buffer.
 * 
 * Assumptions:
 * - from-cursor always starts at zero (the first btye)
 * - entire from[] buffer transferred to to[] buffer
 * 
 * @param to buffer to receive bytes
 * @param from buffer to take bytes
 */
void
PNG::xferBytesBetweenBuffers( std::vector<uint8_t>&to,
                              const std::vector<uint8_t>&from )
{
  for( uint8_t byte : from )
  {
    to.push_back( byte );
  }
  return;
}

/******************************************************************************/
/* struct ChunkLayout methods implementations */

/**
 * It is well known that the total byte-count for the whole chunk
 * is length of data plus 12:  4-len, 4-type, 4-CRC.
 */
void
PNG::ChunkLayout::concatenate4parts() {
  uint32_t total{length()+12};
  wholeChunkBuffer = new uint8_t[total];
  uint32_t i{0}, j{0};
  for( j=0; j<NUM_BYTES_CHUNK_LENGTH; j++ ) {
    wholeChunkBuffer[i] = lengthBytes[j]; i++;
  }
  for( j=0; j<NUM_BYTES_CHUNK_TYPE; j++ ) {
    wholeChunkBuffer[i] = typeBytes[j]; i++;
  }
  for( j=0; j<length(); j++ ) {
    wholeChunkBuffer[i] = dataBuffer[j]; i++;
  }
  for( j=0; j<NUM_BYTES_CHUNK_CRC; j++ ) {
    wholeChunkBuffer[i] = crcBytes[j]; i++;
  }
}

/** @return the actual string */
std::string
PNG::ChunkLayout::crc() {
  char hex[3];
  std::string s{};
  for( int j=0; j<NUM_BYTES_CHUNK_CRC; j++ ) {
    sprintf( hex, "%02X", crcBytes[j] );
    s.append( hex );
  }
  return s;
}

/** @return the actual string */
std::string
PNG::ChunkLayout::data() {
  std::string s{};
  char hex[3];

  for( uint32_t j=0; j<length(); j++ ) {
    sprintf( hex, "%02X", dataBuffer[j] );
    s.append( hex );
  }
  return s;
}

/** @return the decimal length */
uint32_t
PNG::ChunkLayout::length() {
  uint8_t oneByte{0};
  uint32_t val{0};
  for( int j=0; j<NUM_BYTES_CHUNK_LENGTH; j++ ) {
    oneByte = lengthBytes[j];
    val <<= 8;
    val += oneByte;
  }
  return val;
}

/** @return the actual string */
std::string
PNG::ChunkLayout::type() {
  std::stringstream ss;
  for( int j=0; j<NUM_BYTES_CHUNK_TYPE; j++ ) { ss << typeBytes[j]; }
  return ss.str();
}

/** @return the actual string */
std::string
PNG::ChunkLayout::wholeChunkStr() {
  std::string s{};
  char hex[3];
  uint32_t total{length()+12};

  for( uint32_t j=0; j<total; j++ ) {
    sprintf( hex, "%02X", wholeChunkBuffer[j] );
    s.append( hex );
  }
  return s;
}

/** @return the whole chunk as a vector */
std::vector<uint8_t>
PNG::ChunkLayout::wholeChunk() {
  std::vector<uint8_t>v;
  uint32_t total{length()+12};

  for( uint32_t j=0; j<total; j++ ) {
    v.push_back( wholeChunkBuffer[j] );
  }
  return v;
}
//--------- END struct ChunkLayout methods implementations ---------------------

/******************************************************************************/
/* struct UTCtime methods implementations */

/** 1 byte -> [0-255] or [0x00-0xFF].
 * 
 * @param val to parse into bytes
 * @param toBytes array of individual bytes
 * @param endian where true = big, false = little
 */
void Text::UTCtime::expressUINT32AsUTCyear( const uint32_t val,
                                            uint8_t toBytes[],
                                            const bool endian )
{
  uint8_t oneByte;
  // Copy const val for shifting
  int tmp = val;

  if( endian ) {
    for( int i=2-1; i>=0; i-- ) { // save bytes in big-endian order
      oneByte = (tmp & 0xff);
      toBytes[i] = oneByte;
      tmp >>= 8;
    }
  }
  else {
    for( int i=0; i<2; i++ ) { // save bytes in little-endian order
      oneByte = (tmp & 0xff);
      toBytes[i] = oneByte;
      tmp >>= 8;
    }
  }
}

/** @param yr current year */
void Text::UTCtime::setYear( int yr ) {
  year = static_cast<uint32_t>(yr);
  expressUINT32AsUTCyear( year, yearBytes, true );
}

/** @return string read-friendly */
std::string Text::UTCtime::to_s() {
  std::string s{"  ^UTC time^"};
  s.append( "  year: " + std::to_string(year) );
  s.append( "   mon: " + std::to_string(mon) );
  s.append( "   day: " + std::to_string(day) );
  s.append( "  hour: " + std::to_string(hr) );
  s.append( "   min: " + std::to_string(min) );
  s.append( "   sec: " + std::to_string(sec) );
  return s;
}
//--------- END struct UTCtime methods implementations -------------------------


}   // END namespace
