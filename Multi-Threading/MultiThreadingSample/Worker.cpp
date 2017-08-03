/* Define the base worker class.
** This will do handling of options and actions common to all
** worker thread types.
*/

#include "Worker.h"

/* Initialiaze the object with static values.*/
workerclass::workerclass ()
{
    InFileCount = 0;
    InFile2Count = 0;
    InFile3Count = 0;
    OutPathCount = 0;
    silent = true;
    noAPDFL = false;
    InFilePath = InFileName = InFileSuffix = OutFilePath = NULL;
}

/* Free memory allocated for the objects option values */
workerclass::~workerclass ()
{
    for (int index = 0; index < InFileCount; index++)
    {
        free (InFilePath[index]); free (InFileName[index]); free (InFileSuffix[index]);
    }
    for (int index = 0; index < InFile2Count; index++)
    {
        free (InFile2Path[index]); free (InFile2Name[index]); free (InFile2Suffix[index]);
    }
    for (int index = 0; index < InFile3Count; index++)
    {
        free (InFile3Path[index]); free (InFile3Name[index]); free (InFile3Suffix[index]);
    }
    if (InFileCount)
    {
        free (InFilePath);
        free (InFileName);
        free (InFileSuffix);
    }
    if (InFile2Count)
    {
        free (InFile2Path);
        free (InFile2Name);
        free (InFile2Suffix);
    }
    if (InFile3Count)
    {
        free (InFile3Path);
        free (InFile3Name);
        free (InFile3Suffix);
    }
    for (int index = 0; index < OutPathCount; index++)
        free (OutFilePath[index]);
    if (OutPathCount) 
        free (OutFilePath);
    delete threadAttributes;
}

/* return the next name from the name list, 
** reduces modulo the size of the list
**
** The string returned must be free()ed.
*/
char *workerclass::GetInFileName (int threadSequence)
{
    char workname[2048], *result;
    int index = threadSequence % InFileCount;
    if (InFilePath[index][0])
        sprintf (workname, "%s%c%s.%s", InFilePath[index], PathSep, InFileName[index], InFileSuffix[index]);
    else
        sprintf (workname, "%s.%s", InFileName[index], InFileSuffix[index]);
    result = (char *)malloc (strlen (workname) + 1);
    strcpy (result, workname);
    return (result);
}

/* return the next name from the name 2 list,
** reduces modulo the size of the list
**
** The string returned must be free()ed.
*/
char *workerclass::GetInFile2Name (int threadSequence)
{
    if (InFile2Count == 0)
        return (NULL);

    char workname[2048], *result;
    int index = threadSequence % InFile2Count;
    if (InFile2Path[index][0])
        sprintf (workname, "%s%c%s.%s", InFile2Path[index], PathSep, InFile2Name[index], InFile2Suffix[index]);
    else
        sprintf (workname, "%s.%s", InFile2Name[index], InFile2Suffix[index]);
    result = (char *)malloc (strlen (workname) + 1);
    strcpy (result, workname);
    return (result);
}

/* return the next name from the name three list,
** reduces modulo the size of the list
**
** The string returned must be free()ed.
*/
char *workerclass::GetInFile3Name (int threadSequence)
{
    if (InFile3Count == 0)
        return (NULL);

    char workname[2048], *result;
    int index = threadSequence % InFile3Count;
    if (InFile3Path[index][0])
        sprintf (workname, "%s%c%s.%s", InFile3Path[index], PathSep, InFile3Name[index], InFile3Suffix[index]);
    else
        sprintf (workname, "%s.%s", InFile3Name[index], InFile3Suffix[index]);
    result = (char *)malloc (strlen (workname) + 1);
    strcpy (result, workname);
    return (result);
}

