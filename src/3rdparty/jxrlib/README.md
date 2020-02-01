# jxrlib
### JPEG XR Format
JPEG XR is a still image format based on
technology originally developed by Mirosoft under the name HD Photo (formerly
Windows Media™ Photo). The JPEG XR format is similar, but not identical, to the
HD Photo/Windows Media™ Photo format.

The JPEG XR format replaces the HD Photo/Windows Media™ Photo format in both
Windows 8 and the Windows Image Component (WIC). WIC accompanies the Internet
Explorer 10 redistributable packages for down-level versions of Windows.
Some “Windows Media™ Photo” (WMP) naming conventions are still used internally
with this release of the DPK.

JPEG XR files use the .jxr extension.  Applications that support the JPEG XR
file format should recognize and decode HD Photo/Windows Media™ Photo
.hdp/.wdp files, but only offer to create files with the .jxr extension.

### JPEG XR Image Coding Specification
The JPEG XR Image Coding Spectification provides a detailed specification of the 
compression encoder and decoder algorithms plus the detailed structure of the 
compressed data (elementary) bit stream.  This document is designed to be used in 
conjunction with the included source code.  If you find instances where the code 
differs from the documentation, the code implementation should be used as the 
reference.

The JPEG XR Image Coding Spectification is an international standard and is
available at: http://www.itu.int/rec/T-REC-T.832 while the reference software is
available at: http://www.itu.int/rec/T-REC-T.835.

### Status of JPEG XR implementation
"JPEGXR_DPK_1.0.doc" documents the contents of this porting kit, the usage of 
the command line file conversion utilities (JXREncApp.exe and JXRDecApp.exe), and 
technical details of the API's and data structures between these sample command 
line applications and the core codec.

The code and documentation in this release represent the final design and 
specification of the 1.0 bit stream, and can be used as the reference for final 
implementations of encoders and decoders for JPEG XR.

This release of the DPK has received extensive testing of all the various pixel 
formats, encoder options and modes of operation.  We are confident that most errors 
and other bugs have been resolved.  Any code bugs, documentation errors or other 
discrepancies found in this release candidate should be reported to Microsoft as 
promptly as possible.  These can be submitted to hdview@microsoft.com.

This DPK provides basic support for big endian architectures.  We have 
successfully tested the encoder and decoder using a big endian processor.  This 
support is provided as a starting reference to be adapted to the specific 
platform and hardware architecture of the target system.

### Contact Information
For any and all technical questions or feedback about any part of this Device
Porting Kit, including the documentation, please send email to:

  hdview@microsoft.com

We will respond as promptly as possible with answers to your questions.

Additional information, best practices, tools, utilities, sample code, sample 
image content, links to additional resources and community discussion can 
currently be found at http://hdview.wordpress.com/.

#### The Microsoft JPEG XR Development Team
### Interesting links
[JPEG XR Updates](https://hdview.wordpress.com/2013/05/30/jpegxr-updates/)

[JPEG XR Photoshop Plugin and Source Code](https://hdview.wordpress.com/2013/04/11/jpegxr-photoshop-plugin-and-source-code/)
