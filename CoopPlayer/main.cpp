#include <iostream>
#include <vlc/vlc.h>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <curl/curl.h>

using namespace std;

struct ctx {
    bool isPlaying;
    bool isPaused;
    libvlc_time_t currentTime;
    libvlc_media_player_t* mediaPlayer; // Add a reference to the media player
};

// VLC event callback function
static void event(const struct libvlc_event_t* event, void* data) {
    struct ctx* ctx = (struct ctx*)data;
    switch (event->type) {
    case libvlc_MediaPlayerTimeChanged:
        ctx->currentTime = event->u.media_player_time_changed.new_time;
        break;
    case libvlc_MediaPlayerPaused:
        ctx->isPaused = true;
        ctx->isPlaying = false;
        break;
    case libvlc_MediaPlayerPlaying:
        ctx->isPlaying = true;
        ctx->isPaused = false;
        break;
    default:
        break;
    }
}


libvlc_time_t convertToMilliseconds(const string& timeStr) {
    stringstream ss(timeStr);
    int milliseconds;

    ss >> milliseconds;

    // Convert to milliseconds
    libvlc_time_t totalTime = milliseconds;

    return totalTime;
}


// Function to write all server data to data.txt file
void writeServerDataToFile(const std::string& dataServerPath, const std::string& dataPath) {
    std::ifstream dataServerFile(dataServerPath);
    std::ofstream dataFile(dataPath);

    if (dataServerFile.is_open() && dataFile.is_open()) {
        std::string line;
        while (std::getline(dataServerFile, line)) {
            dataFile << line << std::endl;
        }
        std::cout << "Server data written to " << dataPath << " successfully." << std::endl;
        dataServerFile.close();
        dataFile.close();
    } else {
        std::cerr << "Error opening files: " << dataServerPath << " or " << dataPath << std::endl;
    }
}


// Function to compare the content of data.txt with dataServer.txt
bool compareDataToServer(const std::string& dataPath, const std::string& dataServerPath) {
    // Variables to store file contents
    std::string dataFromFile;
    std::string dataFromServer;

    // Read data from local file
    std::ifstream dataFile(dataPath);
    if (dataFile.is_open()) {
        // Read the entire file into dataFromFile
        dataFromFile.assign((std::istreambuf_iterator<char>(dataFile)), std::istreambuf_iterator<char>());
        dataFile.close();
    } else {
        std::cerr << "Error opening local data file: " << dataPath << std::endl;
        return false;
    }

    // Read data from server file
    std::ifstream serverFile(dataServerPath);
    if (serverFile.is_open()) {
        // Read the entire file into dataFromServer
        dataFromServer.assign((std::istreambuf_iterator<char>(serverFile)), std::istreambuf_iterator<char>());
        serverFile.close();
    } else {
        std::cerr << "Error opening server data file: " << dataServerPath << std::endl;
        return false;
    }

    // Compare the contents of the two strings
    if (dataFromFile == dataFromServer) {
        std::cout << "Files have the same content." << std::endl;
        return true;
    } else {
        std::cout << "Files have different content." << std::endl;
        return false;
    }
}

// Write all data to data.txt file
void writeDataToFile(const struct ctx* ctx) {
    ofstream outFile("data.txt"); // Open file in out mode (overwrite previous content)
    if (outFile.is_open()) {
        // Write data to the file
        outFile << ctx->currentTime << endl;
        outFile << ctx->isPlaying << endl;
        outFile << ctx->isPaused << endl;
        outFile.close();
    } else {
        cout << "Unable to open file!" << endl;
    }
}

// Callback function to write received data to a string
size_t writeToString(void* contents, size_t size, size_t nmemb, std::string* str) {
    str->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void WriteDataToServerFile(const std::string& serverLink, const std::string& filename) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string responseString;

        curl_easy_setopt(curl, CURLOPT_URL, serverLink.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            // Write the response string to the local file
            std::ofstream dataFile(filename);
            if (dataFile.is_open()) {
                dataFile << responseString;
                dataFile.close();
                std::cout << "Data written to " << filename << " successfully." << std::endl;
            } else {
                std::cerr << "Error opening file: " << filename << std::endl;
            }
        } else {
            std::cerr << "Error fetching data from server: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Error initializing cURL." << std::endl;
    }
}


