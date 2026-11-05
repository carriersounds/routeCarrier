#include "FileIO.h"
#include "Program.h"


#include "FileIO.h"
#include "Program.h"


FileIO::FileIO(Program* input) {
    prog = input;
    EXEpath = getEXEDir();
    AssetsFolder = EXEpath.parent_path().parent_path() / "assets";
    saveDir = "D:\\LRI\\Vision\\";

    PresetFolder = AssetsFolder / "Settings" / "Presets";
    GlobalFolder = AssetsFolder / "Settings" / "Global";

    makeSureDirectoryExists(saveDir);
    makeSureDirectoryExists(PresetFolder);
    makeSureDirectoryExists(GlobalFolder);

}


bool FileIO::SaveLastPresetPath(FileIOFlags type, string pathStringToSave) {

    string txtFileTarget;

    if (type == f_preset) {
        LastPreset = pathStringToSave;
        txtFileTarget = GlobalFolder.string() + "\\LastPreset.txt";
    }

    std::ofstream file(txtFileTarget, std::ios::trunc);

    if (!file.is_open()) {
        Logger::log("SaveLastPreset: cannot write", level_WARNING, source_FILES);
        return false;
    }
    file << pathStringToSave;
    return true;
}

string FileIO::LoadLastPresetPath(FileIOFlags type) {

    string toOpen;

    if (type == f_preset) {
        toOpen = GlobalFolder.string() + "\\LastPreset.txt";
    }

    std::ifstream file(toOpen);
    if (!file.is_open()) return "";   // first run, or file not yet created

    std::string pathFromFile;
    std::getline(file, pathFromFile);

    if (type == f_preset) {
        LastPreset = pathFromFile;
    }

    return pathFromFile;
}

GlobalPreset FileIO::LoadPreset(string path) {
    bool LocalCoInitialize = SUCCEEDED(CoInitializeEx(nullptr, 0));
    GlobalPreset preset;

    IXMLDOMDocument2* xml = XmlFromFile(path);
    if (!xml) {
        Logger::log("VPRESET: could not load " + path + ", resetting to defaults",
            level_WARNING, source_FILES);
        if (LocalCoInitialize) CoUninitialize();
        return preset;
    }

    IXMLDOMNode* params = XmlSelectSingle(xml, L"/VPRESET/PARAMS");
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
    SaveLastPresetPath(f_preset, path);

    if (LocalCoInitialize) CoUninitialize();
    return preset;
}

bool FileIO::SavePreset(const GlobalPreset& preset, std::string path) {
    std::string out;
    out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out += "<VPRESET version=\"2\">\n";
  //  out += "    <PARAMS parameter=\"" + std::to_string(preset.ThresholdParameter) + "\" method=\"" + std::to_string(preset.ThresholdMethod) + "\"/>\n";
    out += "    <FILTERS>\n";
  //  out += preset.Area.AsXMLEntry();
  //  out += preset.DefectCount.AsXMLEntry();
    out += "    </FILTERS>\n";
    out += "</VPRESET>\n";


    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        Logger::log("PRESET: cannot open for writing: " + path, level_ERROR, source_FILES);
        return false;
    }
    file << out;

    return true;
}

vector<path> FileIO::GetPresetList() {
    namespace fs = std::filesystem;

    vector<std::pair<fs::path, fs::file_time_type>> entries;

    if (!fs::exists(PresetFolder)) return {};

    for (const auto& entry : fs::directory_iterator(PresetFolder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".xml")
            entries.emplace_back(entry.path(), entry.last_write_time());
    }

    // Most recently modified first.
    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<fs::path> presets;
    presets.reserve(entries.size());
    for (auto& [path, time] : entries) presets.push_back(std::move(path));

    return presets;
}

