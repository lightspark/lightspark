from pathlib import Path
from yaml import load, dump

featherGlyphs = Path("./featherIcons/").glob("*.svg")

glyphs=(
    *featherGlyphs,
)

yamlDump = {
    'font':{
        'ttf': 'OpenFontIcons.ttf',
        'fixedwidth': True,
        'name': 'OpenFontIcon',
    },
    'glyphs':[],
}

for glyph in glyphs:
    yamlDump['glyphs'].append({
        'glyph': str(glyph),
        'name': glyph.name[5:][:-4],
        'code': hex(int(glyph.name[:4])+57344),
        'char': chr(int(glyph.name[:4])+57344),
    })


Path('./fontManifest.yaml').write_text(dump(yamlDump, allow_unicode=True))

yamlDump = {
    'font':{
        'ttf': 'BrandFontIcons.ttf',
        'fixedwidth': True,
        'name': 'BrandFontIcon',
    },
    'glyphs':[],
}

for glyph in Path("./brands/").glob("*.svg"):
    yamlDump['glyphs'].append({
        'glyph': str(glyph),
        'name': glyph.name[8:][:-4],
        'code': hex(int(glyph.name[:7])+983040),
        'char': chr(int(glyph.name[:7])+983040),
    })


Path('./brandFontManifest.yaml').write_text(dump(yamlDump, allow_unicode=True))
