module nijiexpose.tracking.ext;
import ft;
import ft.adaptors;
public import nijiexpose.tracking.ext.exvmc;

Adaptor neCreateAdaptor(string name, string[string] options = null) {
    if (name == "VMC Receiver") {
        return new ExVMCAdaptor;
    } else {
        if (options is null) return ftCreateAdaptor(name);
        else return ftCreateAdaptor(name, options);
    }
}