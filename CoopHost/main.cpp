#include <iostream>
#include <vlc/vlc.h>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <fstream>


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


// Function to pause the video player
void pauseVideo(struct ctx* ctx) {
    libvlc_media_player_set_pause(ctx->mediaPlayer, 1);
    ctx->isPaused = true;
    ctx->isPlaying = false;
}

// Function to unpause and continue playing the video player
void unpauseVideo(struct ctx* ctx) {
    libvlc_media_player_set_pause(ctx->mediaPlayer, 0); // Set pause to 0 to unpause
    ctx->isPaused = false;
    ctx->isPlaying = true;
}

// Function to seek the video back by 15 seconds
void seekBackward(struct ctx* ctx) {
    libvlc_time_t currentTime = ctx->currentTime - 15000; // 15 seconds in milliseconds
    libvlc_media_player_set_time(ctx->mediaPlayer, currentTime);
}
void seekForward(struct ctx* ctx){
    libvlc_time_t currentTime = ctx->currentTime + 15000; // 15 seconds in milliseconds
    libvlc_media_player_set_time(ctx->mediaPlayer, currentTime);
}

int main() {
    string videoPath;
    cout << "Enter the path to the video: ";
    getline(cin, videoPath);

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

    // Start a thread to listen for keyboard input
    thread inputThread([&]() {
        int count = 0;
        bool flag =false;
        while (true) {
            flag =false;
            int ch = getch(); // Get a character from the terminal
            if (ch == ' ' && ctx->isPaused == false && count == 0) { // If space key is pressed and video is currently playing
                pauseVideo(ctx); // Pause the video player
                flag=true;
            }
            if (ch == ' ' && ctx->isPaused == true && count == 1) { // If space key is pressed and video is currently paused
                unpauseVideo(ctx); // Unpause the video player
                flag=true;
            }
            if (ch == KEY_LEFT) { // If left arrow key is pressed
                seekBackward(ctx); // Seek the video back by 15 seconds
                flag=true;
            }
            if (ch == KEY_RIGHT) { // If right arrow key is pressed
                seekForward(ctx); // Seek the video back by 15 seconds
                flag=true;
            }
            count += 1;
            if (count == 2) {
                count = 0;
            }
            if(flag){
                writeDataToFile(ctx);
            }
        }
    });

    // Start a thread to periodically print the current time and status
    thread statusThread([&]() {
        while (true) {
            // Print current status
            cout << "Current Time: " << ctx->currentTime << " ms, ";
            if (ctx->isPlaying)
                cout << "Status: Playing" << endl;
            else if (ctx->isPaused)
                cout << "Status: Paused" << endl;
            else
                cout << "Status: Unknown" << endl;

        }
    });

    inputThread.join(); // Wait for the input thread to finish
    statusThread.join(); // Wait for the status thread to finish

    libvlc_media_player_stop(mp);
    libvlc_media_player_release(mp);
    libvlc_release(inst);

    delete ctx; // Free the allocated memory

    endwin(); // End ncurses mode

    return 0;
}
