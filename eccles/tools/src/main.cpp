/*
    the main entry point to eccles-tts-packaging tool made by Nwobodo Ecclesiastes A.K.A
    igwe starking, this custom packaging tool scans eccles/resources dir in PIO and get
    all .wav file and pack them into a custom package format for runtime parsing
*/

//dependencies
#include "Eccles.h"
#include <iostream>
#include <cstring>

ECCLES_SYSTEM 

using namespace std;

constexpr e_string TAG = "eccles-tts-packaging-tool:";

//our program entry
ECCLES_API_ENTRY e_int eccles_main(e_uint count,char* params[]){
    //check if arguments are actually present
    if(count == 0){
        cerr<<TAG<<" no argument specified: usage eccles-tts-packager <type> <foldeer>"<<endl;
        return 1;
    }

    //reserved for a future dynamic-packing mode (see ModelType::DYNAMIC in AudioConfig.h),
    //not yet implemented — staticModel::pack() is the only packer that currently exists
    [[maybe_unused]] e_boolean dynamic = false;

    //loop through all args
    for(e_uint8 i = 0; i < count; i++){
        if(strcmp("-platformIO",params[i]) == 0){
            globalState.platformIO = true;
        } else if(strcmp("-type",params[i]) == 0){
            //peek at the next argument (the type value) without consuming it via a side
            //effect — params[i++] previously compared against "-type" itself (always
            //false) and silently skipped the next argument due to the extra increment
            if((e_uint)(i + 1) < count && strcmp("dynamic",params[i + 1]) == 0){
                dynamic = true;
            }
        } else if(string(params[i]).find(".txt") != string::npos){
            globalState.configPath = params[i];
        } else {
            //assume a folder
            globalState.folderName = params[i];
        }
    }

    if(globalState.platformIO){
        staticModel sm;
        return sm.pack() ? 0 : 1;
    }
    return 1;
}