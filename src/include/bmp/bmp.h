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

namespace NFIMM {


/** @brief Support for operations on images in BMP format
 *
 * Multi-byte values in BMP format are little-endian. For example, if the first
 * six bytes of the file contain '42 4D EE A5 03 00', then the file size in
 * bytes 2-5 = '00 03 A5 EE' == 239,086.
 *
 * There are three INFOHEADER elements that are updated:
 *   * horiz and vert resolution (always)
 *   * image size (if and only if source image size was == 0)
 *
 * BMP metadata is contained between the first byte of the file and the first
 * byte of the pixel-data-array.  This metadata is comprised of File Header
 * and the DIB (device independent bitmap) Header (bitmap information header).
 *
 * The File Header is always the file's first 14 bytes.
 *
 * **NFIMM** supports only the following DIB Headers:
 *
 *   * BITMAPCOREHEADER - 12-bytes
 *   * BITMAPINFOHEADER - 40-bytes
 *
 * Since there is no version field in the headers, the only way to determine
 * the type of image structure is by checking the DIB header's `size` field
 * which is the first 4-bytes of the header.
 */
class BMP : public NFIMM {

  public:
  static const int NUM_BYTES_BITMAPFILEHEADER{14};
  static const int NUM_BYTES_DIB_BITMAPCOREHEADER{12};
  static const int NUM_BYTES_DIB_BITMAPINFOHEADER{40};
  static const int NUM_BYTES_BM_IDENTIFIER{2};

  /** @brief Default constructor not used */
  BMP() = delete;
  /** @brief Overloaded constructor always used */
  BMP( std::shared_ptr<MetadataParameters> & );
  /** @brief Does nothing */
  ~BMP() {}

  /** @brief Modify the headers according to source image format */
  void modify() override;
  /** @brief Retrieve current Metadata Parameters */
  std::string to_s();

  /** @brief Transfer all bytes from source to destination buffer */
  void xferBytesBetweenBuffers( std::vector<uint8_t>&,
                                const std::vector<uint8_t>& );

  /** @brief Read source image pixel data (after the headers) */
  void readImagePixels( const uint32_t, std::vector<uint8_t> & );
};   // END class BMP


/** @brief BMP File header contains 14-bytes
 *
 * The file header is read and saved in memory (buffer) for write to
 * destination image; there are no mods to this header.
 *
 * The first two bytes are checked to be == 'BM' and error is thrown if not.
 *
 * The last 4 bytes contain the offset to the start of the image (pixel)
 * data; this value should not have to change since the info header metadata
 * is replaced to maintain its length.
 */
class FileHeader {
public:
  /** @brief Default constructor not used */
  FileHeader() = delete;
  /** @brief Always use this overloaded ctor */
  FileHeader( std::shared_ptr<MetadataParameters> & );

  /** @brief Image header info passed-by and runtime log returned-to caller */
  std::shared_ptr<MetadataParameters> _params;

  /** @brief Read the first 14 bytes of the file into the File header container */
  void read();

  /** @brief Container for entire header */
  std::vector<uint8_t> _vecEntireHeader;

  /** @brief Comparator for first two bytes of BMP header, usually 'BM' */
  const uint8_t _fileType[BMP::NUM_BYTES_BM_IDENTIFIER] = { 0x42, 0x4D };  // 'BM'
  /** @brief bytes 0-1 == 'BM' */
  uint8_t _bfType[BMP::NUM_BYTES_BM_IDENTIFIER]{0};
  /** @brief bytes 2-5 file size */
  uint8_t _bfSize[4]{0};
  /** @brief bytes 6-7 unused */
  uint8_t _bfReserved1[2]{0};
  /** @brief bytes 8-9 unused */
  uint8_t _bfReserved2[2]{0};
  /** @brief bytes 10-13 offset to start of pixel data */
  uint8_t _bfOffBits[4]{0};

