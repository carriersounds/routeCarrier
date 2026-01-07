#include "FileIO.h"
#include "Program.h"

FileIO::FileIO(Program* input) {
    currentImage = 0;
    imgCount = 0;
    prog = input;
    lastSavedImage = "";

    EXEpath = getEXEDir();
    saveDir = getEXEDir() / "assets\\Recordings\\";

}

std::filesystem::path FileIO::getEXEDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

void FileIO::selectFile(bool folder, int recursive, bool openDialog, string extFileLoad)
{
    allFilesInScope = openAndLoadFiles(recursive, folder, openDialog, extFileLoad);
}

void FileIO::manualSaveFile(bool as) {

    void* bufferToSave = nullptr;

    if (bufferToSave == nullptr) {
        Logger::log("No image to be saved");
        return;
    }



    // default
    if (!as) {
        //------------- SET FILENAME + PATH ---------------//
        std::string FilePath = saveDir.string();


        time_t t = time(0);   // get time now
        tm* now = localtime(&t);
        char timestamp[80];
        strftime(timestamp, 80, "%Y-%m-%d %H.%M.%S - ", now);

        FilePath += "ManualSave_";
        FilePath += timestamp;

        std::string FullDirectory = FilePath + "Vision.bmp";  
    }
    else {
        string fulldir = saveFileDialog();      // SAVE AS ...

        if (fulldir == "") {
            Logger::log("File not saved", level_ERROR, source_FILES);
            return;
        }

    }

    Logger::log("Image saved at " + lastSavedImage, level_INFO, source_FILES);
}

void FileIO::saveTextFile(string path) {

    //------------- SET UNIQUE FILENAME + PATH ---------------//

    char addchar[9];
    sprintf(addchar, "%08x", 1337);

    std::string FilePath;
    std::string FullDirectory;
    std::string TextFileName;

    TextFileName = FullDirectory + ".vvr";

    //------------------ TEXT OUTPUT --------------------//

    std::ofstream file(TextFileName);

    string HEAD = "<MARKERS version=\"0\">\n";
    string FOOT = "</MARKERS>";

    string TextContent = HEAD;

    string blobData;


    TextContent.append(FOOT);

    if (file.is_open()) {       // Check if the file is successfully opened
        file << TextContent;    // Write the string content to the file
        file.close();           // Close the file stream
    }
}

string FileIO::saveFileDialog(int filetype) {

    char szFileName[MAX_PATH] = ""; // Initialize  empty

    OPENFILENAMEA ofn; // ANSI character set
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;


    if (filetype == f_image) {
        ofn.lpstrFilter = "PNG (*.png)\0*.png\0" "JPEG (*.jpg)\0*.jpg\0" // Filter for text files
            "BMP (*.bmp)\0*.bmp\0";
    }
    else if (filetype == f_settings) {
        ofn.lpstrFilter = "Config file (*.txt)\0*.txt\0";
    }
    ofn.nFilterIndex = 1;       // default start
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);

    // Set initial directory. NULL = current directory.
    ofn.lpstrInitialDir = NULL;

    // Set the dialog box title.
    ofn.lpstrTitle = "Save File As";

    // Set the default file extension if the user doesn't provide one.
    if (filetype == f_image) ofn.lpstrDefExt = "bmp"; else ofn.lpstrDefExt = "txt";

    ofn.Flags = OFN_OVERWRITEPROMPT | // Prompt the user if the selected file already exists.
        OFN_NOCHANGEDIR;      // Restores the current directory to its original value if it was changed by the user.

// displays the "Save As" dialog box.
// It returns TRUE if the user specifies a file name and clicks the OK button,
// and FALSE otherwise (e.g., if the user clicks Cancel or an error occurs).

    if (GetSaveFileNameA(&ofn) == TRUE) {
        // If the user clicked "Save", print the selected file path.
        return szFileName;
    } else {
        return "";
        CommDlgExtendedError();
    }

}

void FileIO::loadTextFile(string imagepath) {

    std::filesystem::path filepath(imagepath);
    string base_filename = filepath.stem().string();
    size_t found = base_filename.find(" - Binarized");   // if is binarized image

    if (found != string::npos) {                // if is binarized image, remove appendix

        string example = std::to_string(found);
        base_filename.erase(found);
    }

    // Construct the filename of the corresponding text file
    string text_filename = base_filename + ".vvr";
    std::ifstream file(text_filename);

    int lineNumber = 1; // where the blob data starts

    if (file.is_open()) {

        string line;

        std::getline(file, line); // move fwd 1 line

        lineNumber = 0; // to reset blobdata array allocation

        try {

            while (std::getline(file, line)) {

                lineNumber++;
            }
        }
        catch (std::exception e) {
            Logger::log((string)("Reading file failed: ") + e.what(), level_WARNING, source_FILES);
        }
        file.close();
    }
   
  
}

vector<string> FileIO::openAndLoadFiles(bool recursive, bool folder, bool openDialog, string extFileLoad) {

    string folderPath;
    string filePath;
    CoInitialize(nullptr);

    if (openDialog) {
        if (!folder) {
            filePath = OpenFileDialog();
            folderPath = filePath.substr(0, filePath.find_last_of("/\\")); // Extract folder path

        }
        else {
            folderPath = OpenFolderDialog();
        }
    }
    else {
        filePath = extFileLoad;
        folderPath = filePath.substr(0, filePath.find_last_of("/\\")); // Extract folder path
    }


    auto allFilesInScope = glob(folderPath, recursive);

    imgCount = (int)allFilesInScope.size();

    currentImage = 0;   // folder selection starts at 0

    for (int i = 0; i < imgCount; i++)                // get current index for image
        if(filePath == allFilesInScope[i]) currentImage = i;   // short circuit evaluation (assign if they're equal)

    Logger::log(to_string(imgCount) + " images loaded from folder: " + folderPath, level_INFO, source_FILES);

    return allFilesInScope;
}

vector<string> FileIO::glob(const std::string& foldername, bool recursive)
{
    std::vector<std::string> fileList;
    std::string searchPath = foldername + "\\*";
    bool isValidFile = 0;


    if (foldername.empty()) {
        Logger::log("Invalid folder path.",level_WARNING,source_FILES);
        return {}; // Return empty list if the folder is invalid.
    }


    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);


    if (hFind == INVALID_HANDLE_VALUE) return fileList; // If the folder can't be opened, return empty list

    vector<string> allowedExtentions = { ".bmp", ".tif", ".jpg", ".jpeg", ".png", ".tiff", ".JPEG", ".JPG"};

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

            if (isValidFile) fileList.push_back(fullPath);
        }


    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
    return fileList;
}

string FileIO::OpenFolderDialog() {

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
                    std::wstring folderPath(pszFolderPath);
                    CoTaskMemFree(pszFolderPath);
                    pItem->Release();
                    pFileDialog->Release();

                    string strFolderPath{ folderPath.begin(),folderPath.end() };

                    return strFolderPath;
                }
                pItem->Release();
            }
        }
        pFileDialog->Release();
    }
    return ""; // Return empty string if no folder selected
}

string FileIO::OpenFileDialog(int filetype) {
    string filePath;

    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    if (filetype == f_image) {
        ofn.lpstrFilter = ("Image files\0*.png;*.bmp;*.tif;*.jpg;*.jpeg;*.tiff\0");
    }
    else if(filetype == f_settings){
        ofn.lpstrFilter = "Config files\0*.txt\0";
    }

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameA(&ofn)) return {}; // If no file is selected, return empty list

    filePath = ofn.lpstrFile;

    return filePath;
}