// ------- XML IO Functions ----------
IXMLDOMDocument2* FileIO::XmlCreate() {
    IXMLDOMDocument2* xml = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(DOMDocument60), NULL, CLSCTX_INPROC_SERVER,
        __uuidof(IXMLDOMDocument2), (void**)&xml);
    if (FAILED(hr) || !xml) {
        Logger::log("XMLCreate: CoCreateInstance failed (COM initialised on this thread?)",
            level_ERROR, source_FILES);
        return nullptr;
    }
    xml->put_async(VARIANT_FALSE);
    xml->put_validateOnParse(VARIANT_FALSE);
    xml->put_resolveExternals(VARIANT_FALSE);
    return xml;
}
IXMLDOMDocument2* FileIO::XmlFromString(const std::string& utf8) {
    IXMLDOMDocument2* xml = XmlCreate();
    if (!xml) return nullptr;

    std::wstring wide = tools::StringToWide(utf8);
    VARIANT_BOOL ok = VARIANT_FALSE;
    xml->loadXML(_bstr_t(wide.c_str()), &ok);

    if (ok != VARIANT_TRUE) {
        Logger::log("XMLFromString: " + XmlParseErrorText(xml), level_ERROR, source_FILES);
        xml->Release();
        return nullptr;
    }
    return xml;
}
IXMLDOMDocument2* FileIO::XmlFromFile(const std::string& path) {
    IXMLDOMDocument2* xml = XmlCreate();
    if (!xml) return nullptr;

    // MSXML reads the file itself, so no ifstream / stringstream needed here.
    VARIANT_BOOL ok = VARIANT_FALSE;
    xml->load(_variant_t(path.c_str()), &ok);

    if (ok != VARIANT_TRUE) {
        Logger::log("XMLFromFile: " + XmlParseErrorText(xml) + " (" + path + ")",
            level_WARNING, source_FILES);
        xml->Release();
        return nullptr;
    }
    return xml;
}
string FileIO::XmlParseErrorText(IXMLDOMDocument2* xml) {
    if (!xml) return "no document";

    IXMLDOMParseError* err = nullptr;
    if (FAILED(xml->get_parseError(&err)) || !err) return "parse failed";

    long line = 0, pos = 0;
    BSTR reason = nullptr;
    err->get_line(&line);
    err->get_linepos(&pos);
    err->get_reason(&reason);

    std::string text = reason ? tools::WideToStdString(reason) : "parse failed";
    if (reason) SysFreeString(reason);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();

    err->Release();
    return "line " + std::to_string(line) + ", pos " + std::to_string(pos) + ": " + text + "\nActual Document: ";
}
IXMLDOMNodeList* FileIO::XmlSelectNodes(IXMLDOMDocument2* xml, const wchar_t* xpath, long& countOut) {
    countOut = 0;
    if (!xml || !xpath) return nullptr;

    IXMLDOMNodeList* list = nullptr;
    // _bstr_t rather than a (WCHAR*) cast: selectNodes wants a real BSTR
    // (length-prefixed), and the cast only worked because MSXML tolerates it.
    if (FAILED(xml->selectNodes(_bstr_t(xpath), &list)) || !list) return nullptr;

    list->get_length(&countOut);
    return list;
}
IXMLDOMNode* FileIO::XmlSelectSingle(IXMLDOMDocument2* xml, const wchar_t* xpath) {
    if (!xml || !xpath) return nullptr;
    IXMLDOMNode* node = nullptr;
    if (FAILED(xml->selectSingleNode(_bstr_t(xpath), &node))) return nullptr;
    return node;
}
IXMLDOMNode* FileIO::XmlNodeAt(IXMLDOMNodeList* list, long index) {
    if (!list) return nullptr;
    IXMLDOMNode* node = nullptr;
    if (FAILED(list->get_item(index, &node))) return nullptr;
    return node;
}
string FileIO::XmlAttr(IXMLDOMNode* node, const wchar_t* attrName) {
    if (!node) return "";

    IXMLDOMNamedNodeMap* attrs = nullptr;
    node->get_attributes(&attrs);
    if (!attrs) return "";

    IXMLDOMNode* attr = nullptr;
    attrs->getNamedItem(_bstr_t(attrName), &attr);
    if (!attr) {
        attrs->Release();
        return "";
    }

    BSTR value = nullptr;
    attr->get_text(&value);
    std::string result = value ? tools::WideToStdString(value) : "";
    if (value) SysFreeString(value);

    attr->Release();
    attrs->Release();
    return result;
}
int FileIO::XmlAttrInt(IXMLDOMNode* node, const wchar_t* attrName, int fallback) {
    std::string raw = XmlAttr(node, attrName);
    if (raw.empty()) return fallback;
    try { return std::stoi(raw); }
    catch (const std::exception&) {
        Logger::log("XMLAttrInt: bad integer attribute \"" + raw + "\"", level_WARNING, source_FILES);
        return fallback;
    }
}
float FileIO::XmlAttrFloat(IXMLDOMNode* node, const wchar_t* attrName, float fallback) {
    std::string raw = XmlAttr(node, attrName);
    if (raw.empty()) return fallback;
    try { return std::stof(raw); }
    catch (const std::exception&) {
        Logger::log("XMLAttrFloat: bad float attribute \"" + raw + "\"", level_WARNING, source_FILES);
        return fallback;
    }
}
bool FileIO::XmlAttrBool(IXMLDOMNode* node, const wchar_t* attrName, bool fallback) {
    std::string raw = XmlAttr(node, attrName);
    if (raw.empty()) return fallback;
    if (raw == "1" || raw == "true" || raw == "True" || raw == "TRUE")  return true;
    if (raw == "0" || raw == "false" || raw == "False" || raw == "FALSE") return false;
    return fallback;
}
string FileIO::XmlEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
        case '&':  out += "&amp;";  break;
        case '<':  out += "&lt;";   break;
        case '>':  out += "&gt;";   break;
        case '"':  out += "&quot;"; break;
        default:   out += c;        break;
        }
    }
    return out;
}


