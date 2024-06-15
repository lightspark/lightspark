Use Unicode's [private use
area](https://en.wikipedia.org/wiki/Private_Use_Areas) to provide standard
glyphs for use on the linux command line.

We use the `U+E000..U+F8FF` range, with 6,400 possible glyphs. We try not to
include brands or specific tools. The glyph for "github" for example, might be
better served by a generic VCS glyph.

The goal is to create a standard for unicode icon glyphs that isn't as strict as
unicode, and that can be made generally available to developers.

Glyphs representing types of tools (eg: and eyedropper tool) or specific
concepts (eg: a difference operation on a set) are welcome, as long as they're
clear.
The goal is to create "timeless" glyphs instead of glyphs for a whole bunch of
different applications/products that may not exist in 50 years.

Brand-icon glyphs and product glyphs are fine, but aren't the primary focus of this project.
They should live in the `U+F0000..U+FFFFD` namespace, called `Supplemental Private Use
Area-A`. If you'd like to register a code point for the use of a brand or a tool,
you're welcome to do so here. Just include an appropriate SVG in the `brands/`
folder, starting with a unique 7-digit number.

`Supplemental Private Use Area-B` will be reservered for large third-party
custom icon sets.

The glyphs from 0000-0257 are built from the MIT-licensed [feather
icons](https://feathericons.com/).


# Glyphs

|Symbol|Char|Name|Hex|File|
|:----:|:--:|----|---|----|
{% for glyph in glyphs -%}
|![]({{glyph.glyph}})|{{glyph.char}}|{{glyph.name}}|{{glyph.code}}|{{glyph.glyph}}|
{% endfor %}

## Building

We use python to generate a `fontManifest.yaml` for use with 
[glyphs2font](https://github.com/rse/glyphs2font).

We use j2cli to build documentation


```bash
python generateFontManifest.py
glyphs2font fontManifest.yaml
j2 template_README.md fontManifest.yaml > README.md #Build docs

```
