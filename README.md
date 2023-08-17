![nist-logo](doc/img/f_nist-logo-brand-2c_512wide.png)

# NFIMM
The **NFIMM (NIST Fingerprint Image Metadata Modifier)** is available for Windows and Linux.  It builds on MacOS but has not been tested.

**NFIMM** is written in C++ and built into a software library.  This affords users to write their own using-application.

All computer-system requirements to build and store **NFIMM** are fairly minimal and are easily satisfied by modern hardware and software tools.  All software dependencies and tools used to compile source code and implement functionality are available at no cost.

Across all operating systems, C++ 11 or higher is required to compile.

# Overview
**NFIMM** functions to update image (file) metadata information.  It leaves the original image file unchanged and generates a new image file with the updated metadata.  It is incumbent upon users to manage their image files.

<!-- end project overview -->

## Example Usage Scenario by NFIR
The NIST Fingerprint Image Resampler (**NFIR**) library is utilized to downsample a PNG source image's image-data from 600 to 500 PPI (pixels per inch); the 500PPI image (known as the "destination") is saved to the file system.  In order to be most correct after resampling, the destination image sample rate (resolution/DPI) header info should be updated to reflect the new sample rate.

Get **NFIR** here: https://github.com/usnistgov/NFIR

After the downsample, **NFIR** calls **NFIMM** and passes it the following image metadata parameters:

Metadata | Object | Value
--------------------------------|---------|--------------------------
source image file               | buffer  | array of bytes
source image compression format | string  | "png"
source sample rate              | int     | 600
destination sample rate         | int     | 500
destination sample rate units   | string  | "inch"
PNG custom text                 | string  | see below

*Table 1 - NFIMM metadata parameters*


For PNG, **NFIMM** programatically insert the following `tEXt` chunk key:value pairs:
```
   Comment:[ Updated pHYs | Inserted pHYs] 
   Software:Header mod by NFIMM
```

Note that specified source and destination sample rate applies to the horizontal and vertical "rows and columns" (of the image).

<span class="redc">
*For fingerprint imagery, the horizontal and vertical sample rates are ALWAYS treated as equal*.
</span>

## NFIMM API

The destination image metadata to be updated is supplied by the NFIMM user/caller:

- Required
  - source image bytes-stream (`std::vector<uint8_t>`)
  - source image compression format
  - destination image sample rate
  - destination image sample rate units
- Optional
  - source image sample rate
  - PNG custom text: Description, Author, Creation Time, etc.

Note that the destination image compression format is constrained to be the same as the source image and therefore is not a required input parameter.

Table 2 lists the available metadata parameters:

Metadata | Required? | Notes
--------------------------------|-----|--------------------------
source image compression format | yes | [ "png" or "bmp" ]
destination image sample rate   | yes | NFIMM raison d'etre
destination image sample units  | yes | [ "inch" or "meter" ]
source image sample rate        | no  | see table below
custom text                     | no  | see table below
**Table 2 - NFIMM metadata**

Parameter | Required? | Note
----------|-----------|-
source image sample rate | no | however, is required if used in PNG custom text
sample image rate units  | no | same as destination
**Table 3 - Source Sample rate parameters**

### PNG Custom Text
Custom text is not ever required to be supplied by the caller of NFIMM.  However, it is useful, see example scenario below.  There are two custom text chunks that are programmatically created and inserted into the destination header by default:
* Software:NFIMM version, and either
* Comment:NFIMM updated source pHYs resolution from {src-img} to {dest-img}PPI, or
* Comment:NFIMM inserted pHYs resolution as {dest-img}PPI

Keyword | Required? | Text
--------|-----------|-
Description   | no | any custom text supplied by user (can be long)
Software      | no | that used to modify the source image's actual image data
Creation Time | no | [ "now" or "file" ]
et al         | no | See PNG Specification 1.2
**Table 4 - PNG valid custom text parameters**

Keyword  | Text | Note
---------|------|-
Description  | Resampled with NFIR from 600PPI ideal bilinear | .
Software | resampled by NFIR v0.1.1 | .
Author   | Twain | .
Creation Time | now | system time at runtime
**Table 5 - PNG custom text example scenario**


## Miscues/Exceptions
All caught exceptions cause the library to abort and throw the exception message to the user.


# Build NFIMM
See file `./build_commands.txt`.

# Use NFIMM
A binary is included in this project to server as an example of how to use NFIMM.

## Perform the Metadata Modification

```
  // Declare the pointers to metadata params and metadata modifier objects.
  // NFIMM is the base class for PNG and BMP derived classes.
  std::shared_ptr<NFIMM::MetadataParameters> mp;
  std::unique_ptr<NFIMM::NFIMM> nfimm_mp;

  try
  {
    mp.reset( new NFIMM::MetadataParameters( imageFormat ) );
    mp->srcImg.resolution.horiz = srcSampleRate;
    mp->srcImg.resolution.vert = srcSampleRate;
    mp->set_srcImgSampleRateUnits( sampleRateUnits );
    mp->destImg.resolution.horiz = destSampleRate;
    mp->destImg.resolution.vert = destSampleRate;
    mp->set_destImgSampleRateUnits( sampleRateUnits );
    mp->destImg.textChunk = vecPngTextChunk;
  }
  catch( const NFIMM::Miscue &e )
  {
    std::cout << "NFIMM user caught exception: " << e.what() << std::endl;
    exit(0);
  }

  // Determine which object to instantiate.
  if( imageFormat == "bmp" )
    nfimm_mp.reset( new NFIMM::BMP( mp ) );
  else
    nfimm_mp.reset( new NFIMM::PNG( mp ) );

  // Read the image, modify the header, save to new file
  try
  {
    nfimm_mp->readImageFileIntoBuffer( pathToSrcImage );
    nfimm_mp->modify();
    nfimm_mp->writeImageBufferToFile( pathToDestImage );
  }
  catch( const NFIMM::Miscue &e )
  {
    std::cout << "NFIMM modification exception " << e.what() << std::endl;
    for( auto s : mp->log ) {
      std::cout << termcolor::red << " * " << s << termcolor::grey << std::endl;
    }
    exit(0);
  }
```

### Doxygen
See README under `./doc` directory.

----------

# Contact
Project Lead: John Libert (john.libert@nist.gov)

Developer NFIMM: Bruce Bandini (bruce.bandini@nist.gov)


# Disclaimer
See the NIST disclaimer at https://www.nist.gov/disclaimer
