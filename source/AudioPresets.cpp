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