/* return an output name created from the the Nth
** reduced modulo list size) input one file name, and the
** Nth (reduced modulo list size) output directory.
**
** The string returned must be free()ed.
*/
char * workerclass::GetOutFileName (int threadSequence, int inner)
{
    int indexOut = threadSequence % OutPathCount;
    int indexIn = threadSequence % InFileCount;
    char workname[2048], *result;;
    if (inner == -1)
        sprintf (workname, "%s%c%s_%01d.%s", OutFilePath[indexOut], PathSep, InFileName[indexIn], threadSequence + 1, InFileSuffix[indexIn]);
    else
        sprintf (workname, "%s%c%s_%01d_%01d.%s", OutFilePath[indexOut], PathSep, InFileName[indexIn], threadSequence + 1, inner + 1, InFileSuffix[indexIn]);
    result = (char *)malloc (strlen (workname) + 1);
    strcpy (result, workname);
    return (result);
}


/* For non indows platforms, save start time. 
** For all platforms, initialize the APDFL library
** (if desired), and pass the worker type value for silent 
** in to the thread communication (Used by main process to silence
** or not silence messages about this thread there).
*/
void workerclass::startThreadWorker (ThreadInfo *info)
{
#ifndef WIN_PLATFORM
    struct timezone zone;
    memset ((char *)&zone, 0, sizeof (struct timezone));
    gettimeofday (&info->startTime, &zone);
    info->startCPU = clock ();
#endif
    if (noAPDFL)
    {
        info->instance = NULL;
        info->noAPDFL = true;
    }
    else
    {
        ASUns32 flags = kPDFLInitPreferLocalFonts;
        if (!info->LoadPlugins)
            flags |= kDontLoadPlugIns;
        info->instance = new APDFLib (flags, frameAttributes);
        if (info->UseTempMemFileSys)
            ASSetTempFileSys (ASGetRamFileSys ());
    }
    info->silent = silent;
}

/* End of each thread 
**
** If the library was started, terminate it.
** For non windows platforms, capture end time, and times used.
*/
void workerclass::endThreadWorker (ThreadInfo *info)
{
    if (info->instance)
        delete info->instance;

#ifndef WIN_PLATFORM
    struct timezone zone;
    memset ((char *)&zone, 0, sizeof (struct timezone));
    gettimeofday (&info->endTime, &zone);
    info->endCPU = clock ();
    info->wallTimeUsed = ((info->endTime.tv_sec - info->startTime.tv_sec) * 1.0) +
                         (((info->endTime.tv_usec - info->startTime.tv_usec) * 1.0) / 100000000);
    info->cpuTimeUsed = ((info->endCPU - info->startCPU) * 1.0) / CLOCKS_PER_SEC;
    info->percentUtilized = (info->cpuTimeUsed / info->wallTimeUsed) * 100;
#endif
    /* This is used by non windows thread pump to detect that a thread is complete */
    info->threadCompleted = true;
}

/* Utiltity routine to divide a file name into path, name, and suffix */
void workerclass::splitpath (char *path, char **toPath, char **filename, char **suffix)
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
            break;
        }
        *filename = finder;
        break;
    }
}

/* Utility to append the standard data path to a file name, if the
** Name begins with a %.
*/
char *workerclass::expandFileName (char *inname)
{
    char *result;
    if (inname[0] == '%')
    {
        result = (char *)malloc (strlen (inname) + strlen (inputPath) + 1);
        sprintf (result, "%s%c%s", inputPath, PathSep, &inname[1]);
    }
    else
    {
        result = (char *)malloc (strlen (inname) + 1);
        strcpy (result, inname);
    }
    return (result);
}

/* Convert the input string for a file name into 
** path,name, suffix, stored in the file name 1 array
*/
void workerclass::parseOneFileName (int index, char *inname)
{
    char *expandedName = expandFileName (inname);

    char *path, *name, *suffix;
    workerclass::splitpath (expandedName, &path, &name, &suffix);

    free (expandedName);

    if (path)
    {
        InFilePath[index] = (char *)malloc (strlen (path) + 1);
        strcpy (InFilePath[index], path);
    }
    else
    {
        InFilePath[index] = (char *)malloc (1);
        InFilePath[index][0] = 0;
    }

    InFileName[index] = (char *)malloc (strlen (name) + 1);
    strcpy (InFileName[index], name);
    InFileSuffix[index] = (char *)malloc (strlen (suffix) + 1);
    strcpy (InFileSuffix[index], suffix);
}

