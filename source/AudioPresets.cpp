#include "AudioEngine.h"
#include "Program.h"        // for fileIO


void GlobalPreset::Load(FileIOFlags openDialog, string path) {

    std::string PresetFilePath;

    Logger::log("Loading preset...", level_INFO, source_FILES);

    if (openDialog == f_openDialog)
        PresetFilePath = FileIO::OpenFileDialog(f_preset);
    else if (path.size())
        PresetFilePath = path;
    else
        PresetFilePath = FileIO::LastPreset.string();

    if (PresetFilePath.size() < 1) return;

    node::SetCurrentEditor(engine->prog->gui.node_Context);
    engine->clearAll();

    bool LocalCoInitialize = SUCCEEDED(CoInitializeEx(nullptr, 0));
    auto* xml = FileIO::XmlFromFile(PresetFilePath);
    if (!xml) {
        Logger::log("PRESET: could not load " + PresetFilePath + ", Incorrect file format",level_WARNING, source_FILES);
        if (LocalCoInitialize) CoUninitialize();      
    }

    long countNode = 0;
    IXMLDOMNodeList* NodeList = FileIO::XmlSelectNodes(xml, L"/PRESET/NODES/NODE", countNode);
   
    for (long i = 0; i < countNode; i++) {
        IXMLDOMNode* SingleNode = FileIO::XmlNodeAt(NodeList, i);
        if (!SingleNode) continue;

        BlockType type = (BlockType)FileIO::XmlAttrInt(SingleNode, L"type");
        NodeID ID = FileIO::XmlAttrInt(SingleNode, L"id");

        if (type == BlockType::InputDevice || type == BlockType::OutputDevice) 
           engine->addNewDeviceNode(type, FileIO::XmlAttr(SingleNode, L"name"), ID);      
        else 
            engine->addNewDSPNode(type,ID);
        
        ImVec2 SetPos = { FileIO::XmlAttrFloat(SingleNode, L"posX"),FileIO::XmlAttrFloat(SingleNode, L"posY") };     
        node::SetNodePosition(ID, SetPos);
   
        // switch case param handler ?

        SingleNode->Release();
    }

    if(NodeList) NodeList->Release();
      
    long countLink = 0;
    IXMLDOMNodeList* LinkList = FileIO::XmlSelectNodes(xml, L"/PRESET/LINKS/LINK", countLink);

    for (long i = 0; i < countLink; i++) {
        IXMLDOMNode* SingleLink = FileIO::XmlNodeAt(LinkList, i);
        if (!SingleLink) continue;

        LinkID Lid = FileIO::XmlAttrInt(SingleLink, L"ID");
        PinID LeftPin = FileIO::XmlAttrInt(SingleLink, L"Left");
        PinID RightPin = FileIO::XmlAttrInt(SingleLink, L"Right");
        engine->createLink(LeftPin, RightPin, Lid);
        SingleLink->Release();
    }
    if (LinkList) LinkList->Release();


    Logger::log("Loaded preset: " + PresetFilePath, level_INFO, source_FILES);

    // Link list



    node::SetCurrentEditor(nullptr);
    if (xml)
    xml->Release();
    FileIO::SaveLastPresetPath(f_preset, path);
    if (LocalCoInitialize) CoUninitialize();

}

void GlobalPreset::Save(FileIOFlags openDialog, string path){


    string savePath;

    if (openDialog == f_openDialog)
        savePath = FileIO::saveFileDialog(f_preset);
    else if (path.size())
        savePath = path;
    else
        savePath = FileIO::LastPreset.string();

    if (savePath.size() < 1) return;


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

    Logger::log("Saved preset to : " + savePath, level_INFO, source_FILES);

    FileIO::SaveXML(out, savePath);
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

