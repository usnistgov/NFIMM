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

#include "miscue.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

/** @brief NFIMM version number */
const std::string NFIMM_VERSION{"0.1.2"};


namespace NFIMM {


/** @brief Image header metadata modification support
 *
 * ## Overview
 * This class is used to set the image header metadata in the destination
 * image.  The constructor signature with string param verfies the user-input
 * compression type against those that are supported by NFIMM: bmp and png.
 *
 * It also contains a "log" container that is updated with runtime info that
 * could be helpful in the event of metadata update failures.  The log is kept
 * here because the metadata parameters are available to the caller.
 *
 * ## Source image sample-rate for PNG
 * The source image resolution/sample-rate metadata is optional unless the image
 * resolution is NOT included in the source image header, and, the text field
 * is requested to be added to the destination image in a `tEXt` chunk.
 *
 * Example scenarios:
 *  - source image contains resolution in the `pHYs` chunk that is non-zero:
 *    this value (for example 1000) is used to build `tEXt` chunk Comment:
 *    - "Original image resolution 1000PPI"
 *  - source image does not contain resolution: the `srcImg.resolution`
 *    value passed-in by the caller is used to update the `tEXt` chunk
 *    Description:
 *     - "Image resampled by NFIR: 600 to 500 PPI"
 */
class MetadataParameters
{
  /** @brief Source image format (hence the destination format): png or bmp */
  std::string compression{};

  public:

  /** @brief Runtime log updated by NFIMM, init empty */
  std::vector<std::string> log{};
  /** @brief Function to push to log required to access from derived classes */
  void loggit( const std::string & );

  /** @brief Source image metadata */
  struct {
    std::string compression{};  ///< src image compression format
    /** @brief source image resolution */
    struct {
      uint32_t horiz{0};   ///< horizontal sample-rate
      uint32_t vert{0};    ///< vertical sample-rate
      uint8_t  units{0};   ///< header spec for horiz and vert units
      std::string unitsStr{};  ///< [ inch | meter | other ]
    } resolution;
    struct {
      uint32_t width{0};   ///< in pixels
      uint32_t height{0};  ///< in pixels
    } dimensions;          ///< of the image
    std::string path{};    ///< for case PNG file creation timestamp
    uint32_t existingPhysResolution{0};  ///< for PNG pHYs chunk in src image
  } srcImg;

  /** @brief Destination image metadata
   * Note that dest image dimensions cannot be changed hence no struct
   * similar to srcImg.
   */
  struct {
    std::string compression{};  ///< dest image compression format
    /** @brief Dest image resolution */
    struct {
      uint32_t horiz{0};   ///< horizontal sample-rate
      uint32_t vert{0};    ///< vertical sample-rate
      uint8_t  units{0};   ///< single byte horiz and vert units
      std::string unitsStr{};  ///< [ inch | meter | other ]
    } resolution;
    std::vector<std::string> textChunk{};    ///< list of custom comments
  } destImg;

  struct {
    /** @brief Count of chunks in source image header */
    uint32_t countSourceChunks{0};
    /** @brief Count is incremented when pHYS is missing in source image
        and for each custom text specified by user */
    uint32_t countInsertChunks{0};
    /** @brief Total of source image chunks plus any chunks to be inserted */
    uint32_t sumChunks()
    {
      return countSourceChunks + countInsertChunks;
    }
  } pngWriteImageInfo;

  /** @brief Default constructor not used */
  MetadataParameters() = delete;
  /** @brief This constructor always used */
  MetadataParameters( const std::string & );


  /** @brief Get image sample rate units */
  std::string get_imgSampleRateUnits( const std::string & );

  /** @brief Set == 1 if "meter", 0 otherwise */
  void set_destImgSampleRateUnits( const std::string & );

  /** @brief Set == 1 if "meter", 0 otherwise */
  void set_srcImgSampleRateUnits( const std::string & );

