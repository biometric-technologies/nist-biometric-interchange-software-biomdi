![](.github/readme/banner.svg)

# NIST BiomDI application package, mirror

An unmodified copy of NIST's BiomDI application package (NIST also calls it BIOMAPP). BiomDI, short for Biometric Data Interchange, is the US National Institute of Standards and Technology's C code for the standard record formats biometric systems exchange: ISO/IEC 19794 and the matching ANSI INCITS formats for fingerprint minutiae, fingerprint images, face images and iris images. The core BiomDI libraries handle those records; this repository holds the companion applications that are built on top of them. The original plain-text `README` from NIST sits next to this file and is kept as published. Iriscan keeps the mirror as a stable reference for [biometric-converter](https://github.com/biometric-technologies/biometric-converter), which builds ISO/IEC 19794-2 and ANSI INCITS 378 minutiae records with the BiomDI finger minutiae library.

## What is inside

From NIST's own `README`:

- `smartcard`: libraries and tools to talk to smart cards over PC/SC and to handle tag-length-value objects.
- `matchoncard`: tools and example test drivers from the NIST Match-on-Card evaluation; uses the smart card library.
- `piv`: library and tools (`pivv`, `pivprobe`, `m1rec2piv`) to validate the biometric parts of Personal Identity Verification records. PIV fingerprint records are ANSI INCITS 378 minutiae records, so `pivv` uses the BiomDI finger minutiae (FMR) library.
- `irex` and `irexsdkv`: tools, a library and an example test driver from the NIST IREX 2008 iris evaluation. NIST notes this part has not been maintained since October 2009.

The core BiomDI libraries themselves (`common`, `fingerminutiae`, `fingerimage`, `facerecognition`) are not in this repository. `piv/README` says they must be built and installed first, from NIST's BiomDI distribution; the biometric-converter README clones them from [github.com/usnistgov/biomdi](https://github.com/usnistgov/biomdi).

## How to build

The top-level `Makefile` builds `irex` by default. Edit `SUBDIRS` to choose packages, for example `SUBDIRS := smartcard matchoncard piv` for the card tools (uncomment the line that is already there). Then:

```sh
make
sudo make install
```

Requirements, from the package READMEs: the BiomDI core libraries installed first (headers are looked up under `/usr/local/include`); a PC/SC implementation such as `pcsclite` for `smartcard`, `matchoncard` and `piv`; the Jasper JPEG 2000 and GD libraries for `irex`. On macOS the PC/SC headers come with the Command Line Tools.

## Where it is used

[biometric-converter](https://github.com/biometric-technologies/biometric-converter) uses the BiomDI finger minutiae library together with NIST NBIS to convert fingerprint images to minutiae templates and templates between ISO/IEC 19794-2 and ANSI INCITS 378. This mirror is kept alongside the NBIS mirror so the two NIST dependencies of the converter are available at a known revision.

## Related repositories

- [biometric-converter](https://github.com/biometric-technologies/biometric-converter): the converter that depends on BiomDI and NBIS.
- [nist-biometric-image-software-nbis](https://github.com/biometric-technologies/nist-biometric-image-software-nbis): mirror of NIST NBIS (minutiae detection, matching, WSQ).
- [docs](https://github.com/biometric-technologies/docs): source of the developer documentation at docs.iriscan.net.

## License

Public domain. The software was developed at NIST by employees of the US Federal Government in the course of their official duties and, under 17 U.S.C. 105, is not subject to copyright; NIST makes no guarantees about its quality or reliability. The statement is at the top of `README`, the `Makefile` and each package's `README`. There is no separate LICENSE file.

## Contact

Questions about this mirror or the converter: info@iriscan.net, https://www.iriscan.com. For BiomDI itself, see NIST's biometric software pages.
