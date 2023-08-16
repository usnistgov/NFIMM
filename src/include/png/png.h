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
#pragma once

#include "nfimm_lib.h"
#include "png/crc_public_code.h"

#include <vector>

namespace NFIMM {


/** @brief The first eight bytes of a PNG file
 *
 * The first eight bytes of a PNG file always contain the following values:\n
 *   DEC: 137  80  78  71  13  10  26  10\n
 *   HEX: x89 x50 x4E x47 x0D x0A x1A x0A
 */ 
class Signature {

  const int NUM_BYTES_SIGNATURE{8};

public:
  /** @brief Default constructor not used */
  Signature() = delete;
  /** @brief Constructor used to parse the PNG signature */
  Signature( std::shared_ptr<MetadataParameters> &, std::vector<uint8_t> & );
  /** @brief Does nothing */
  ~Signature() {};

  /** @brief Useful for debug */
  std::string to_s();

  /** @brief PNG signature defined by spec */
  const std::vector<uint8_t> defined{ 137, 80, 78, 71, 13, 10, 26, 10 };
  /** @brief PNG signature defined by spec */
  static inline const std::vector<uint8_t> s_definedHex
        { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
  /** @brief Receives source-image bytes */
  std::vector<uint8_t> dataBytes;

  /** @brief Image header info passed-by and runtime log returned-to caller */
};


/** @brief Support for operations on images in PNG format
 *
 * Only source images that are valid are capable of being processed; destination
 * images cannot be built from 'scratch'.
 *
 * ## PNG Specification Reference
 * PNG (Portable Network Graphics) Specification, Version 1.2, July 1999
 *
 * According to the PNG spec, a PNG file consists of a PNG signature followed
 * by a series of chunks.  The first eight bytes of a PNG file (the "signature")
 * always contain the following (decimal) values:
 *   - 137 80 78 71 13 10 26 10
 *
 * This signature indicates that the remainder of the file contains a single
 * PNG image, consisting of a series of chunks beginning with an `IHDR` chunk
 * and ending with an `IEND` chunk.
 * 
 * Chunk layout: Each chunk consists of four parts:
 * - Length:
 *   A 4-byte unsigned integer giving the number of bytes in the chunk’s data
 *   field.  The length counts only the data field, not itself, the chunk type
 *   code, or the CRC.  Zero is a valid length.
 * - Chunk Type:
 *   A 4-byte chunk type code. For convenience in description and in examining
 *   PNG files, type codes are restricted to consist of uppercase and lowercase
 *   ASCII letters (A–Z and a–z, or 65–90 and 97–122 decimal).
 * - Chunk Data:
 *   The data bytes appropriate to the chunk type, if any.  This field can be
 *   of zero length.
 * - CRC:
 *   A 4-byte CRC (Cyclic Redundancy Check) calculated on the preceding bytes
 *   in the chunk, including the chunk type code and chunk data fields, but not
 *   including the length field.  The CRC is always present, even for chunks
 *   containing no data.
 *
 * Chunks can appear in any order, subject to the restrictions placed on each
 * chunk type.  (One notable restriction is that IHDR must appear first and
 * IEND must appear last; thus the IEND chunk serves as an end-of-file
 * marker).  Multiple chunks of the same type can appear, but only if
 * specifically permitted for that type.
 *
 * ## Modifier Implementation Notes
 * The source image "stream" of bytes (array) is parsed chunk by chunk
 * until `IEND`.
 *
 * The first eight bytes comprise the signature; it is not considered as
 * a chunk.
 *
 * - All Subsequent Chunks:
 * The function `parseAllChunks()` parses all chunks until the IEND chunk is
 * detected. Each chunk is stored in its own object and and array of pointers
 * is maintained to process the chunks after they are parsed.  The first chunk
 * is always `IHDR`; the last chunk is always `IEND` (PNG spec page 18).
 *
 * Two "cursor/bytes-offset" variables are maintained as index into the image
 * buffers.  This is required such that the starting-point for each chunk is
 * well known when reading and writing.
 *
 * - Read-cursor: the first chunk after signature starts on
 *     cursor/byte-offset 8.
 * - Write-cursor: starts at zero to accommodate the PNG signature.
 *
 * After all chunks have been parsed and stored in their own read-object,
 * modification occurs.  The only chunk that is supported for modification
 * is `pHYs` (the image horiz and vert resolution data).  All other chunks are
 * "passed" in the same order as-is, i.e., copied unchanged, from their
 * read-object to the write-buffer.
 *
 * #### `IHDR` standard critical chunk (PNG spec page 15)
 * Since source images must be valid, all `IHDR` data elements are correct.
 * Therefore this chunk is passed without modification from the source to the
 * destination image.
 *
 * #### `pHYs` ancillary chunk (PNG spec page 26)
 * If this chunk exists in the source image header, it is modified and passed
 * to the destination image.  If it does not exist, it is created and inserted
 * into the destination image header.
 *
 * Resolution for horizontal and vertical directions are supported:
 * - set to the value specified by the user as the destination-image sample rate
 * - always set equal to each other
 * - always set in destination image using "meter" as units
 *
 * Note: if destination sample rate is specified as "inch" (PPI), units are
 * converted to "meter" to meet spec requirement.
 *
 * #### Text ancillary chunk (PNG spec page 23)
 * The PNG spec recommends that small text chunks, such as the image title,
 * appear before `IDAT`.  This is the case for the insertion of custom text
 * that could be relevant and would indicate that the destination image has
 * had its metadata modified by this NFIMM library.
 *
 * NFIMM only supports insertion of newly created `tEXt` chunk into the
 * destination image.  Pre-existing `tEXT` chunk(s) in source image are never
 * modified and passed without modification to the destination image.
 * The `tEXt` chunk contains the `tEXt` keyword and a text string:
 * - Keyword: "tEXt"
 * - Null separator: 1 byte : 0x0000
 * - Text: n bytes (character string) : for example,
 *     "Image resampled by NFIR from X to Y PPI".
 *
 * One or more inserted `tEXt` chunks are located immediately before the first
 * `IDAT` chunk.  There is no assumed relation to any other text chunks that
 * may be included in the source image.
 *
 * ### Chunk Notes
 * Inserted chunk-order for destination image:
 * - `pHYs`: before `IDAT`, before the first, inserted `tEXt` chunk, and after
 *    any/all `tEXt` chunks in the source image
 * - `tEXt`: before `IDAT` and after inserted `pHYs` chunk
 *
 * The following metadata are returned to the caller:
 * - `IHDR` chunk:
 *   - source and destination image size (they are identical)
 *   - source image bit-depth, color-type, etc.
 *   - destination image bit-depth, color-type, etc.
 * - `pHYs` chunk:
 *   - source image resolution if it exists
 *   - destination image resolution in PPI and PPMM
 * - `tEXt` chunk(s):
 *   - all existing in source and those inserted by NFIMM
 *   - keyword and its associated text
 */
class PNG : public NFIMM {