// Function to pause the video player
void pauseVideo(struct ctx* ctx) {
    libvlc_media_player_set_pause(ctx->mediaPlayer, 1);
}
// Function to unpause and continue playing the video player
void unpauseVideo(struct ctx* ctx) {
    libvlc_media_player_set_pause(ctx->mediaPlayer, 0); // Set pause to 0 to unpause
}
// Function to seek the video curent time
void seekCurrent(struct ctx* ctx) {
    libvlc_time_t currentTime = ctx->currentTime; 
    libvlc_media_player_set_time(ctx->mediaPlayer, currentTime);
}

void updateCurrentMediaValues(struct ctx* ctx,const std::string& dataPath){
    cout<<"__________________________________Updated Values__________________________________"<<endl;
    ifstream file(dataPath);
    string tmpArray[3];

    // Read each line and store in tmpArray
    for (int i = 0; i < 3; ++i) {
        if (getline(file, tmpArray[i])) {
            // Remove any newline characters
            tmpArray[i].erase(tmpArray[i].find_last_not_of("\n") + 1);
        } else {
            // If there are fewer than 3 lines, break the loop
            break;
        }
    }
    
    bool tmpIsPaused=false;
    bool tmpIsPlaying=false;

    if(tmpArray[1]=="0"){
        tmpIsPlaying=false;
    }
    if(tmpArray[1]=="1"){
        tmpIsPlaying=true;
    }
    if(tmpArray[2]=="0"){
        tmpIsPaused=false;
    }
    if(tmpArray[2]=="1"){
        tmpIsPaused=true;
    }

    ctx->currentTime=convertToMilliseconds(tmpArray[0]);
    ctx->isPaused=tmpIsPaused;
    ctx->isPlaying=tmpIsPlaying;

    if(tmpIsPaused==true){
        pauseVideo(ctx);
    }
    if(tmpIsPlaying==true){
        unpauseVideo(ctx);
    }
    seekCurrent(ctx);

    // Close the file
    file.close();

    // Print tmpArray to verify
    for (int i = 0; i < 3; ++i) {
        cout << "tmpArray[" << i << "]: " << tmpArray[i] << endl;
    }


}

int main() {


    string videoPath;
    string serverLink = "https://69b0-185-222-94-252.ngrok-free.app/";
    string dataServerPath = "dataServer.txt";
    string dataPath = "data.txt";



    cout << "Enter the path to the video: ";
    getline(cin, videoPath);

    cout << "Enter the given link: ";
    getline(cin, serverLink);

    libvlc_instance_t* inst;
    libvlc_media_player_t* mp;
    libvlc_media_t* m;

    // Declare the ctx structure
    ctx* ctx = new struct ctx;
    ctx->isPlaying = false;
    ctx->isPaused = false;
    ctx->currentTime = 0;

    inst = libvlc_new(0, NULL);
    m = libvlc_media_new_path(inst, videoPath.c_str());
    mp = libvlc_media_player_new_from_media(m);
    libvlc_media_release(m);

    ctx->mediaPlayer = mp; // Save media player reference in ctx

    libvlc_event_manager_t* em = libvlc_media_player_event_manager(mp);
    libvlc_event_attach(em, libvlc_MediaPlayerTimeChanged, event, ctx);
    libvlc_event_attach(em, libvlc_MediaPlayerPaused, event, ctx);
    libvlc_event_attach(em, libvlc_MediaPlayerPlaying, event, ctx);

    libvlc_media_player_play(mp);

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // Enable keypad for arrow keys

    writeDataToFile(ctx);
    

    // Write all data from serverLink to data.txt 
    // If the previous data.txt is different from the new data gotten from server then update data.txt with newest values.
    //after setting up new values, use SetDataFromServer(ctx, serverLink); function.  
    thread statusUpdateThread([&]() {
        while(true){

        if(compareDataToServer(dataPath,dataServerPath)==false){
            writeServerDataToFile(dataServerPath,dataPath);
            updateCurrentMediaValues(ctx,dataPath);
            writeDataToFile(ctx);
        }
        WriteDataToServerFile(serverLink,dataServerPath);

    }});

    statusUpdateThread.join(); // Wait for the status thread to finish

    libvlc_media_player_stop(mp);
    libvlc_media_player_release(mp);
    libvlc_release(inst);

    delete ctx; // Free the allocated memory

    endwin(); // End ncurses mode

    return 0;
}