/* Convert the input string for a file name into
** path,name, suffix, stored in the file name 2 array
*/
void workerclass::parseTwoFileName (int index, char *inname)
{
    char *expandedName = expandFileName (inname);

    char *path, *name, *suffix;
    workerclass::splitpath (expandedName, &path, &name, &suffix);

    free (expandedName);

    InFile2Path[index] = (char *)malloc (strlen (path) + 1);
    strcpy (InFile2Path[index], path);
    InFile2Name[index] = (char *)malloc (strlen (name) + 1);
    strcpy (InFile2Name[index], name);
    InFile2Suffix[index] = (char *)malloc (strlen (suffix) + 1);
    strcpy (InFile2Suffix[index], suffix);
}

/* Convert the input string for a file name into
** path,name, suffix, stored in the file name 3 array
*/
void workerclass::parseThreeFileName (int index, char *inname)
{
    char *expandedName = expandFileName (inname);

    char *path, *name, *suffix;
    workerclass::splitpath (expandedName, &path, &name, &suffix);

    free (expandedName);

    InFile3Path[index] = (char *)malloc (strlen (path) + 1);
    strcpy (InFilePath[index], path);
    InFile3Name[index] = (char *)malloc (strlen (name) + 1);
    strcpy (InFile3Name[index], name);
    InFile3Suffix[index] = (char *)malloc (strlen (suffix) + 1);
    strcpy (InFile3Suffix[index], suffix);
}

