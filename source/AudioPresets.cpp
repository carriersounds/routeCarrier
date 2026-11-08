#include "AudioEngine.h"
#include "Program.h"        // for fileIO / XML

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

void NodePreset::AssignParameters(IXMLDOMNode* XMLParams) {

    for (auto& GUIParam : this->params) {

        switch (GUIParam.type) {
        case ParamType::Float: 
            *(float*)GUIParam.paramPtr = FileIO::XmlAttrFloat(XMLParams, tools::StringToWide(GUIParam.name).c_str());
             break;
        case ParamType::Int: 
             *(int*)GUIParam.paramPtr = FileIO::XmlAttrInt(XMLParams, tools::StringToWide(GUIParam.name).c_str());       
             break;
        case ParamType::Bool: 
            *(bool*)GUIParam.paramPtr = FileIO::XmlAttrBool(XMLParams, tools::StringToWide(GUIParam.name).c_str());
             break;
        case ParamType::String: 
            *(string*)GUIParam.paramPtr = FileIO::XmlAttr(XMLParams, tools::StringToWide(GUIParam.name).c_str());       // might break with pointers ?
            break;
        default:
            break;
        }
    }
}

string GUIParam::saveParamValue(){
    switch (type)
    {
    case ParamType::Float: {
        float* cast = (float*)paramPtr;
        return to_string(*cast);
    }
        break;
    case ParamType::Int: {
        int* cast = (int*)paramPtr;
        return to_string(*cast);
    }
        break;
    case ParamType::Bool: {
        bool* cast = (bool*)paramPtr;
        return to_string(*cast);
    }
        break;
    case ParamType::String: {
        string* cast = (string*)paramPtr;
        return *cast;
    }
        break;
    default:
        break;
    }


}
