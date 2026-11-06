#pragma once
#include "mainHeader.hpp"
#include <wrl/client.h>
#include <coml2api.h>       
#include <objidl.h>
#include <combaseapi.h>
#include <windows.h>
#include <commdlg.h>
#include <objbase.h>  
#include <msxml6.h> 

#include "AudioEngine.h"

using std::filesystem::path;

class Program;
class AudioNode;


class FileIO {

public:

    FileIO(Program* input);

    // Basic File functions
    static string           OpenFolderDialog();
    static string           OpenFileDialog(FileIOFlags filetype = f_audio);
    static string           saveFileDialog(FileIOFlags filetype = f_audio, path defaultName = "");
    static void             makeSureDirectoryExists(const path path);
    static void             deleteDirectoryContents(const path& dir);
    static vector<string>   glob(const std::string& foldername, bool recursive, bool imagesOnly = true, string LMFFilter = "");
    static path             getEXEDir();
    static bool             ShowInExplorer(const std::wstring& filePath);

    static bool             SaveXML(string presetXML, std::string path); // includes SaveLast

    static string           LoadLastPresetPath(FileIOFlags type);       // includes setting member
    static bool             SaveLastPresetPath(FileIOFlags type, string path);  // includes setting member

    vector<path>            GetPresetList();

    // XML Parsing
    static IXMLDOMDocument2* XmlCreate();
    static IXMLDOMDocument2* XmlFromString(const std::string& utf8);
    static IXMLDOMDocument2* XmlFromFile(const std::string& path);
    static std::string       XmlParseErrorText(IXMLDOMDocument2* xml);

    static IXMLDOMNodeList* XmlSelectNodes(IXMLDOMDocument2* xml, const wchar_t* xpath, long& countOut);
    static IXMLDOMNode*     XmlSelectSingle(IXMLDOMDocument2* xml, const wchar_t* xpath);
    static IXMLDOMNode*     XmlNodeAt(IXMLDOMNodeList* list, long index);

    static string            XmlAttr(IXMLDOMNode* node, const wchar_t* attrName);
    static int               XmlAttrInt(IXMLDOMNode* node, const wchar_t* attrName, int fallback = 0);
    static float             XmlAttrFloat(IXMLDOMNode* node, const wchar_t* attrName, float fallback = 0.0f);
    static bool              XmlAttrBool(IXMLDOMNode* node, const wchar_t* attrName, bool fallback = false);
    static string            XmlEscape(const std::string& text);
    template<typename T>
    static T                 XmlAttrValue(IXMLDOMNode* node, const wchar_t* attrName, T fallback) {
        if constexpr (std::is_same_v<T, int>)   return XmlAttrInt(node, attrName, fallback);
        if constexpr (std::is_same_v<T, float>) return XmlAttrFloat(node, attrName, fallback);
    }

    static inline path EXEpath;
    static inline path saveDir;
    static inline path AssetsFolder;
    static inline path PresetFolder;
    static inline path GlobalFolder;

    static inline path LastPreset;

private:
    Program* prog;
};

