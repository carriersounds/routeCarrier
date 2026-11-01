#include "AudioEngine.h"
#include "AudioNodes.h"
#include "AudioPresets.h"


string NodePreset::saveCurrentState() {
    string NodeStateString = "NODE {";

    NodeStateString += "<" + blockNames[(int)master->getBlockType()] + ">";
    NodeStateString += master->getBlockName();
    NodeStateString += to_string(master->getID());

    for (auto& param : params) {
        NodeStateString += param.name;
        NodeStateString += " = ";
        NodeStateString += param.saveParamValue();
        NodeStateString += ";";
    }

    NodeStateString += "\n}";

    return NodeStateString;     // confirmed working
}


// save whole node as 1 big json obj incl settings. assumes all member vars have been set already
json NodePreset::SaveNodeToJson() {

    // id = 

    json j;
    j = {
        {"id", id},
        {"type", type},
        {"x", posX},
        {"y", posY}
    };

    for (auto& param : params) {
        j[param.name] = param.to_json();
    }


    string dinges = j.dump(1);

    OutputDebugStringA(dinges.c_str());

    return j;
}