  public:
  static const int NUM_BYTES_CHUNK_LENGTH{4};
  static const int NUM_BYTES_CHUNK_TYPE{4};
  static const int NUM_BYTES_CHUNK_CRC{4};

  static size_t _insertChunkIndex;   ///< Maintain index of inserted chunks

  /** @brief Default constructor not used */
  PNG() = delete;
  /** @brief Overloaded constructor always used */
  PNG( std::shared_ptr<MetadataParameters> & );
  /** @brief Does nothing */
  ~PNG() {}


  /** @brief Performs parse of all Chunks
   *
   * Identifies eligible Chunks for modification, inserts Chunks into "new"
   * header, and finally inserts CustomText().
   * Parse the image file's chunks.
   * 1. parse all chunks in the source image read-buffer and save in each
   *    their own chunk-object
   * 2. identify and process the pHYs modification-eligible chunk
   * 3. Determine tEXt custom text insertion
   * 4. transfer chunks in proper order to the destimation image write-buffer
   */
  void modify() override;

  /** @brief Parse all chunks in source image including the image bytes */
  void parseAllChunks( int = 8 );

  // Functions to check PNG image after all chunks have been parsed.
  /** @brief Checks all chunks located in the remaining-chunks buffers */
  void processExistingChunks();
  /** @brief Checks all chunks in the src image for insert into destination */
  void insertChunkPhys();

