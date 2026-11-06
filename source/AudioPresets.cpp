#include "AudioEngine.h"
#include "Program.h"        // for fileIO


void GlobalPreset::Load(FileIOFlags openDialog, string path) {

    string realpath = FileIO::OpenFileDialog(f_preset);
    bool LocalCoInitialize = SUCCEEDED(CoInitializeEx(nullptr, 0));
    auto* xml = FileIO::XmlFromFile(realpath);
    if (!xml) {
        Logger::log("PRESET: could not load " + realpath + ", Incorrect file format",level_WARNING, source_FILES);
        if (LocalCoInitialize) CoUninitialize();      
    }

    long count = 0;
    IXMLDOMNodeList* NodeList = FileIO::XmlSelectNodes(xml, L"/PRESET/NODES/NODE", count);
    
    node::SetCurrentEditor(engine->prog->gui.node_Context);
    
    for (long i = 0; i < count; i++) {
        IXMLDOMNode* SingleNode = FileIO::XmlNodeAt(NodeList, i);
        if (!SingleNode) continue;


        BlockType type = (BlockType)FileIO::XmlAttrInt(SingleNode, L"type");
        NodeID ID = FileIO::XmlAttrInt(SingleNode, L"id");

        if (type == BlockType::InputDevice || type == BlockType::OutputDevice) {
           engine->addNewDeviceNode(type, FileIO::XmlAttr(SingleNode, L"name"), ID);
        }
        else {
            engine->addNewDSPNode(type,ID);
        }

        ImVec2 SetPos = { FileIO::XmlAttrFloat(SingleNode, L"posX"),FileIO::XmlAttrFloat(SingleNode, L"posY") };
        
        node::SetNodePosition(ID, SetPos);
   
        // switch case param handler ?

        SingleNode->Release();
    }

    node::SetCurrentEditor(nullptr);

    if(NodeList)
    NodeList->Release();
      

    // Link list
   // auto* global = FileIO::XmlSelectSingle(xml, L"/PRESET/GLOBAL/SETUP");

    if (xml)
    xml->Release();
    FileIO::SaveLastPresetPath(f_preset, path);
    if (LocalCoInitialize) CoUninitialize();
   // LoadToEngine();
}

void GlobalPreset::Save(FileIOFlags openDialog, string path){

    int VERSION = 1;

    string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out += "<PRESET version=\" "+ to_string(VERSION) + "\">\n";

    out += "   <GLOBAL>\n";
    out += "      <SETUP LastID=\"" + to_string(engine->uniqueID) + "\"/>\n";
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

void GlobalPreset::LoadToEngine() {






    // settings within dsp / device node (gain)
    // missing devices
    // message if the preset is from an older/newer version

}

// Requires context for location information
string NodePreset::saveCurrentState(node::EditorContext* context) {

    string NodeStateString;
    NodeStateString +=
        "      <NODE type=\"" + to_string((int)master->getBlockType()) +
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

