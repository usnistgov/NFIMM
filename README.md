![nist-logo](doc/img/f_nist-logo-brand-2c_512wide.png)

# NFIMM
The **NFIMM (NIST Fingerprint Image Metadata Modifier)** is available for Windows and Linux.  It builds on MacOS but
has not been tested.

**NFIMM** is written in C++ and built into a software library.  This affords users to write their own using-application.

All computer-system requirements to build and store **NFIMM** are fairly minimal and are easily satisfied by modern
 hardware and software tools.  All software dependencies and tools used to compile source code and implement
 functionality are available at no cost.

Across all operating systems, C++17 or higher is required to compile.

# NFIMM Purpose
**NFIMM** has one major purpose: to update an image-file's resolution metadata.

When an image is down- or up-sampled, its resolution metadata should be updated to reflect the "new" resolution.

For those image compression-types that support additional information, like PNG, new metadata may be inserted.

**NFIMM** does not modify the image-file's image-data; it is "transferred" to the newly generated image.

# Quick Start
Since **NFIMM** is designed as a software library, a driver is included in this distribution for convenience.

The binary and its source code provide the following:
1. Run out-of-the-box to validate the build
2. Demonstrate how to use the **NFIMM** library
3. Serve as starting-point for user to write their own using-application

## Windows Build
Using `cmake` and `MSBuild`, build the library and executable.

```
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.8.6
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************

C:\Program Files\Microsoft Visual Studio\2022\Enterprise>cmake --version
cmake version 3.22.3

CMake suite maintained and supported by Kitware (kitware.com/cmake).

C:\Program Files\Microsoft Visual Studio\2022\Enterprise>MSBuild --version
MSBuild version 17.8.5+b5265ef37 for .NET Framework
17.8.5.5502
```

Create and change-to `build` subdir:
```
NFIMM\build> cmake .. -DCMAKE_CONFIGURATION_TYPES="Release" -D_WIN32_64=1
NFIMM\build> MSBuild NFIMM_ITL.sln
```

See also file `./build_commands.txt`.

## Complementary Binary
This simple binary exercises the `NFIMM` library and generates a "new" image.

Note that for the runtime command below, the `-m` switch is not required because PNG is serviced by default, and
 the source-image sample rate is optional (`-a`).

```
.\NFIMM\build\src\bin\Release> NFIMM_bin.exe -b 500 -c inch -s ..\img\src\ducks_grey.png -t ..\img\dest\ducks_grey_comp.png -e "Description:Original image downsampled from 600PPI" "Author:NIST-ITL" "Comment:for NFIMM pub release" "Creation Time:file" -z

```

## Check the Result
There should be a new `ducks_grey.png` image here:
```
NFIMM\img\dest> dir

    <DIR>          .
    <DIR>          ..
           191,986 ducks_grey.png
           191,986 ducks_grey_comp.png
```
The generated and "compare" image headers should be identical except timestamp.

## Example Usage Scenario by NFIR
The NIST Fingerprint Image Resampler (**NFIR**) library is utilized to downsample a PNG source image's image-data
 from 600 to 500 PPI (pixels per inch); the 500PPI image (known as the "destination") is saved to the file system.
 In order to be most correct after resampling, the destination image sample rate (resolution/DPI) header info should
 be updated to reflect the new sample rate.

Get **NFIR** here: https://github.com/usnistgov/NFIR

After the downsample, **NFIR** calls **NFIMM** and passes it the following image metadata parameters:

Metadata | Object | Value
--------------------------------|---------|--------------------------
source image compression format | string  | "png"
source sample rate              | int     | 600
destination sample rate         | int     | 500
destination sample rate units   | string  | "inch"
PNG custom text                 | string  | see below

*Table 1 - NFIMM metadata parameters for NFIR example*


For PNG, **NFIMM** programatically inserts the following `tEXt` chunk key:value pairs:
```
   Comment:[ Updated pHYs | Inserted pHYs] 
   Software:Header mod by NFIMM
```

Note that specified source and destination sample rate applies to the horizontal and vertical "rows and columns"
(of the image).

<span class="redc">
*For fingerprint imagery, the horizontal and vertical sample rates are ALWAYS treated as equal*.
</span>

## NFIMM API
The following are supplied by the NFIMM user/caller:

1. Source image PATH or bytes-stream (`std::vector<uint8_t>`)
2. Destination (aka target) image PATH
3. Source and destination image metadata:

- Required
  - source image compression format
  - destination image sample rate
  - destination image sample rate units
- Optional
  - source image sample rate
  - PNG custom text: Description, Author, Creation Time, etc.

Note that the destination image compression format is constrained to be the same as the source image and therefore
is not a required input parameter.

*Table 2* lists the available metadata parameters:

Metadata | Required? | Notes
--------------------------------|-----|--------------------------
source image compression format | yes | [ "png" or "bmp" ]
destination image sample rate   | yes | NFIMM raison d'etre
destination image sample units  | yes | [ "inch" or "meter" ]
source image sample rate        | no  | see table below
custom text                     | yes | see table below
*Table 2 - NFIMM metadata*

Parameter | Required? | Note
----------|-----------|-
source image sample rate | no | however, is required if used in PNG custom text
sample image rate units  | no | same as destination
*Table 3 - Source Sample rate parameters*

### PNG Custom Text
There are two custom text-chunks that are programmatically created and inserted into the destination header
 by default:
* Software:NFIMM version, and either
* Comment:NFIMM updated source pHYs resolution from {src-img} to {dest-img}PPI, or
* Comment:NFIMM inserted pHYs resolution as {dest-img}PPI

Also, at lease one custom text-chunk is required to be supplied by the caller of NFIMM.

Keyword | Required? | Text
--------|-----------|-
Description   | no | any custom text supplied by user (can be long)
Software      | no | that used to modify the source image's actual image data
Creation Time | no | [ "now" or "file" ]
et al         | no | See PNG Specification 1.2
*Table 4 - PNG valid custom text parameters*

Keyword  | Text | Note
---------|------|-
Description  | Resampled with NFIR from 600PPI ideal bilinear | .
Software | resampled by NFIR v0.1.1 | .
Author   | Twain | .
Creation Time | now | system time at runtime
*Table 5 - PNG custom text example scenario*


## Miscues/Exceptions
All caught exceptions cause the library to abort and throw the exception message to the user.

### Doxygen
See README under `./doc` directory.

----------

# Contact
Project Lead: John Libert (john.libert@nist.gov)

Developer NFIMM: Bruce Bandini (bruce.bandini@nist.gov)


# Disclaimer
See the NIST disclaimer at https://www.nist.gov/disclaimer
