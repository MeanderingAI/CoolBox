# CoolBox Tutorials

Tutorial source files live in this folder and use the `.tut` format.

## Supported directives

- `@title1: Text` through `@title6: Text`
- `@tags: tag1, tag2, tag3`
- `@image: path-or-url | alt text`
- `@video: path-or-url | caption text`
- `@link: url | label`
- Plain text paragraphs on regular lines

Blank lines separate paragraphs. Local image and video assets can be stored next to the tutorial file and will be copied into the generated static site.

## Publications

The tutorials folder also contains a publications area at [tutorials/publications](publications). It can start empty.

When publication metadata files are added, they can include:

- `title`
- `authors`
- `abstract`
- `year`
- `tags`
- `pdf`
