// Copyright (c) 2017, Datalogics, Inc. All rights reserved.
//
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
//=====================================================================
// This header file defines classs used in the multiThreading sample.

#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <vector>
#include <string>
#include <time.h>

#ifdef WIN_PLATFORM
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "InitializeLibrary.h"

/* These two classes are used to implement a command line parser, for the syntax:
**
**  Command Line = pair { space | pair}*
**  pair = name {=value}
**  name = any text character except space or "=".
**  value = {[} string {"," | spaces | string}* ]} \
**  string = any text character except "," o space
**
** So a command line may look, for instance, like:
**    TotalThreads=100 Threads=10 processes=[PDFX, PDFA,TextExtract]
**
** Keys and Values will always be stored upper cased, and tested upper cased.
*/

class valuelist
{
private:
    std::vector<char *> list;
public:
    valuelist (char *string)
    {
        /* trim any spaces off the start of the string */
        char *start = string;
        while (start[0] == ' ')
            start++;

        /* Check is this is a list, or a single entry
        ** (Presence of brackets)
        */
        if (start[0] == '[')
        {
            /* remove the array open bracket */
            start++;

            /* There is a multiple entry list here */
            /* cut off strings until we have them all */
            while ((start[0] != ']') && (start[0] != 0))
            {
                while (start[0] == ' ')
                    start++;
                if ((start[0] != ']') && (start[0] != 0))
                {
                    char *end = strstr (start, ",");
                    if (!end)
                        end = strstr (start, "]");
                    if (!end)
                        end = start + strlen (start);
                    int length = end - start;
                    char *saveValue = (char *)malloc (length + 1);
                    strncpy (saveValue, start, length);
                    saveValue[length] = 0;
                    list.push_back (saveValue);
                    start = end;
                    if (end[0] == ',')
                        start++;
                }
            }
        }
        else
        {
            /* This is a single entry list */
            char *saveValue = (char *)malloc (strlen (start) + 1);
            strcpy (saveValue, string);
            list.push_back (saveValue);
        }
    }

    ~valuelist ()
    {
        for (unsigned int count = 0; count < list.size (); count++)
            free (list[count]);
    }

    int size () { return list.size (); }

    char *value (int index) { return list[index]; }
};