  // Functions to insert custom tEXt.
  /** @brief Insert any/all text chunks into destination image header */
  void insertCustomText();

  // Functions to build and write PNG image.
  /** @brief Transfer chunks from source to destination buffer */
  void xferChunks();
  // unsigned long crc( unsigned char *, int );
  /** @brief Transfer all bytes from source to destination buffer */
  void xferBytesBetweenBuffers( std::vector<uint8_t>&,
                                const std::vector<uint8_t>& );
  /** @brief Useful for debug */
  std::string to_s();

  public:

  /** @brief Container for PNG chunk and the 4-parts:
   *  length, type, data, and CRC. */
  struct ChunkLayout {
    /** @brief Pointer to the whole chunk (all 4-parts)
     *  Deleted after xfer to write-buffer, see PNG::xferChunks(). */
    uint8_t *wholeChunkBuffer;

    /** @brief Convert the entire chunk's buffer bytes to single vector */
    std::vector<uint8_t> wholeChunk();
    /** @brief Convert the entire chunk's buffer bytes to single string */
    std::string wholeChunkStr();

    uint8_t lengthBytes[NUM_BYTES_CHUNK_LENGTH]{0}; ///< Indiv bytes array

    /** @brief Convert the length-bytes to single value */
    uint32_t length();

    uint8_t typeBytes[NUM_BYTES_CHUNK_TYPE]{0};  ///< Indiv bytes array

    /** @brief Convert the type-bytes to single string */
    std::string type();

    uint8_t *dataBuffer;  ///< Pointer to the chunk data
    /** @brief Convert the data buffer bytes to single string; useful for debug */
    std::string data();

    uint8_t crcBytes[NUM_BYTES_CHUNK_CRC]{0};  ///< Indiv bytes array
    /** @brief Convert the CRC-bytes to single string; useful for debug */
    std::string crc();

    /** @brief Concat each of the 4-parts into a single array */
    void concatenate4parts();
  };   // END struct ChunkLayout

  /** @brief Container for pointers to chunks parsed from source image. */
  std::vector<std::shared_ptr<ChunkLayout>> _srcChunkPointers;
  /** @brief Container for pointers to chunks inserted into destination image. */
  static std::vector<std::shared_ptr<ChunkLayout>> s_insertChunkPointers;

  /** @brief Set to true if `pHYs` chunk exists in source image header */
  static inline bool s_pHYsChunkExists{false};
 
  /** @brief Supports parsing of each byte in the buffer */
  static inline uint8_t s_oneByte{0};
  /** @brief Current offset/index into destination image buffer for WRITE */
  int _w_cursor{0};

private:
  /** @brief Maintain COUNT of source-image chunks */
  uint32_t _countChunk{0};

