#ifndef AUDIO_PRESETS
#define AUDIO_PRESETS
#include "mainHeader.hpp"
#include <JuceHeader.h>

class Program;
class AudioNode;
class AudioEngine;

// Modular interface for the GUI parameters, can be 4 types
struct GUIParam {
    GUIParam(const string& name, void* ptr, ParamType type) :
        name(name), paramPtr(ptr), type(type) {
    }
    string name;
    void* paramPtr;
    ParamType type;
    string saveParamValue();                // dereference and save to string (XML format)
};

class NodePreset {

// Interface for the XML parser. this only holds X/Y location as real data. 
// All the other node properties are dereferenced or obtained via the master node

public:
    NodePreset(AudioNode* mast) : master(mast) {}
    // settings for that node:

    float posX = 0.0f;
    float posY = 0.0f;

    AudioNode* master;
    vector<GUIParam> params;

    string saveCurrentState(node::EditorContext* context);
    void AssignParameters(IXMLDOMNode* params);

    // dereference all pointers and SET params based on strings
    // initial Node Preset (empty) during construction (know which params there are)   

    // to initialize / define settings in new audio fx class (constructor)
    template<typename T>
    void defineSetting(const string& settingName, T* setting) {}

    template<>
    void defineSetting<float>(const string& settingName, float* setting) {
        params.emplace_back(settingName, setting, ParamType::Float);
    }
    template<>
    void defineSetting<int>(const string& settingName, int* setting) {
        params.emplace_back(settingName, setting, ParamType::Int);
    }
    template<>
    void defineSetting<bool>(const string& settingName, bool* setting) {
        params.emplace_back(settingName, setting, ParamType::Bool);
    }
    template<>
    void defineSetting<std::string>(const string& settingName, std::string* setting) {
        params.emplace_back(settingName, setting, ParamType::String);
    }


};



#endif