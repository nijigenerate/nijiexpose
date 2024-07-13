module nijiexpose.io.image;

import nijilive;

Puppet neLoadModelFromImage(string filename) {
    auto tex = new ShallowTexture(filename);
    inTexPremultiply(tex.data, tex.channels);

    auto part = inCreateSimplePart(*tex, null, filename);
    Puppet puppet = new Puppet(part);
    return puppet;
}