  /** @brief Container for all PNG Critial and Ancillary chunk types
   *
   * "IHDR" Image header\n
   * "PLTE" Pallete contains from 1 to 256 entries(each a three-byte series)\n
   * "IDAT" the Image Data\n
   * "IEND" last chunk (required)\n
   * "cHRM" Primary chromaticities\n
   * "gAMA" relationship between the image samples and the desired display
   *        output intensity as a power function\n
   * "iCCP" embedded ICC profile\n
   * "sBIT" significant BITs\n
   * "sRGB" standard RGB color space\n
   * "bKGD" background color\n
   * "hIST" palette histogram\n
   * "tRNS" Transparency\n
   * "pHYs" pHYsical pixel dimensions\n
   * "sPLT" suggested palette\n
   * "tIME" image last-modification time\n
   * "iTXt" international textual data\n
   * "tEXt" textual data\n
   * "zTXt" compressed textual data
   */
  std::vector<std::string> _allChunkTypes{
    "IHDR",
    "PLTE",
    "IDAT",
    "IEND",
    "cHRM",
    "gAMA",
    "iCCP",
    "sBIT",
    "sRGB",
    "bKGD",
    "hIST",
    "tRNS",
    "pHYs",
    "sPLT",
    "tIME",
    "iTXt",
    "tEXt",
    "zTXt"
  };
};   // END class PNG

/** @brief PNG Image header chunk
 *
 * The IHDR chunk must be the first chunk after the signature (chunk).
 * It contains 13 bytes of data, and the total length of this chunk is 25 bytes:
 *   - LEN: 4 bytes
 *   - IHDR: string `IHDR`, 4 bytes
 *   - DATA: 13 bytes
 *   - CRC: 4 bytes
 *
 * The IHDR offset is byte 8 from START (byte 0).
 *
 * The image width in this chunk is in bytes 0-3; height in bytes 4-7.
 * Bit depth, Color type, Compression method, Filter method, and Interlace
 * method take the remaining 5 bytes of the DATA.
 */
class IhdrX {

  static const int NUM_BYTES_CHUNK_IHDR_TOTAL{25};
  static const int NUM_BYTES_IHDR_DATA{13};
  static const int NUM_BYTES_IHDR_WIDTH{4};
  static const int NUM_BYTES_IHDR_HEIGHT{4};

  public:
  /** @brief Default constructor not used */
  IhdrX() = delete;
  /** @brief Constructor used to parse chunk */
  IhdrX( std::shared_ptr<MetadataParameters> &, std::shared_ptr<PNG::ChunkLayout> & );
  /** @brief Does nothing */
  ~IhdrX() {}

  /* @brief Index into the `_srcChunkPointers` vector */
  // uint32_t _idx{0};

  /* @brief Image header info passed-by and runtime log returned-to caller  */
  // std::shared_ptr<MetadataParameters> _params;

  protected:

  /** @brief Container for image width and height */
  struct Dimension {
    uint32_t width{0};    ///< 4 bytes
    uint32_t height{0};   ///< 4 bytes

    /** @brief Individual-bytes array */
    uint8_t widthBytes[NUM_BYTES_IHDR_WIDTH]{0};
    /** @brief Individual-bytes array */
    uint8_t heightBytes[NUM_BYTES_IHDR_HEIGHT]{0};
  };

  /** @brief Container for the `IHDR` chunk's data
   * - width
   * - height
   * - bit depth
   * - color type
   * - compression method
   * - filter method
   * - interlace method
   */
  struct ImageInfo {

    /** @brief Single-byte giving the number of bits per sample or per
        palette index (not per pixel) */
    uint8_t bitDepth{0};
    /** @brief Single-byte that describes the interpretation of the image
        data */
    uint8_t colorType{0};
    /** @brief Single-byte that indicates the method used to compress
     *  the image data
     *
     * Only compression method 0 (deflate/inflate compression with a sliding
     * window of at most 32768 bytes) is defined.
     */
    uint8_t compressionMethod{0};
    /** @brief Single-byte that indicates the method used to filter */
    uint8_t filterMethod{0};
    /** @brief Single-byte that indicates the interlace method */
    uint8_t interlaceMethod{0};

    Dimension dimension;

    /** @brief New resolution with which to update destination-image */
    struct {
      uint32_t horizontal{0};  ///< Image resolution
      uint32_t vertical{0};    ///< Image resolution
    } resolution;

  };   // END struct ImageInfo

  /** @brief Container for the entire `IHDR` chunk
   *
   * Since the `IHDR` is fixed, the length of the data is always 13 bytes.
   */
  struct ImageHDR {
    /** @brief Entire chunk from LEN to CRC inclusive */
    uint8_t  wholeChunk[NUM_BYTES_CHUNK_IHDR_TOTAL];
    uint8_t  lenData[PNG::NUM_BYTES_CHUNK_LENGTH]{0};  ///< Indiv bytes array
    uint32_t length{0};                           ///< Len of data, 13 bytes
    uint8_t  typeBytes[PNG::NUM_BYTES_CHUNK_TYPE]{0};  ///< Indiv bytes array
    uint8_t  data[NUM_BYTES_IHDR_DATA]{0};        ///< Indiv bytes array
    ImageInfo imageInfo;                          ///< The chunk's data
    uint8_t  crc[PNG::NUM_BYTES_CHUNK_CRC]{0};         ///< Indiv bytes array

