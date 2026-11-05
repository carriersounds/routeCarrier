#ifndef AUDIO_PRESETS
#define AUDIO_PRESETS
#include "mainHeader.hpp"
#include <JuceHeader.h>


class Program;
class AudioNode;

struct GUIParam {
    GUIParam(const string& name, void* ptr, ParamType type) :
        name(name), paramPtr(ptr), type(type) {
    }

    string name;
    void* paramPtr;
    ParamType type;

    // dereference and save to string
    string saveParamValue() {
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

    // save a single GUI value to a json obj. 'name' is already the main index of param.name
   
    // to / from XML
   

};

class GlobalPreset {

    // nodes
    // links
    // settings within dsp / device node (gain)
    // missing devices
    // message if the preset is from an older/newer version
    // text or bytecode representation function built-in, easy input for fileIO


    // queue preset representation in audioEngine "exportToPresetQueue()"

    // what do I need to create? on creation, queue action.. or look at the current mixer state and build that with some algorithm

    // parse first, then allocate
    // what to save? actions... ID's handle themselves

    // node location????
    // order in preset: device nodes first, then dsp nodes, then links, finally... settings 

    // topologically sorted :

    // node.printCurrentState();

    // how 2

    //const char* preset = "hw=1004,x,y";
    //const char* preset = "hw=1008,x,y";
    //const char* prezet = "fx=1002,x,y";
    //const char* prozxt = "fx=1002,x,y";
    //const char* preset = "hw=1004,x,y";
    //const char* preset = "hw=1008,x,y";
    //const char* prezet = "lk=1002,pin1,pin2";
    //const char* prozxt = "lk=1002,pin1,pin2";
    //const char* preset = "lk=1004,pin1,pin2";
    //const char* preset = "lk=1008,pin1,pin2"; // will always sort topologically

  //  vector<NodePreset> nodes;       // should already update pinnodepairs on creation
                                    // 
  //  vector<BlockLink>& links;

  //  void save() {

 //       string out;

 //       for (auto& node : nodes) {
  //          out += node.saveCurrentState();

 //       }
 //   }

    // LOAD:
    // create node, apply setting, then link


    // full state?

    //const char* preset = "lastID=1008"; //finally set newID to this, so it is above all loaded things (prevent ID clash)
};

class NodePreset {

    // get_to = variable already exists
    // get  = create new variable <arg=type>



public:
    NodePreset(AudioNode* mast) : master(mast) {}
    // settings for that node:

    int id;
    int type;
    float posX;
    float posY;
    // Node Type (Device / DSP)
    // dev=Name ; dsp = type

    AudioNode* master;
    vector<GUIParam> params;

    string blockNames[12] = { "Null", "Input", "Output", "FileIn", "FileOut",
        "Filter","Gain","Reverb","Graphic Equalizer","Saturator","Channel Utility","Compressor" };

    
    string saveCurrentState();


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


};


#endif