class attributes
{
private:
    typedef std::map<std::string, valuelist *> attributeDict;
    attributeDict keys;
public:
    attributes (int argc, char **argv)
    {
        /* for each command line argument.
        ** NOTE: Command line arguments are delimted by spaces. so is you write a list
        ** value for an argument that includes spaces, put the argument in quotes!
        */
        for (int count = 0; count < argc; count++)
        {
            /* define attributes name and value.*/
            char *key = NULL, *value = NULL;


            if (count == 0)
            {
                /* Attribute zero is always the path of the
                ** executible being run.
                */
                key = (char *)malloc (12);
                strcpy (key, "ProcessPath");
                if (argv[0] == NULL)
                    continue;
                value = (char *)malloc (strlen (argv[0]) + 1);
                strcpy (value, argv[0]);
            }
            else
            {
                /* This can be just a key word, or a keyword and a value */
                char *equal = strstr (argv[count], "=");
                if (equal)
                {
                    /* Sperate out and store the key */
                    int keyLen = equal - argv[count];
                    key = (char *)malloc (keyLen + 1);
                    strncpy (key, argv[count], keyLen);
                    key[keyLen] = 0;

                    /* a value may be a single string, or is may be
                    ** an array of strings. If it is an array, it may have embedded
                    ** spaces. That would break up the string in the command parser.
                    **
                    ** check is the string is an array,and if that array completes
                    ** in this string. If it does not, append the following strings,
                    ** until we find the end of the array
                    */
                    value = (char *)malloc (strlen (&equal[1]) + 1);
                    strcpy (value, &equal[1]);
                    if (value[0] == '[')
                    {
                        int nest = 1;
                        /* To make this simplier, we are going to replace '[' with '}'
                        ** during the nest detection, we will convert themback after we
                        ** complete the string.
                        */
                        value[0] = '{';
                        while (nest)
                        {
                            /* find the end array marker,
                            ** allowing for nesting.
                            */
                            char *endarray = strstr (value, "]");
                            char *nestarray = strstr (&value[1], "[");

                            /* append following groups until we find a close array */
                            if ((!endarray) && ((count + 1) < argc))
                            {
                                value = (char *)realloc (value, strlen (value) + strlen (argv[count + 1]) + 1);
                                strcat (value, argv[count + 1]);
                                count++;
                                continue;
                            }

                            /* if we found an open array before the close array, remove is, and increment the
                            ** nest count by one
                            */
                            if ((nestarray) && (nestarray < endarray))
                            {
                                nest++;
                                nestarray[0] = '{';
                                continue;
                            }

                            /* If we found an end array, decriment the nest count */
                            if ((!nestarray) && (endarray))
                            {
                                nest--;
                                endarray[0] = '}';
                                continue;
                            }

                            if (count == argc - 1)
                            {
                                /* this is an unbalanced array !*/
                                printf ("The command line contained an unbalanced array of values!.\n");
                                exit (-1);
                            }
                        }

                        /* replace the braces with brackets */
                        char *brace = strstr (value, "{");
                        while (brace)
                        {
                            brace[0] = '[';
                            brace = strstr (value, "{");
                        }
                        brace = strstr (value, "}");
                        while (brace)
                        {
                            brace[0] = ']';
                            brace = strstr (value, "}");
                        }
                    }

                }
                else
                {
                    /* There is no value associated with this keyword */
                    key = (char *)malloc (strlen (argv[count]) + 1);
                    value = (char *)malloc (1);
                    value = NULL;
                    strcpy (key, argv[count]);
                }
            }
            for (int index = 0; key[index] != 0; index++)
                key[index] = toupper (key[index]);
            valuelist *values = new valuelist (value);
            AddKeyValue (key, values);
        }
    }

    ~attributes ()
    {
    }

    void AddKeyValue (char *key, valuelist *value)
    {
        std::string newKey(key);
        keys.insert ({ newKey, value });
    }

    valuelist *GetKeyValue (char *key)
    {
        char localKey[100];
        strcpy (localKey, key);
        for (int index = 0; localKey[index] != 0; index++)
            localKey[index] = toupper (localKey[index]);
       
        std::string actualKey (localKey);
        attributeDict::iterator found = keys.find (actualKey);
        if (found != keys.end())
        {
            return (found->second);
        }
        else
            return (NULL);
    }

    bool IsKeyPresent (char *key)
    {
        char localKey[100];
        strcpy (localKey, key);
        for (int index = 0; localKey[index] != 0; index++)
            localKey[index] = toupper (localKey[index]);

        std::string actualKey (localKey);
        attributeDict::iterator found = keys.find (actualKey);
        return (found != keys.end());
    }

    bool GetKeyValueBool (char *key, bool default)
    {
        if (IsKeyPresent (key))
        {
            valuelist *values = GetKeyValue (key);
            if (values->size () == 0)
                return default;
            char *value = values->value (0);
            for (int index = 0; value[index] != 0; index++)
                value[index] = toupper (value[index]);
            if (!_stricmp (value, "TRUE"))
                return true;
            if (!_stricmp (value, "FALSE"))
                return false;
            return default;
        }
        else
            return default;
    }

    int GetKeyValueInt (char *key)
    {
        if (IsKeyPresent (key))
        {
            valuelist *values = GetKeyValue (key);
            if (values->size () == 0)
                return 0;
            char *value = values->value (0);
            return (atoi (value));
        }
        else
            return 0;
    }
};






/* Device independent defininitions for 
** Multi Threading thread creation/deletion/Communication and synchronization
*/
#ifdef WIN_PLATFORM
#include "windows.h"
#include "process.h"