    /** @brief Convert the 4-bytes of type to a string */
    std::string tostring_type();
  } _imageHDR;   ///< Container for IHDR chunk
};   // END class IhdrX


/** @brief PNG physical pixel dimensions chunk
 *
 * The `pHYs` chunk contains 9 bytes of data, and the total length of this
 * chunk is 21 bytes:
 *   - LEN:  4 bytes
 *   - pHYs: 4 bytes, string `pHYs`
 *   - DATA: 9 bytes, horiz & vert resolution and units
 *   - CRC:  4 bytes
 *
 * Build the DATA based on the image resolution value specified by the user.
 * The horizontal and vertical resolutions are identical.
 */ 
class Phys {

  static const int NUM_BYTES_CHUNK_PHYS_TOTAL{21};
  static const int NUM_BYTES_PHYS_DATA{9};
  static const int NUM_BYTES_PHYS_RESOLUTION{4};
  static const int BYTE_PHYS_UNITS{0x01};

  public:
  /** @brief Default constructor not used */
  Phys() = delete;
  /** @brief Support to insert-new or update-existing chunk */
  Phys( std::shared_ptr<MetadataParameters> &,
        std::shared_ptr<PNG::ChunkLayout> & );
  /** @brief Does nothing */
  ~Phys() {}

  /** @brief Insert `pHYs` chunk since does not exist in source image header */
  void insertChunk();
  /** @brief Parse `pHYs` chunk if exists in source image header */
  void parseChunk();
  /** @brief Update existing `pHYs` chunk from source image header for xfer
   * to destination image header */
  void updateChunk();

  /** @brief Image header info passed-by and runtime log returned-to caller */
  std::shared_ptr<MetadataParameters> _params;

  /** @brief Index into the `_srcChunkPointers` vector */
  uint32_t _idx{0};

  /** @brief Container for the pHYs chunk's data  */
  struct ImageResolution {

    uint32_t horizontal{0};  ///< Horiz resolution
    uint32_t vertical{0};    ///< Vert resolution
    uint8_t  units{0};       ///< Single-byte integer of resolution units

    uint8_t horizontalBytes[NUM_BYTES_PHYS_RESOLUTION]{0}; ///< Indiv bytes array
    uint8_t verticalBytes[NUM_BYTES_PHYS_RESOLUTION]{0};   ///< Indiv bytes array

    /** @brief Build the string */
    std::string horizBytesHex();

    /** @brief Build the string */
    std::string vertBytesHex();

    /** @brief Build the string */
    std::string resolution_to_s();

    /** @brief Decifer units text from byte value */
    std::string units_to_s();

    /** @brief Builds string of pHYs's data using hex formatted values */
    std::string to_s();
  };   // END struct ImageResolution

private:
  /** @brief Container for the entire pHYs chunk
   *
   * Since the pHYs is fixed, the length of the data is always 9 bytes.
   */
  struct ImagepHYs {
    /** Entire chunk from LEN to CRC inclusive */
    uint8_t  wholeChunk[NUM_BYTES_CHUNK_PHYS_TOTAL];
    uint8_t  lenData[PNG::NUM_BYTES_CHUNK_LENGTH]{0};  ///< Indiv bytes array
    uint32_t length{0};                           ///< Len of data, 9 bytes
    uint8_t  typeBytes[PNG::NUM_BYTES_CHUNK_TYPE]{0};  ///< Indiv bytes array
    uint8_t  data[NUM_BYTES_PHYS_DATA]{0};        ///< Indiv bytes array
    ImageResolution imageResolution;              ///< The chunk's data
    uint8_t  crc[PNG::NUM_BYTES_CHUNK_CRC]{0};         ///< Indiv bytes array


