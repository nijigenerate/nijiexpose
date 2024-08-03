module nijiexpose.io.image;

import nijilive;

Puppet neLoadModelFromImage(string filename) {
    auto tex = new ShallowTexture(filename);
    inTexPremultiply(tex.data, tex.channels);

    int minX = tex.width + 1, minY = tex.height + 1, maxX = -1, maxY = -1;
    for (int y = 0; y < tex.height; y ++) {
        for (int x = 0; x < tex.width; x ++) {
            if (tex.data[(y * tex.width + x) * tex.channels + tex.channels - 1] != 0) {
                minX = min(minX, x);
                minY = min(minY, y);
                maxX = max(maxX, x);
                maxY = max(maxY, y);
            }
        }
    }
    import std.stdio;
    writefln("%s: %d, %d, %d, %d", filename, minX, minY, maxX, maxY);
    ubyte[] data;
    data.length = (maxX - minX + 1) * (maxY - minY + 1) * tex.channels;
    for (int y = minY; y <= maxY; y ++) {
        for (int x = minX; x < maxX; x ++) {
            for (int c = 0; c < tex.channels; c++) {
                data[((y-minY) * (maxX - minX + 1) + (x-minX)) * tex.channels + c] = tex.data[(y * tex.width + x) * tex.channels + c];
            }
        }
    }
    auto tex2 = new ShallowTexture(data, maxX - minX + 1, maxY - minY + 1, tex.channels);

    auto part = inCreateSimplePart(*tex2, null, filename);
    Puppet puppet = new Puppet(part);
    return puppet;
}