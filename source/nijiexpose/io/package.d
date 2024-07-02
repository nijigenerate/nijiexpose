/*
    Copyright © 2020-2023, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.io;

import tinyfiledialogs;
public import tinyfiledialogs : TFD_Filter;
import std.string;
import i18n;

private {
}

/**
    Call a file dialog to open a file.
*/
string insShowOpenDialog(const(TFD_Filter)[] filters, string title = "Open...", string parentWindow = "") {
        c_str filename = tinyfd_openFileDialog(title.toStringz, "", filters, false);
        if (filename !is null) {
            string file = cast(string) filename.fromStringz;
            return file;
        }
        return null;
}