  /** @brief Container for actual values
   *
   * This is useful to verify source-image metadata params. */
  struct {
    /** @brief Size of entire image file: headers + pixels */
    uint32_t fileSize{0};
    /** @brief Number of bytes to start of pixels from the first byte */
    uint32_t offsetToPixelData{0};
    /** @brief Difference: fileSize - offsetToPixelData */
    uint32_t calculated_size_image{0};
  } _actual;

  /** @brief Loads the FILEHEADER in vector container */
  void headerAsVector();

  /** @brief Convert the FileHeader to single string; useful for debug */
  std::string to_s( const std::string & );
  /** @brief Convert the FileHeader to string of hex; useful for debug */
  std::string to_s_hex();
};   // END class FileHeader


/** @brief BMP Info header contains 40-bytes
 *
 * There are three fields to be updated: _biSize, biXPelsPerMeter,
 * and biYPelsPerMeter.
 */
class InfoHeader {
public:
  /** @brief Default constructor not used */
  InfoHeader() = delete;
  /** @brief Always use this overloaded ctor */
  InfoHeader( std::shared_ptr<MetadataParameters> & );

  /** @brief Image header info passed-by and runtime log returned-to caller  */
  std::shared_ptr<MetadataParameters> _params;

  /** @brief Read the 40 bytes of the Info header */
  void read();
  /** @brief Update the Info header with user-specified metadata values */
  void update();
  /** @brief Write the individual header elements into one entire buffer */
  // void write();

  /** @brief Retrieve current header data */
  std::string to_s( const std::string & );
  /** @brief Convert the InfoHeader to string of hex; useful for debug */
  std::string to_s_hex();

  /** @brief Container for entire Info header */
  std::vector<uint8_t> _vecEntireHeader;

  /** @brief bytes 0-3 header size, value must be == 40 */
  uint8_t _biSize[4]{0};
  /** @brief bytes 4-7 image width */
  uint8_t _biWidth[4]{0};
  /** @brief bytes 8-11 image height */
  uint8_t _biHeight[4]{0};
  /** @brief bytes 12-13 num of planes must == 1 */
  uint8_t _biPlanes[2]{0};
  /** @brief bytes 14-15 bits per pixel (depth) 1,4,8,16,24, or 32 */
  uint8_t _biBitCount[2]{0};
  /** @brief bytes 16-19 compression type */
  uint8_t _biCompression[4]{0};
  /** @brief bytes 20-23 image size - may be zero if not compressed */
  uint8_t _biSizeImage[4]{0};
  /** @brief bytes 24-27 X sample rate pixels per meter */
  uint8_t _biXPelsPerMeter[4]{0};
  /** @brief bytes 28-31 Y sample rate pixels per meter */
  uint8_t _biYPelsPerMeter[4]{0};
  /** @brief bytes 32-35 num entries in color-map actually used */
  uint8_t _biClrUsed[4]{0};
  /** @brief bytes 36-39 num significant colors */
  uint8_t _biClrImportant[4]{0};

  /** @brief Container for actual values
   *
   * This is useful to verify source-image metadata params. It is also used
   * to update the dest-image X and Y dir resolution. That is, the source-image
   * resolution is incorrect (otherwise no purpose for this NFIMM library),
   * and that resolution is overwritten with the correct(ed)
   * resolution/sample-rate as specified by user (metadata).
   */
  struct {
    uint32_t headerCountBytes{0};
    uint32_t width{0};
    uint32_t padded_width{0};
    uint32_t height{0};
    uint16_t count_planes{0};
    uint16_t bit_depth{0};
    uint32_t compression_type{0};
    uint32_t size_image{0};
    uint32_t horizontal_ppmm{0};
    uint32_t vertical_ppmm{0};
    uint32_t horizontal_ppi{0};
    uint32_t vertical_ppi{0};
    uint32_t colors_used{0};
    uint32_t colors_important{0};
  } _actual;

  /** @brief Loads the INFOHEADER in vector container */
  void headerAsVector();

};   // END class InfoHeader

}   // END namespace