/* Obtain the worker options from the frawework options,
** Set values for all of the options common to all workers
*/
void    workerclass::ParseOptions (attributes *FrameAttributes, char *defaultInFileName, char *defaultOutFilePath)
{
    /* Save the frame attribute dictionary to pass into PDFLInit */
    frameAttributes = FrameAttributes;

    /* Get the worker options, if there are any */
    valuelist *values = FrameAttributes->GetKeyValue (WorkerIDEntry->paramName);

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
        /* Create an empty options dictionary, if there are no options for this worker
        */
        char *nullPtr = NULL;
        threadAttributes = new attributes (1, &nullPtr);
    }

    /* Obtain the input file name (or list of names.
    ** Split each name into a path, a name, and a suffix
    */
    if (threadAttributes->IsKeyPresent ("InFileName"))
    {
        valuelist *nameValues = threadAttributes->GetKeyValue ("InFileName");
        InFileCount = nameValues->size ();
        InFilePath = (char **)malloc (sizeof (char*) * InFileCount);
        InFileName = (char **)malloc (sizeof (char*) * InFileCount);
        InFileSuffix = (char **)malloc (sizeof (char*) * InFileCount);

        for (int index = 0; index < InFileCount; index++)
            parseOneFileName (index, nameValues->value (index));
    }
    else
    {
        InFileCount = 1;
        InFilePath = (char **)malloc (sizeof (char*));
        InFileName = (char **)malloc (sizeof (char*));
        InFileSuffix = (char **)malloc (sizeof (char*));
        parseOneFileName (0, defaultInFileName);
    }

    /* Do the same for Input 2 file names */
    if (threadAttributes->IsKeyPresent ("InFile2Name"))
    {
        valuelist *nameValues = threadAttributes->GetKeyValue ("InFile2Name");
        InFile2Count = nameValues->size ();
        InFile2Path = (char **)malloc (sizeof (char*) * InFile2Count);
        InFile2Name = (char **)malloc (sizeof (char*) * InFile2Count);
        InFile2Suffix = (char **)malloc (sizeof (char*) * InFile2Count);

        for (int index = 0; index < InFileCount; index++)
            parseTwoFileName (index, nameValues->value (index));
    }

    /* And again for input 3 file names */
    if (threadAttributes->IsKeyPresent ("InFile3Name"))
    {
        valuelist *nameValues = threadAttributes->GetKeyValue ("InFile3Name");
        InFile3Count = nameValues->size ();
        InFile3Path = (char **)malloc (sizeof (char*) * InFile3Count);
        InFile3Name = (char **)malloc (sizeof (char*) * InFile3Count);
        InFile3Suffix = (char **)malloc (sizeof (char*) * InFile3Count);

        for (int index = 0; index < InFileCount; index++)
            parseThreeFileName (index, nameValues->value (index));
    }

    /* Obtain the list of output directory names
    */
    if (threadAttributes->IsKeyPresent ("OutFilePath"))
    {
        valuelist *nameValues = threadAttributes->GetKeyValue ("OutFilePath");
        OutPathCount = nameValues->size ();
        OutFilePath = (char **)malloc (sizeof (char *) * OutPathCount);
        for (int index = 0; index < OutPathCount; index++)
        {
            char *value = nameValues->value (index);
            OutFilePath[index] = (char *)malloc (strlen (value) + 1);
            strcpy (OutFilePath[index], value);
        }
    }
    else
    {
        OutFilePath = (char **)malloc (sizeof (char *));
        OutPathCount = 1;
        OutFilePath[0] = (char *)malloc (strlen (defaultOutFilePath) + 1);
        strcpy (OutFilePath[0], defaultOutFilePath);
    }

    /* All threads accept silent as an option.
    ** The default is true if there is no logfile, and
    ** false if there is a logfile
    */
    silent = true;
    if (FrameAttributes->IsKeyPresent ("LogFile"))
        silent = false;
    if (threadAttributes->IsKeyPresent ("Silent"))
        silent = threadAttributes->GetKeyValueBool ("Silent");

    /* All threads accept NoAPDFL as an option.
    ** Some threads may wish to override this to false, as the designers
    ** may know that APDFL initialization in the threads is 100% required
    */
    if (threadAttributes->IsKeyPresent ("NoAPDFL"))
        noAPDFL = threadAttributes->GetKeyValueBool ("NoAPDFL");

    /* All threads accept LoadPlugins as an option.
    ** Some threads may wish to override this to true, as the designers
    ** may know that Plugin initialization in the threads is 100% required
    */
    if (threadAttributes->IsKeyPresent ("LoadPlugins"))
        WorkerIDEntry->LoadPlugins = threadAttributes->GetKeyValueBool ("LoadPlugins");

    /* Validate that every intput file name exists, and is readable
    ** Fail if thie is not true!
    */
    char workpath[2048];
    for (int index = 0; index < InFileCount; index++)
    {
        if (InFilePath[index][0])
            sprintf (workpath, "%s%c%s.%s", InFilePath[index], PathSep, InFileName[index], InFileSuffix[index]);
        else
            sprintf (workpath, "%s.%s", InFileName[index], InFileSuffix[index]);
        if (access (workpath, 04))
        {
            printf ("The input file does not exist? \n%   \"%s\"\n", workpath);
            exit (-1);
        }
    }

    for (int index = 0; index < InFile2Count; index++)
    {
        if (InFile2Path[index][0])
            sprintf (workpath, "%s%c%s.%s", InFile2Path[index], PathSep, InFile2Name[index], InFile2Suffix[index]);
        else
            sprintf (workpath, "%s.%s", InFile2Name[index], InFile2Suffix[index]);
        if (access (workpath, 04))
        {
            printf ("The input file does not exist? \n%   \"%s\"\n", workpath);
            exit (-1);
        }
    }

    for (int index = 0; index < InFile3Count; index++)
    {
        if (InFile3Path[index][0])
            sprintf (workpath, "%s%c%s.%s", InFile3Path[index], PathSep, InFile3Name[index], InFile3Suffix[index]);
        else
            sprintf (workpath, "%s.%s", InFile3Name[index], InFile3Suffix[index]);
        if (access (workpath, 04))
        {
            printf ("The input file does not exist? \n%   \"%s\"\n", workpath);
            exit (-1);
        }
    }

    /* Validate that every output directory exists, and is writable
    ** create the directory if it doesnot exist. Creating the complete
    ** path will not be done
    ** If the path does not exist, and cannot be created, or accessed,
    ** fail the run.
    */
    for (int index = 0; index < OutPathCount; index++)
    {
        if (access (OutFilePath[index], 6))
        {
#ifdef WIN_PLATFORM
            mkdir (OutFilePath[index]);
#else
            mkdir (OutFilePath[index], 0777);
#endif
            if (access (OutFilePath[index], 6))
            {
                printf ("The output path cannot be found or created!\n    \"%s\"\n", OutFilePath);
                exit (-1);
            }
        }
    }
}
