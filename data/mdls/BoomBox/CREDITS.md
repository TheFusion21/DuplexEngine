CC0 1.0 Universal (Public Domain) — © 2017, Microsoft, from the
[Khronos glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/BoomBox)
repository (`Models/BoomBox`).

`BoomBox.obj` is a re-export of the original glTF geometry (`trimesh`, tangents/bitangents
dropped - `Mesh::LoadOBJ` computes those itself). Textures are re-exported from the original
PNGs, downscaled from 2048² to 1024² and, for the packed
`BoomBox_occlusionRoughnessMetallic.png`, split into separate `BoomBox_occlusion.png` (R),
`BoomBox_roughness.png` (G) and `BoomBox_metallic.png` (B) channels to match this engine's
`BsdfMaterial`, which expects one texture per PBR channel rather than glTF's packed ORM
convention. No other edits were made.