    /** @brief Convert the 9-bytes of data to a string */
    std::string tostring_data();

    /** @brief Convert the 4-bytes of type to a string */
    std::string tostring_type();

    /** @brief Builds string of pHYs's data using hex formatted values */
    std::string to_s();

  } _imagepHYs;   ///< Container for pHYs chunk

  /** @brief Used for update of source pHYs header */
  std::shared_ptr<PNG::ChunkLayout> _chnk;
};   // END class Phys


/** @brief PNG Textual information chunk
 *
 * Build the chunks by iterating through the vector of comments (strings);
 * each vector-string is "converted" into its own `tEXt` chunk.
 *
 * Each comment string's keyword is verified against the list of predefined;
 * invalid comment-keyword is ignored.  Ignored keywords are dumped to log file.
 *
 * The Comment chunk describes resampling from/to rates, e.g., "1000 to 500 PPI".
 * The sample rates are taken from the MetadataParameters to build the string
 * (of bytes) for the chunk.
 *
 * Example chunk that is built:
 * LEN-type-data-CRC where:
 * - LEN = len of keyword + len of text + 1  where the 1 is for the
 *     null-separator=0x00
 * - type = `tEXt`
 * - data = metadata string converted to bytes: 'keyword + null-separator + text'
 * - CRC = calculation for concatenation of type and data
 */
class Text {

  const int NULL_SEPARATOR{0x00};  ///< Between tEXt keyword and its text
  const int NUM_BYTES_UTC{7};      ///< Univ Time Coordinated

  public:
  /** @brief Default constructor not used */
  Text() = delete;
  /** @brief Overloaded constructor always used */
  Text( std::shared_ptr<MetadataParameters> & );
  /** @brief Does nothing */
  ~Text() {}

  /** @brief Prior to 1972, this time was called Greenwich Mean Time (GMT)
   *
   * Now referred as Coordinated Universal Time or Universal Time Coordinated
   * (UTC).  It is a coordinated time scale, maintained by the Bureau
   * International des Poids et Mesures (BIPM). It is also known as "Z time"
   * or "Zulu Time".
   */
  static struct UTCtime{
    uint8_t yearBytes[2];  ///< Indiv bytes array
    uint32_t year{0};      ///< 4-digits
    uint8_t mon{0};        ///< 1 - 12
    uint8_t day{0};        ///< 0 - 31
    uint8_t hr{0};         ///< 0 - 23
    uint8_t min{0};        ///< 0 - 59
    uint8_t sec{0};        ///< 0 - 60

    /** @brief Express a uint32 value as a series of two bytes */
    void expressUINT32AsUTCyear( const uint32_t, uint8_t[], const bool );

    /** @brief Convert year into 2-byte array */
    void setYear( int yr );

    /** @brief Make readable */
    std::string to_s();

  } uTCtime;  ///< Support PNG tEXt chunk

  /** @brief Insert `tEXt` key:value pairs into destination image metadata */
  void insertChunks();

  /** @brief Image header info passed-by and runtime log returned-to caller  */
  std::shared_ptr<MetadataParameters> _params;
  /** @brief Get date/time for file last modified */
  void getFiletime( const std::string &, UTCtime * );
  /** @brief Get date/time from computer clock */
  void getUTCtime( UTCtime * );

  /** @brief Predefined keywords for chunk `tEXt`
   *
   * Per the PNG spec: Keywords must be spelled exactly as registered, so that
   * decoders can use simple literal comparisons when looking for particular
   * keywords.  In particular, keywords are considered case-sensitive.  Any
   * number of text chunks can appear, and more than one with the same keyword
   * is permissible.
   */
  std::vector<std::string> _textKeywords {
    "Title",
    "Author",
    "Description",
    "Copyright",
    "Creation Time",
    "Software",
    "Disclaimer",
    "Warning",
    "Source",
    "Comment"
  };

};   // END class Text


}   // END namespace