typedef HANDLE SDKThreadID;
#define ThreadFuncReturnType unsigned int WINAPI
typedef LPVOID ThreadFuncArgType;
typedef ThreadFuncReturnType ThreadFuncType (ThreadFuncArgType);
#define createThread( func, tinfo ) ((tinfo.threadID = (SDKThreadID)_beginthreadex( \
	NULL, 0, (ThreadFuncType *)(&func), &tinfo, 0, NULL)) != 0)
#define destroyThread( tinfo ) CloseHandle( tinfo.threadID )

#define WaitForAnyThreadComplete(list, size) \
    WaitForMultipleObjects (size, (HANDLE *)list, false, INFINITE) - WAIT_OBJECT_0;

        

typedef CRITICAL_SECTION CSMutex;
#define EnterCS( CSMutex ) EnterCriticalSection( &CSMutex )
#define LeaveCS( CSMutex ) LeaveCriticalSection( &CSMutex )
#define InitCS( CSMutex ) InitializeCriticalSection( &CSMutex )
#define DestroyCS( CSMutex ) DeleteCriticalSection( &CSMutex )

#else
#include <pthread.h>
#include <unistd.h>

typedef pthread_t SDKThreadID;
typedef void * ThreadFuncReturnType;
typedef void * ThreadFuncArgType;
typedef ThreadFuncReturnType ThreadFuncType (ThreadFuncArgType);
#define createThread( func, tinfo ) (pthread_create( &tinfo.threadID, NULL, (ThreadFuncType *)func, tinfo.threadArgs ) == 0)

// #define createThread( func, args, ptid ) (pthread_create( ptid, NULL, func, args ) == 0)
#define destroyThread( tinfo ) pthread_detach( tinfo.threadID )


typedef pthread_mutex_t *CSMutex;
#define InitCS( CSMutex ) do { \
	CSMutex = (pthread_mutex_t *)malloc( sizeof(pthread_mutex_t) ); \
	pthread_mutex_init( CSMutex, NULL ); \
	} while (0)

#define EnterCS( CSMutex ) pthread_mutex_lock( CSMutex )
#define LeaveCS( CSMutex ) pthread_mutex_unlock( CSMutex )
#define DestroyCS( CSMutex ) do { \
	pthread_mutex_destroy( CSMutex ); \
	free( CSMutex ); \
	} while (0)

#endif


/* Thread Communication */
class workerclass;

typedef struct
{
    ASInt32         threadNumber;
    SDKThreadID     threadID;
    void           *object;
    int             objectType;
    APDFLib        *instance;
    ASInt32         result; 
    time_t          startTime, endTime;
    clock_t         startCPU, endCPU;
    double          wallTimeUsed, cpuTimeUsed;
    bool            silent;
} ThreadInfo;

/* This is a base class for the worker threads
**   At this point, it provides for a guarenteed
** sequence number, for the sequence within this set of
** workers, and nothing else
*/
class workerclass
{
private:
    int         sequence;

    CSMutex     mutex;

public:
    /* Every worker thread needs an input and output file */
    char       *InFilePath;
    char       *InFileName;
    char       *InFileSuffix;
    char       *OutFilePath;
    bool        silent;
    attributes *threadAttributes;

    workerclass () { sequence = 0;
                     silent = true;
                     InitCS (mutex); 
                     InFilePath = InFileName = InFileSuffix = OutFilePath = NULL;
    }
    ~workerclass () { DestroyCS (mutex); 
                      if (InFilePath) free (InFilePath); 
                      if (InFileName) free (InFileName); 
                      if (InFileSuffix) free (InFileSuffix); 
                      if (OutFilePath) free (OutFilePath);
                      delete threadAttributes;
    }

    void WorkerThread (ThreadInfo *)
    {
        return;
    }

    int getNextSequence ()
    {
        int result;
        EnterCS (mutex);
        result = sequence++;
        LeaveCS (mutex);
        return result;
    }

