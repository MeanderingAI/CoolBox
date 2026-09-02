module coolbox;

import core.stdc.stddef : size_t;
import core.stdc.string : strlen;

extern (C) nothrow @nogc {
    const(char)* coolbox_c_version();
    const(char)* coolbox_c_describe();
    size_t coolbox_c_capability_count();
    const(char)* coolbox_c_capability_at(size_t index);
    int coolbox_c_is_ready();
}

struct Client {
    string endpoint;

    string version() const {
        return _fromCString(coolbox_c_version());
    }

    string describe() const {
        return _fromCString(coolbox_c_describe());
    }

    bool isReady() const {
        return coolbox_c_is_ready() != 0;
    }

    size_t capabilityCount() const {
        return coolbox_c_capability_count();
    }

    string capabilityAt(size_t index) const {
        if (index >= capabilityCount()) {
            return "";
        }
        return _fromCString(coolbox_c_capability_at(index));
    }

    string[] capabilities() const {
        const count = capabilityCount();
        auto values = new string[](count);
        foreach (i; 0 .. count) {
            values[i] = capabilityAt(i);
        }
        return values;
    }
}

Client createDefault() {
    return Client("local://coolbox");
}

Client forEndpoint(string endpoint) {
    return Client(endpoint);
}

private string _fromCString(const(char)* value) {
    if (value is null) {
        return "";
    }
    return value[0 .. strlen(value)].idup;
}
