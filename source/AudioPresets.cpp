#include "AudioEngine.h"
#include "Program.h"        // for fileIO


void GlobalPreset::Load(FileIOFlags openDialog, string path) {

    bool LocalCoInitialize = SUCCEEDED(CoInitializeEx(nullptr, 0));
    auto* xml = FileIO::XmlFromFile(path);

    if (!xml) {
        Logger::log("VPRESET: could not load " + path + ", resetting to defaults",
            level_WARNING, source_FILES);
        if (LocalCoInitialize) CoUninitialize();
        
    }

    IXMLDOMNode* params = FileIO::XmlSelectSingle(xml, L"/VPRESET/PARAMS");
    if (params) {
        //preset.ThresholdParameter = XmlAttrFloat(params, L"parameter", preset.ThresholdParameter);
        //preset.ThresholdMethod = XmlAttrInt(params, L"method", preset.ThresholdMethod);
        params->Release();
    }
    else {
        Logger::log("VPRESET: no <PARAMS> node, threshold params left at defaults",
            level_INFO, source_FILES);
    }


    xml->Release();
    FileIO::SaveLastPresetPath(f_preset, path);

    if (LocalCoInitialize) CoUninitialize();


    LoadToEngine();
}

void GlobalPreset::Save(FileIOFlags openDialog, string path){

    UpdateFromEngine();

    string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out += "<PRESET version=\"1\">\n";

    out += "   <GLOBAL>\n";
    out += "      <SETUP LastID=\"" + to_string(engine->uniqueID) + "\"\n";
    out += "   </GLOBAL>\n";
    out += "   <NODES>\n";
    for (auto& [ID,node] : engine->nodes)
        out += node->currentState.saveCurrentState(engine->prog->gui.node_Context);       // save each node's own preset params. required engine context to get ax::nodecontext for location

    out += "   </NODES>\n";  
    out += "   <LINKS>\n";
    for (auto& [ID,lp] : engine->links)
        out += lp.ToXML();

    out += "   </LINKS>\n";
 
    out += "</PRESET>";

    string realpath = FileIO::saveFileDialog(f_preset);

    Logger::log("Saved preset to : " + realpath, level_INFO, source_FILES);

    FileIO::SaveXML(out, realpath);
}


void GlobalPreset::UpdateFromEngine() {



    // settings within dsp / device node (gain)
    // missing devices
    // message if the preset is from an older/newer version

}

void GlobalPreset::LoadToEngine() {


}

// Requires context for location information
string NodePreset::saveCurrentState(node::EditorContext* context) {

    string NodeStateString;
    NodeStateString +=
        "      <NODE type=\"" + blockNames[(int)master->getBlockType()] +
        "\" name=\"" + master->getBlockName() + "\"" +
          " id=\"" + to_string(master->getID()) + "\" ";                        // Pin ID's get set automatically if node is created with correct ID

        for (auto& param : params) {
            NodeStateString += param.name + "=\"";
            NodeStateString += param.saveParamValue();
            NodeStateString += "\" ";
        }
    node::SetCurrentEditor(context);
    NodeStateString += " posX=\"" + tools::to_string_float(node::GetNodePosition(master->getID()).x) + "\" ";
    NodeStateString += " posY=\"" + tools::to_string_float(node::GetNodePosition(master->getID()).y) + "\" ";
    node::SetCurrentEditor(nullptr);

    NodeStateString += "/>\n";

    return NodeStateString;     // confirmed working
}