    void startThreadWorker (ThreadInfo *info)
    {
#ifndef WIN_PLATFORM
        info->startTime = time (&info->startTime);
        info->startCPU = clock ();
#endif
        info->instance = new APDFLib ();
        info->silent = silent;
    }

    void endThreadWorker (ThreadInfo *info)
    {
        delete info->instance;

#ifndef WIN_PLATFORM
        info->endTime= time (&info->EndTime);
        info->endCPU = clock ();
        info->wallTimeUsed = ((endtime - startTime) * 1.0) / CLOCKS_PER_SEC;
        info->cpuTimeUsed = ((endCPU - startCPU) * 1.0) / CLOCKS_PER_SEC;
#endif

    }

#ifdef WIN_PLATFORM
#define PathSep '\\'
#define access _access
#else
#define PathSep  '/'
#endif


    void splitpath (char *path, char **toPath, char **filename, char **suffix)
    {
        static char pathCopy[2048];
        strcpy (pathCopy, path);
        char *finder = pathCopy + strlen (pathCopy) - 1;
        *suffix = *filename = *toPath = NULL;
        while (finder != pathCopy)
        {
            while ((finder > pathCopy) && (finder[0] != '.') & (finder[0] != PathSep))
                finder--;
            if (finder[0] == '.')
            {
                *suffix = finder + 1;
                finder[0] = 0;
                finder--;
                continue;
            }
            if (finder[0] == PathSep)
            {
                *filename = finder + 1;
                finder[0] = 0;
                finder--;
                *toPath = pathCopy;
            }
            break;
        }
    }

    void    parseCommonOptions (valuelist *values, char *defaultInFileName, char *defaultOutFilePath)
    {
        char *rawFileName;

        if (values)
        {
            /* Parse the attributes as is there were a command line */
            int listSize = values->size ();
            char **valueArray = (char **)malloc ((listSize + 1)* sizeof (char *));
            valueArray[0] = NULL;
            for (int count = 0; count < values->size (); count++)
                valueArray[count + 1] = values->value (count);
            threadAttributes = new attributes (listSize + 1, valueArray);
        }
        else
        {
            char *nullPtr = NULL;
            threadAttributes = new attributes (1, &nullPtr);
        }

        if (threadAttributes->IsKeyPresent ("InFileName"))
            rawFileName = threadAttributes->GetKeyValue ("InFileName")->value (0);
        else
            rawFileName = defaultInFileName;


        char *path, *name, *suffix;
        workerclass::splitpath (rawFileName, &path, &name, &suffix);

        InFilePath = (char *)malloc (strlen (path) + 1);
        strcpy (InFilePath, path);
        InFileName = (char *)malloc (strlen (name) + 1);
        strcpy (InFileName, name);
        InFileSuffix = (char *)malloc (strlen (suffix) + 1);
        strcpy (InFileSuffix, suffix);

        if (threadAttributes->IsKeyPresent ("OutFilePath"))
            rawFileName = threadAttributes->GetKeyValue ("OutFilePath")->value (0);
        else
            rawFileName = defaultOutFilePath;

        OutFilePath = (char *)malloc (strlen (rawFileName) + 1);
        strcpy (OutFilePath, rawFileName);

        if (threadAttributes->IsKeyPresent ("Silent"))
            silent = threadAttributes->GetKeyValueBool ("Silent", true);

        char workpath[2048];
        sprintf (workpath, "%s%c%s.%s", InFilePath, PathSep, InFileName, InFileSuffix);
        if (access (workpath, 04))
        {
            printf ("The input file does not exist? \n%   \"%s\"\n", workpath);
            exit (-1);
        }

        if (access (OutFilePath, 6))
        {
            _mkdir (OutFilePath);
            if (access (OutFilePath, 6))
            {
                printf ("The output path cannot be found or created!\n    \"%s\"\n", OutFilePath);
                exit (-1);
            }
        }
    }


};