  /** @brief Get the current metadata parameters */
  std::string to_s();
};   // END class MetadataParameters

std::string printVersion();

/** @brief The NIST Fingerprint Image Metadata Modification (NFIMM) library API
 * 
 * PURPOSE:
 * - To treat digital fingerprint image as a stream of bytes in order to update
 *   image metadata while maintaining image-data integrity.
 * 
 * The following metadata are supported:
 * - image resolution (BMP and PNG)
 * - custom text (automated or specified by user, PNG only)
 * 
 * TERMINOLOGY:
 * - source image: the image to be updated with new metadata
 * - destination (dest) image: image that has been updated and made available
 *   to the caller
 * - metadata: image information that describes the image (the image "header")
 * 
 * HOW TO USE:
 * - The caller instantiates this class using the constructor that takes the
 *   API struct (object) that contains all metadata required to be changed.
 * - Metadata that is capable of being extracted from the source image is passed
 *   unchanged during the update process (to the destination file).
 * - Metadata specific by the caller (e.g. sample-rate and comments) are added
 *   to the appropriate image header location.
 * 
 * API struct (object):
 * - compression image formats supported: PNG, BMP
 * - metadata parameters:
 *   - source image resolution (horizontal and vertical, optional)
 *   - dest image resolution (horizontal and vertical)
 *   - custom text (PNG only)
 */
class NFIMM
{
public:
  /** @brief Container for entire source input image */
  static inline std::vector<uint8_t> s_readBuffer;
  /** @brief Current offset/index into source image buffer for READ */
  static inline int s_r_cursor;

  /** @brief Container for entire destination output image */
  static inline std::vector<uint8_t> s_writeBuffer;
  /** @brief Current offset/index into source image buffer for WRITE */
  static inline int s_w_cursor;

  /** @brief Image header info passed-by and runtime log returned-to caller */
  std::shared_ptr<MetadataParameters> _params;

public:
  /** @brief Helper function express a series of four bytes to a uint32 value */
  static void expressFourBytesAsUINT32( uint32_t &, const uint8_t[], const bool );
  /** @brief Helper function express a series of two bytes to a uint16 value */
  static void expressTwoBytesAsUINT16( uint16_t &, const uint8_t[], const bool );

public:
  /** @brief Default constructor not used */
  NFIMM() = delete;

  /** @brief Full constructor with all accessible metadata members defined */
  NFIMM( std::shared_ptr<MetadataParameters> & );

  /** @brief Opens and reads the entire source image file into memory */
  void readImageFileIntoBuffer( const std::string & );
  /** @brief Loads the entire source image bytes into vector */
  void readImageFileIntoBuffer( const std::vector<uint8_t> );
  /** @brief Loads the destination image buffer into vector */
  void retrieveWriteImageBuffer( std::vector<uint8_t> & );
  /** @brief Write destination image buffer to file */
  void writeImageBufferToFile( const std::string & );

  /** @brief Modify the headers according to source image format
   * Empty implementation required for linking. */
  virtual void modify() {};

  /** @brief Helper function express a uint32 value as a series of four bytes */
  static void expressUINT32AsFourBytes( const size_t, uint8_t[], const bool );

  /** @brief Helper function get next 4 bytes in buffer */
  static void next4bytes( uint8_t [] );
  /** @brief Helper function get next number of bytes in buffer */
  static void nextLengthBytes( const uint32_t, uint8_t [] );

  /** @brief Get Metadata Parameters */
  virtual std::string to_s() { return ""; }

  /** @brief Does nothing */
  virtual ~NFIMM() {  }

  /** @brief Helper function convert pixels per millimeter to pixels per inch */
  static void convertPPMMtoPPI( const uint32_t, uint32_t & );
  /** @brief Helper function convert pixels per inch to pixels per millimeter */
  static void convertPPItoPPMM( const uint32_t, uint32_t & );
};   // END class NFIMM

}   // END namespace
