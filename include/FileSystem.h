/*
  ECCLES FILE SYSTEM: this file controls the acces of LittleFS and SPIFF files
  it opens the file saved the opened file and read it in chunks it can also 
  read the entire file but we don't mostly do that here
*/

//dependencies
#ifndef ECCLES_FILE_SYSTEM
#define ECCLES_FILE_SYSTEM

/*
  because of the downgrade we couldn't use the system littlefs
  which leads us to use lorol/littlefs but this causes mount 
  issues, so we have no option but to use spiff
*/
#include<SPIFFS.h>
#include "EcclesTypes.h"

#define LittleFS SPIFFS //downgrade version

ECCLES_API {


  //holds the status result of file operations
  enum class FileStatus {
    SUCCESSFUL,
    ERROR,
    END_OF_FILE,
    BAD_ARG,
    MOUNT_FAILED,
    FILE_DOESNT_EXIST,
    BAD_MODE
  };

  //to save RAM this is the maximum amout of data we can read per chunk unless asked to read everything
  #define FILE_MAX_CHUNK_BUFFER 2048 //if we go above this we are litteraly killing the system

  //the main class that controls LittleFs files,NOTE: this class loads a single file
  //own it and all file operation happens on that file,this file also saves the file 
  //index so that when you read first buffer[40],it reads the start of file 0 first but
  //if you call read again with buff[40] it continues from file 40 from the previous read
  //and so on,any operation read,write,append happens on the file loaded with load to do
  //any operation on a new file you must call load on that file but NOTE: it unloads the 
  //previous file and if u need to file at a time u need to have two FileSystem object

  class FileSystem {
    private:
    e_string mode;
    static e_boolean mounted;
    FileStatus status = FileStatus::SUCCESSFUL; //holds the status internal operations

    //the file system is mounted only once and is not ideally thread safe here this variable
    //holds how many file system objects we have so we avoid multi-mounting the LittleFS file
    static e_uint8 u_count;

    
    File file;
    e_uint32 f_ptr;

    public:

    FileSystem();
    ~FileSystem();

    
    
    //loads a file for reading this is called once and every file operation happens to this file
    FileStatus load(e_string path,e_string mode);

    //reads the loaded file to the specified buffer specified and the specified size given
    FileStatus read(e_uint8* buffer,e_uint16 len) __attribute__((nonnull));

    //this reads a file from the provided offset and reads the amount provided in size
    FileStatus chunk(e_uint8* buffer,e_uint32 offset,e_uint16 size) __attribute__((nonnull));

    //this reads the entire file,though we don't recommend that here
    //NOTE: you must call getSize() which returns the size of the entire
    //file to find the size of this buffer
    FileStatus readAll(e_uint8* buffer,e_uint32 len)__attribute__((nonnull));

    //returns the size of the loaded file in byte
    e_uint32 getSize() __attribute__((always_inline));

    //write to the loaded file you must open with mode write "w" to be
    //able to do that otherwise the system return ERROR
    FileStatus write(e_uint8* buffer,e_uint32 len) __attribute__((nonnull));

    //append to an already written file,the file must first be loaded with mode
    //'a' otherwise the system will sent an error
    FileStatus append(e_uint8* buffer,e_uint32 len) __attribute__((nonnull));

    //this insert to a specific area in the file NOTE: this overwrite whatever is 
    //is written in the position we are inserting and does not move it
    //NOTE: insert changes the f_ptr cursor,so if the offset is 0,it insert at the 
    //postion stoped in the previous call
    FileStatus insert(e_uint8* buffer,e_uint32 offset,e_uint32 len) __attribute__((nonnull));

    //here we return a reference to the underlying file object for use in features not 
    //present here
    File& getFile();

    //checks if the specified file path exists in the filesystem
    e_boolean exists() const;

    //skips the len byte from the loaded file this also modifies the f_prt
    FileStatus skip(e_uint32 len);

    //close the currently loaded file, no file operation should be called after this
    //unless another file is loaded
    FileStatus unload();

    //returns text descprition of the file status
    static e_string getStatusText(FileStatus s);

    //operator allowing to check the validity of the file system object
    explicit operator bool() const;
  };
};
#endif