// ---------  Misc / helpers  ------------
std::filesystem::path FileIO::getEXEDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}
vector<string> FileIO::glob(const std::string& foldername, bool recursive, bool imagesOnly, string LMFFilter)
{
    std::vector<std::string> fileList;
    std::string searchPath = foldername + "\\*";
    bool isValidFile = 0;

    if (foldername.empty()) {
        Logger::log("Invalid folder path.", level_WARNING, source_FILES);
        return {}; // Return empty list if the folder is invalid.
    }


    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return fileList; // If the folder can't be opened, return empty list
    vector<string> allowedExtentions = { ".bmp", ".tif", ".jpg", ".jpeg", ".png", ".tiff", ".JPEG", ".JPG" };

    do {
        std::string itemName = findData.cFileName;
        if (itemName == "." || itemName == "..") continue; // Skip "." and ".."

        std::string fullPath = foldername + "\\" + itemName;

        // check recursion
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                auto subFiles = glob(fullPath, true); // Recurse if needed
                fileList.insert(fileList.end(), subFiles.begin(), subFiles.end());
            }
        }
        else {

            // check if it's an image (valid) file  
            isValidFile = 0;
            size_t dot = (int)fullPath.find_last_of(".");
            if (dot < 0 || dot == string::npos) continue;       // no extension
            string currentExtention = fullPath.substr(dot);
            for (string ext : allowedExtentions) isValidFile = (ext == currentExtention) ? 1 : isValidFile;

            if (fullPath.find(" - Binarized") != string::npos) isValidFile = false;

            if (!LMFFilter.empty())
                isValidFile &= (fullPath.find(LMFFilter) != string::npos);

            if (isValidFile || !imagesOnly) fileList.push_back(fullPath);
        }


    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
    return fileList;
}
bool FileIO::ShowInExplorer(const std::wstring& filePath)
{
    CoInitialize(0);
    // Convert to a fully qualified, normalized path (handles relative paths, ../, etc.)
    wchar_t fullPath[MAX_PATH];
    if (!GetFullPathNameW(filePath.c_str(), MAX_PATH, fullPath, nullptr))
        return false;

    // Get the PIDL for the file
    PIDLIST_ABSOLUTE pidl = nullptr;
    HRESULT hr = SHParseDisplayName(fullPath, nullptr, &pidl, 0, nullptr);
    if (FAILED(hr) || pidl == nullptr)
        return false;

    // Open (or reuse) the folder window and select the item
    hr = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);

    ILFree(pidl); // always free, even on failure
    CoUninitialize();

    return SUCCEEDED(hr);
}
void FileIO::makeSureDirectoryExists(const std::filesystem::path currentPath) {

    if (currentPath.string().size() < 3) return;    // once you reach a drive, just give up at this point (recursion)

    if (!std::filesystem::is_directory(currentPath)) {

        // if it fails to create, create parent directory  (recursively)
        if (!std::filesystem::create_directories(currentPath)) {
            makeSureDirectoryExists(currentPath.parent_path());
        }
    }
}
string FileIO::saveFileDialog(FileIOFlags filetype, path defaultName) {

    char szFileName[MAX_PATH] = "";
    string initialDir;

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;

    switch (filetype)
    {
    case f_audio:
        ofn.lpstrFilter = "PNG (*.png)\0*.png\0" "JPEG (*.jpg)\0*.jpg\0" "BMP (*.bmp)\0*.bmp\0";
        ofn.lpstrDefExt = "bmp";   // no leading dot
        break;
    case f_preset:
        ofn.lpstrFilter = "Vision preset (*.xml)\0*.xml\0";
        ofn.lpstrDefExt = "xml";
        initialDir = PresetFolder.string();
        break;
    default:
        break;
    }

    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = ARRAYSIZE(szFileName);
    ofn.lpstrInitialDir = initialDir.empty() ? nullptr : initialDir.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Save File As";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn) == TRUE)
        return szFileName;

    // CommDlgExtendedError() returns 0 on a plain cancel, nonzero on a real error
    DWORD err = CommDlgExtendedError();
    (void)err; // log or inspect here if desired
    return "";
}
string FileIO::OpenFolderDialog() {

    if (FAILED(CoInitialize(nullptr))) {
        Logger::log("CoInitialize FAILED for folder dialog", level_ERROR, source_FILES);
        return "";
    };

    IFileDialog* pFileDialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions))) {
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS); // Enable folder picking
        }
        if (SUCCEEDED(pFileDialog->Show(nullptr))) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
                PWSTR pszFolderPath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath))) {
                    wstring folderPath(pszFolderPath);
                    CoTaskMemFree(pszFolderPath);
                    pItem->Release();
                    pFileDialog->Release();

                    string strFolderPath{ folderPath.begin(),folderPath.end() };

                    CoUninitialize();
                    return strFolderPath;
                }
                pItem->Release();
            }
        }
        pFileDialog->Release();
    }
    CoUninitialize();
    return ""; // Return empty string if no folder selected
}
string FileIO::OpenFileDialog(FileIOFlags filetype) {
    string filePath;
    string initialDir;

    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    switch (filetype)
    {
    case f_audio:  ofn.lpstrFilter = ("Image files\0*.png;*.bmp;*.tif;*.jpg;*.jpeg;*.tiff\0");
        break;
    case f_preset:  ofn.lpstrFilter = "Vision presets\0*.XML;*.xml\0";
        initialDir = PresetFolder.string();
        break;
    default:
        break;
    }
    ofn.lpstrInitialDir = initialDir.empty() ? nullptr : initialDir.c_str();

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameA(&ofn)) return {}; // If no file is selected, return empty list

    filePath = ofn.lpstrFile;

    return filePath;
}
void FileIO::deleteDirectoryContents(const std::filesystem::path& dir) {

    try {
        // if there's no files in the save folder, it just skips i guess
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            std::filesystem::remove(entry);
        }
    }
    catch (std::exception e) {
        Logger::log("No entries to remove", level_INFO, source_FILES);
    }

}