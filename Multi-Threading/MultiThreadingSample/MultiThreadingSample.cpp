// Copyright (c) 2017, Datalogics, Inc. All rights reserved.
//
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
//=====================================================================
// Sample: MultiSampleThreading - Demonstrates the use of the APDFL 
//                                library in a multi-threaded environment
//
//   This sample presents a framework, partially populated, to allow for
//   investigating Mult-Threading interactions in the APDFL library.
//=====================================================================



#include "Utilities.h"
#include "MTHeader.h"

#include "PDCalls.h"
#include "PSFCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"
#include "PDPageDrawM.h"
#include "DLExtrasCalls.h"


/* There will be an expandable collection of thread worker method classes that may be run. 
** Each class will have an initialization method, that will set it's unique variables, from a command line array.
** Each class will have a "worker" method that will be executed for it's process.
**
** The command line controls processing. It may be written on the command line, or stored in a file, and the name 
**   of the file specified as the only entry on the command line. The file default_Commands.txt lists all possible commands, 
**   and thier default values. One simple way to make a new test is to copy that file, remove everything not needed for your 
**   test, and modify the remaining options into what your test requires.

**  These options control the overall processing:
**
**  "LogFile=" Gives the name of a file to hold a progress log. If this is given, the default for "silent" will change to "false".
**             if omitted, output will default to stdout.
**
**  "StatisticsFile=" Gives the name of a file to hold a line of performance statistics. When present, we will to this line:
                    Input File Name, ThreadClass, APDFLVersion, 
                    Total Threads, ActiveThreads, Total Wall Time, Total CPU Time, Concurrency, Per Thread Avg Wall Time, Per Thread Avg CPU Time.
**
**  "TotalThreads=" gives the total number of threads to run. Default is 100 threads.
**                  While this value, like any other, maybe a list, only the first value will be used.
**
**  "ActiveThreads=" gives the number of threads to be run at a given time. Default is 5.
**              Note that this will interact with the following command (PauseEvery) such that the number of active threads may be less than
**              this on occassion. The design of the thread pump is such that we will note only one thread terminated, before starting a new
**              thread, so generally, "ActiveThreads" threads will be running at any given moment. Though some of those threads may have compeleted
**              working.
**
**              The value here may be a list. This allows the test designer to vary the number of working threads over time. The values in the list
**              will be used one by one, as threads are started, and the list will be cycled through when it's end is reached.
**
**  "PauseEvery=" will cause the process to allow the active threads to fall to zero every N threads. The default value is zero,'
**              This simulates a random start/stop environment, in which we will sometimes have no active threads. Whenthe value is zro,
**              we will stop pausing.
**
**              The value here may be a list, in which case we will pause afte the first number in the list, then resume and pause after the
**              second, and so on. We will cycle back to the start of the list when values are exhausted. Note that a zero in the list will
**              cause us to stop pausing.
**
**  "BaseInit=" may be true or false. If true, the library is initialized in the base thread. Default is true,
**              Where ever possible, applications should initialize and terminate the library on the "base" thread. 
**              This does two things. It puts the bulk of the initialize and terminate time into the base thread, allowing
**              worker threads to process more quickly, and it prevents the library from completly closing, and requiring a 
**              complete reopen. The "secondary" open of the library, required on each thread, will them be much faster. We know that
**              in some environments there is no conceptual "base thread" the hang this init/term on. So we permit applications that
**              do not have a base initialization. However, these will perform markedly worse than applications which do have a base thread. 
**              one saving grace here is that if there are simulataneous threads of the library running, only the first to start must do the compelete
**              open, and only the last to finish must do the compelete close. Only one thread at a time may be opening or closing the library!
**
**              This value is singular. If a list is supplied, only the first entry will be used.

**  "Processes=" is a command seperator list, enclosed in brackets, naming each process to run, in the order they
**              are to be run. There may be only one, or there may be many. A given process name can be included in the list
**              more than once. Threads will be started in the order given here, repeating as the list is exhausted.
**
**  "TempMemFileSys=" may be true or false. If true, set default temp file sys to ASMemFileSys at startup.
**
**              You may wish to use this option if a point of contention is access to a disc drive for storing temporary files.
**
**  "Silent=" may be true or false. If true, this silences messages written from the framework (Though not, neccessarily from worker threads).
**          this defaults to true if logfile is not used, and false if logfile is used. Primarily, you may want this set to true to deaden
**          extranious I/O operations while testing. 
**
**   The following paths may be set, to allow the test bed to use libraries or resources which are not positioned as expected 
**    (It is expected to be a memnber of the distribution library CPlusPlus directory).
**   APDFLPath                      Where to find the libraries, default is ../Binaries
**   ResourcesPath                  Where to find resources, default is ../../Resources
**...PluginPath                     Where to find plugins, default is APDFLPath.
**   ColorsPath                     Where to find color profiles, defaults to ..\\..\\Resources\\Color\\Profiles
**...UnicodePath                    Where to find the Unicode Directory, defaults to ../../Resources/Unicode
**   CMapsPath                      Where to find the CMaps Directory, defaults to ../../Resources/CMaps
**
** In studying mutli-threading behaviour, it is useful to lok at changes in behaviour as different memory allocators are used.
** For this reason, we can change the memory allocator used by setting a command line value for MemoryManger. The list of 
** valid memory managers is maintained in Utilities.h, with the code to effect the change in utilities.cpp.
**   MemoryManger                   Which memory manager should APDFL Use? (Currently "None" (default), "malloc", "tcmalloc" (unimplemented), and "rpmalloc")
**
**  Each type of worker may have a set of options specified for it.
**    These are specified as a comma seperated series of keyword/value pairs, inside a set of brackets. The names should
**    be in the form "ProcessNameOptions".
**
**  These options are common to all workers. They do not all need to be used for all workers:
**
**  "InFileName="  The name of a file used for input in the process. If the file does not exist, the run will fail!
**
**          This may be a list of names. The first thread run will use the first name, the second the second name, and so on, 
**          cycling back to the start of the list. This canbe used to create asyymetric behaviour in threads, so that some do more
**          work than others. 
**
**  "InFile2Name=".. The name of a secondary input file. This may be used in workers that require 2 input files.It behaves as 
**           InFileName above.
**
**  "InFile3Name=".. The name of a tertiary input file. This may be used in workers that require 3 input files.It behaves as
**           InFileName above.
**
**  "OutFilePath="  The name of an directory to hold output. If there directory does not exist, it will be created.
**                       if the path to the directory does not exist, it will not be created, and the run will fail!
**
**          This may be a list of names. They will be used in order, the first output to the first path, the second to the second, and so on.

**  "Silent="  may be true of false. If true, the worker will write no messages to the display. Default is true.
**
**  "LoadPlugins=" may be true of false. Default is per worker class (Workers[]).
**
**          If your thread does not need to use plugins, setting this option true can save some time and contention in the Init/Term logic. 
*/

/* This is the current set of known workers 
** To this list, define the new class in a new .cpp/.h file
**               Add the class name to the list of includes here
**               Add a field to the union WorkerClassPtr
**               Add a value to the enumertor in Worker.h (These must be a solid set of numbers,
**                  The last number must be "NumberOfWorkers".)
**               Add a class to the switch in OuterWorker()**
**               Create and initialize an instance of the worker class in the main procedure.

*/
#include "Worker.h"                 /* The base worker class */
#include "NonAPDFL_Worker.h"
#include "PDFA_Worker.h"
#include "PDFX_Worker.h"
#include "XPS2PDF_Worker.h"
#include "TextExtract_Worker.h"
#include "Rasterizer_Worker.h"
#include "Flattener_Worker.h"
#include "Access_Worker.h"
#include "RasterizeDoc_Worker.h"

typedef union workerclassptr
{
    NonAPDFLWorker      *NonAPDFL;
    PDFaWorker          *PDFa;
    PDFxWorker          *PDFx;
    XPS2PDFWorker       *XPS2PDF;
    TextextWorker       *TextExtract;
    RasterizerWorker    *Rasterizer;
    FlattenWorker       *Flattener;
    AccessWorker        *Access;
    RasterizeDocWorker  *RasterizeDoc;
} WorkerClassPtr;



WorkerType workers[NumberOfWorkers];
WorkerClassPtr workerClasses[NumberOfWorkers];


/* This procedure calls a worker thread of a specific type, 
** based on the GetWorkerClass method. 
**
** This must be updated for every new worker type!
*/
void callWorker (ThreadInfo *info)
{
    workerclass *baseObject = (workerclass *)(info->object);

    switch (baseObject->GetWorkerClass())
    {
    case NONAPDFL:
        ((NonAPDFLWorker *)baseObject)->WorkerThread (info);
        break;
    case PDFA:
        ((PDFaWorker *)baseObject)->WorkerThread (info);
        break;
    case PDFX:
        ((PDFxWorker *)baseObject)->WorkerThread (info);
        break;
    case XPS2PDF:
        ((XPS2PDFWorker *)baseObject)->WorkerThread (info);
        break;
    case TextExtract:
        ((TextextWorker *)baseObject)->WorkerThread (info);
        break;
    case Rasterizer:
        ((RasterizerWorker *)baseObject)->WorkerThread (info);
        break;
    case Flattener:
        ((FlattenWorker *)baseObject)->WorkerThread(info);
        break;
    case Access:
        ((AccessWorker *)baseObject)->WorkerThread (info);
        break;
    case RasterizeDoc:
        ((RasterizeDocWorker *)baseObject)->WorkerThread (info);
        break;
    default:
        baseObject->WorkerThread (info);
        break;
    }

    return;
}

/* This procedure is the one called by all threads!
**  it uses the workerclass object to create the library, and 
** collect startup information, then call the WorkerThread of the 
** approrpriate worker object to handle the bulk of the processing. 
** finally, it uses the workerclass to close the library, and collect 
** timing information.
*/
int outerWorker (ThreadInfo *info)
{
    workerclass *baseObject = (workerclass *)(info->object);
   

    baseObject->startThreadWorker (info);

    /* If anyone raises, for any reason,inside of a thread, and it is not caught in the thread,
    ** Catch it here. Do nothing about it, just make sure we execute the thread termination.
    */
    try         
    {
        if (info->noAPDFL)
            callWorker (info);
        else
        {
            DURING
                callWorker (info);
            HANDLER
            END_HANDLER
        }
    }
    catch (...) { };

    baseObject->endThreadWorker (info);

    return (info->result);
}

/* Some of the memory managers require initialization and termination.
**
** If the newly added manager does, add it to these routines
*/
void InitializeAllMemoryManagers ()
{
#ifdef WIN_ENV
    rpmalloc_master_initialize ();
#endif
}

void FinalizeAllMemoryManagers ()
{ 
#ifdef WIN_ENV
    rpmalloc_master_finalize ();
#endif
}

/* program takes the following command line attributes:
**      TotalThreads (Default to 100)       How many threads total whall we run
**      ActiveThreads (Default to 8)        Maximum threads started at any time
**      BaseInit (Defaults to false)        If true, init/term library on base thread. If false, do not.
**
**      Each worker thread may also define a command line attribute. It is in the form:
**          threadtype=[Attribute=value attribute=value ...]. 
**      It will be parsed into a dictionary in the thread, and used as attributes to control that particular thread.
**
** if the argument count is 2, and argument 1 is the name of a file, which can be opened and read, 
** argument 2 will be presumed to be a command file, in the form of the command line options, and will be opened, 
** read, and interpreted.
**
**/

int main(int argc, char** argv)
{
    int errCode = 0;                /* return code tho the user */

    /* Parse the comand line */
    attributes SampleAttributes = attributes (argc, argv);

    /* Establish a file to log output to */
    FILE *logFile = stdout;;
    bool logFileSet = false;
    valuelist  *list = SampleAttributes.GetKeyValue ("LogFile");
    if (list != NULL)
    {
        char *logFileName = list->value (0);
        logFile = fopen (logFileName, "w");
        logFileSet = true;
    }

    /* construct an array of threads to be run */
    int totalThreads = 100;
    if (SampleAttributes.IsKeyPresent ("TotalThreads"))
        totalThreads = SampleAttributes.GetKeyValueInt ("TotalThreads");

    int activeThreads = 5;
    if (SampleAttributes.IsKeyPresent ("ActiveThreads"))
        activeThreads = SampleAttributes.GetKeyValueInt ("ActiveThreads");


    /* Write some information about this run the log! */
    fprintf (logFile, "Running %01d threads, %01d at a time. Processes: [", totalThreads, activeThreads);
    valuelist *procs = SampleAttributes.GetKeyValue ("Processes");
    if (procs != NULL)
    {
        for (int index = 0; index < procs->size (); index++)
        {
            fprintf (logFile, "%s", procs->value (index));
            if ((index + 1) < procs->size ())
                fprintf (logFile, ", ");
            else
                fprintf (logFile, "]\n");
        }
    }
    else
        fprintf (logFile, "TextExtract].\n");

    int pauseEveryCount = 0;
    int *pauseEveryList = NULL;

    if (SampleAttributes.IsKeyPresent ("PauseEvery"))
    {
        valuelist *list = SampleAttributes.GetKeyValue ("PauseEvery");
        pauseEveryCount = list->size ();
        pauseEveryList = (int *)malloc (sizeof (int) * pauseEveryCount);
        for (int index = 0; index < pauseEveryCount; index++)
            pauseEveryList[index] = list->GetValueInt (index);
    }



    if (SampleAttributes.GetKeyValueBool ("BaseInit"))
        fprintf (logFile, "  We will initialize the library on the base thread.\n");
    else
        fprintf (logFile, "  We will NOT initialize the library on the base thread.\n");

    bool UseTempMemFileSys = SampleAttributes.GetKeyValueBool ("TempMemFileSys");
    if (UseTempMemFileSys)
        fprintf (logFile, "  We will use RamFileSys for temporary files.\n");
    else
        fprintf (logFile, "  We will NOT use RamFileSys for temporary files.\n");

    if (SampleAttributes.IsKeyPresent ("MemoryManager"))
        fprintf (logFile, "  We will use the Memory Manager %s.\n\n", SampleAttributes.GetKeyValue("MemoryManager")->value(0));
    else
        fprintf (logFile, "  We will use the Memory Manager %s.\n\n", "None");
    fflush (logFile);

    /* Before the first library is started, we must initialize any mameory managers we may wish to use
    */
    InitializeAllMemoryManagers ();

    /* If we are using a base thread library, start it now */
    APDFLib *baseInstance = NULL;
    if (SampleAttributes.GetKeyValueBool ("BaseInit"))
        baseInstance = new APDFLib (kPDFLInitPreferLocalFonts, &SampleAttributes);

    /* Construct the array of worker types 
    ** Set establish the options for each type*/
    workerClasses[NONAPDFL].NonAPDFL = new NonAPDFLWorker ();
    workerClasses[NONAPDFL].NonAPDFL->ParseOptions (&SampleAttributes, &workers[NONAPDFL]);

    workerClasses[PDFA].PDFa = new PDFaWorker ();
    workerClasses[PDFA].PDFa->ParseOptions (&SampleAttributes, &workers[PDFA]);

    workerClasses[PDFX].PDFx = new PDFxWorker ();
    workerClasses[PDFX].PDFx->ParseOptions (&SampleAttributes, &workers[PDFX]);

    workerClasses[XPS2PDF].XPS2PDF = new XPS2PDFWorker ();
    workerClasses[XPS2PDF].XPS2PDF->ParseOptions (&SampleAttributes, &workers[XPS2PDF]);

    workerClasses[TextExtract].TextExtract = new TextextWorker ();
    workerClasses[TextExtract].TextExtract->ParseOptions (&SampleAttributes, &workers[TextExtract]);

    workerClasses[Rasterizer].Rasterizer = new RasterizerWorker ();
    workerClasses[Rasterizer].Rasterizer->ParseOptions (&SampleAttributes, &workers[Rasterizer]);

    workerClasses[Flattener].Flattener = new FlattenWorker();
    workerClasses[Flattener].Flattener->ParseOptions (&SampleAttributes, &workers[Flattener]);

    workerClasses[Access].Access = new AccessWorker ();
    workerClasses[Access].Access->ParseOptions (&SampleAttributes, &workers[Access]);

    workerClasses[RasterizeDoc].RasterizeDoc = new RasterizeDocWorker ();
    workerClasses[RasterizeDoc].RasterizeDoc->ParseOptions (&SampleAttributes, &workers[RasterizeDoc]);


    /* This will be the list of threads to run */
    ThreadInfo *threads = (ThreadInfo *)malloc (sizeof (ThreadInfo) * totalThreads);

    /* We will alternate threads through the list of processes defined */
    int processes = 1;
    if (SampleAttributes.IsKeyPresent ("Processes"))
        processes = SampleAttributes.GetKeyValue ("Processes")->size ();

    WorkerClassPtr *workerList = (WorkerClassPtr*)malloc (processes * sizeof (WorkerClassPtr));
    int            *workerTypeList = (int *)malloc (processes * sizeof (int));
    workerList[0].PDFa = workerClasses[PDFA].PDFa;
    workerTypeList[0] = PDFA;

    if (SampleAttributes.IsKeyPresent ("Processes"))
    {
        valuelist *list = SampleAttributes.GetKeyValue ("Processes");
        for (int index = 0; index < processes; index++)
        {
            workerList[index].PDFa = NULL;
            char *processName = list->value (index);
            for (int y = 0; processName[y] != 0; y++)
                processName[y] = toupper (processName[y]);
            for (int x = 0; x < NumberOfWorkers; x++)
            {
                char workerName[1024];
                strcpy (workerName, workers[x].name);
                for (int y = 0; workerName[y] != 0; y++)
                    workerName[y] = toupper (workerName[y]);

                if (!strcmp (workerName, processName))
                {
                    workerList[index].PDFa = workerClasses[workers[x].type].PDFa;
                    workerTypeList[index] = workers[x].type;
                    break;
                }
            }
            if (workerList[index].PDFa == NULL)
            {
                fprintf (logFile, "There is no worker type \"%s\".\n", processName);
                exit (-1);
            }
        }
    }

    /* Now, "threads" contains a threadinfo structure for each thread we want to run, 
    ** and "workerList" contains a list of the workers we want to run, in the order we 
    ** want to run them. Populate these into the "threads" list, so each thread will know what 
    ** worker to use.
    */
    int type = 0;
    for (int index = 0; index < totalThreads; index++)
    {
        if (type >= processes)
            type = 0;
        memset ((char *)&threads[index], 0, sizeof (ThreadInfo));
        threads[index].threadNumber = index;
        threads[index].sequence = index / processes;
        threads[index].object = (void *)workerList[type].PDFa;
        threads[index].logFile = logFile;
        threads[index].logFileSet = logFileSet;
        threads[index].LoadPlugins = workers[workerTypeList[type]].LoadPlugins;
        threads[index].UseTempMemFileSys = UseTempMemFileSys;
        type++;
    }

    /* The "threads" table is now populated with the type or worker to run. We just need to 
    ** start "actualCount" threads, and each time a thread ends, start a new thread
    */
    int runningThreads = 0;             /* Number of threads currently active */
    int completedThreads = 0;           /* Number of threads currently completed */
    int startedThreads = 0;             /* Number of threads so far started */

    /* This is used by wait for many to wait for the next thread to finish */
    SDKThreadID *activeThreadArray = (SDKThreadID *)malloc (sizeof (SDKThreadID) * activeThreads);

    /* THis is used interna;y to identify the thread that finished 
    ** (So we don't have to search the list, looking for the the matching thread ID)
    */
    ThreadInfo **activeThreadInfo = (ThreadInfo **)malloc (sizeof (ThreadInfo *) * activeThreads);

    /* Accumulate percentage used */
    double percentageUsed = 0;

    /* This mechanism will allow the queue of active threads to fall to zero
    ** from time to time. If there is a single "pauseEvery" value, it will pause
    ** every N threads. If the pause entry is a list of values, it will pause after the 
    ** first number of threads, then againafter the next number, and so on
    */
    int pauseEveryIndex = 0;
    int pauseEvery = 0;
    if (pauseEveryList)
        pauseEvery = pauseEveryList[pauseEveryIndex];

    /* When this variable is true, we will drain the active queue before proceeding
    */
    bool pausing = false;

	/* Overall tiem for unix platforms */
#ifndef WIN_PLATFORM
	struct timeval  startTime, endTime;                 /* Used in Unix only, wall time started/stopped */
	clock_t         startCPU, endCPU;                   /* Used in Unix only to track CPU time used. */

	struct timezone zone;
	memset((char *)&zone, 0, sizeof(struct timezone));
	gettimeofday(startTime, &zone);
	startCPU = clock();
#endif

    /* This loop is the thread pump */
    while (completedThreads < totalThreads)
    {

        /* If we are paused, and there are no longer any running threads
        ** turn pause off, and reset PauseEvery from the input values.
        */
        if ((pausing) && (runningThreads == 0))
        {
            pausing = false;
            pauseEveryIndex = (pauseEveryIndex + 1) % pauseEveryCount;
            pauseEvery = pauseEveryList[pauseEveryIndex];
        }

        /* If we have less threads running than we want active, and we have not 
        ** already started all threads, start a thread!
        */
        if ((startedThreads < totalThreads) && (runningThreads < activeThreads) && (!pausing))
        {
            createThread (outerWorker, threads[startedThreads]);
            activeThreadArray[runningThreads] = threads[startedThreads].threadID;
            activeThreadInfo[runningThreads] = &threads[startedThreads];
            startedThreads++;
            runningThreads++;

            /* If pause every is zero, we will not pause.
            ** It will default to zero, or may be set there in an entry
            ** in the PauseEvery command. In the later case, it will stop
            ** pausing when this entry is seen.
            */
            if (pauseEvery)
                if (pauseEvery == 1)
                    pausing = true;
                else
                    pauseEvery--;

            continue;
        }

        /* If we get here, we have as many threads running as we can,.
        ** So wait for some to complete!
        */
        if (runningThreads)
        {
            /* Wait for the first in the list of threads to complete
            */
#ifdef WIN_PLATFORM
            ASInt32 index = WaitForAnyThreadComplete (activeThreadArray, runningThreads);
#else
            ASInt32 index = -1;
            while (1)
            {
                for (int x = 0; x < runningThreads; x++)
                {
                    if (activeThreadInfo[x]->threadCompleted)
                    {
                        index = x;
                        break;
                    }
                }
                if (index != -1)
                    break;
                usleep (1000);
            }
#endif

            /* A thread completed! */
            completedThreads++;

            /* If we want to "Do" anything with the thread that just finished, here is where we should 
            ** do it. activeThreadInfo[index] is a pointer to the threads ThreadInfo block
            */
            ThreadInfo *doneThread = activeThreadInfo[index];

            if (doneThread->result > errCode)
                errCode = doneThread->result;

#ifdef WIN_PLATFORM
            /* For windows, it is easier to collect thread info after the thread completes 
            ** The values are in FILETIME, which is nano seconds since 1/1/1601 (For some ofd reason), 
            ** They arested in two adjacent 32 bit integers, sequence such tht they can be considered a 
            ** single 64 bit integer 
            */
            FILETIME start, end, kernel, cpuTime;
            ASUns64 *start64 = (ASUns64*)&start, *end64 = (ASUns64 *)&end, *kernel64 = (ASUns64 *)&kernel, *cpu64 = (ASUns64 *)&cpuTime;
            GetThreadTimes (doneThread->threadID, &start, &end, &kernel, &cpuTime);
            end64[0] -= start64[0];
            cpu64[0] += kernel64[0];
            doneThread->wallTimeUsed = ((end64[0] * 1.0) / 10000000);
            doneThread->cpuTimeUsed = ((cpu64[0] * 1.0) / 10000000);
            doneThread->percentUtilized = (doneThread->cpuTimeUsed / doneThread->wallTimeUsed) * 100;
#endif

            percentageUsed += doneThread->percentUtilized;

            /* If we are not silent, then display a status for the thread completing */
            if (!doneThread->silent)
            {
                fprintf (doneThread->logFile, "Thread %01d completed in %0.6g seconds wall, %0.10g seconds CPU, with code %01d. -- %0.03g%% Utilized.\n",
                    doneThread->threadNumber + 1, doneThread->wallTimeUsed, doneThread->cpuTimeUsed, doneThread->result, doneThread->percentUtilized);
                fflush (doneThread->logFile);
            }

            /* end the thread */
            destroyThread (doneThread);

            /* If the thread to finish was NOT the last thread, then shift all of the arrays
            ** up to remove this thread from the array 
            */
            if (index < (runningThreads - 1))
            {
                for (int x = index; x < (runningThreads - 1); x++)
                {
                    activeThreadArray[x] = activeThreadArray[x + 1];
                    activeThreadInfo[x] = activeThreadInfo[x + 1];
                }
            }

            /* One less running thread */
            runningThreads--;

            continue;
        }

        /* We should never get here. Something went wrong in our counts!*/
        fprintf (logFile, "Something went wrong in our threading counts?\n We say we started %01d of %01d threads,"
                " completed %&01d, but have no threads active?\n", startedThreads, totalThreads, completedThreads);
        exit (-2);
    }

	double WallTimeUsed, CPUTimeUsed, Concurrency;
#ifdef WIN_PLATFORM
	FILETIME start, end, kernel, cpuTime;
	ASUns64 *start64 = (ASUns64*)&start, *end64 = (ASUns64 *)&end, *kernel64 = (ASUns64 *)&kernel, *cpu64 = (ASUns64 *)&cpuTime;

	HANDLE processHandle = GetCurrentProcess ();
	GetProcessTimes(processHandle, &start, &end, &kernel, &cpuTime);
	GetSystemTimeAsFileTime(&end);
	
	end64[0] -= start64[0];
	cpu64[0] += kernel64[0];
	WallTimeUsed = ((end64[0] * 1.0) / 10000000);
	CPUTimeUsed = ((cpu64[0] * 1.0) / 10000000);
#else
	gettimeofday (&endTime, &zone);
	endCPU = clock();
	WallTimeUsed = ((endTime.tv_sec - startTime.tv_sec) * 1.0) +
		((endTime.tv_usec - startTime.tv_usec) / 1000000);
	CPUTimeUsed = ((endCPU -startCPU) * 1.0) / CLOCKS_PER_SEC;
#endif

	Concurrency = (CPUTimeUsed / WallTimeUsed);
	fprintf(logFile, "\n\nTotal Wall time:%0.5g seconds.\nTotal CPU Time used %0.5g seconds.\nConcurrency %0.5g.\n ",
		WallTimeUsed, CPUTimeUsed, Concurrency);

	fprintf(logFile, "%01d Threads, %01d at a time. Each thread took %0.5g seconds CPU, and %0.5g seconds wall.\n",
		completedThreads, activeThreads, CPUTimeUsed / completedThreads, (double)(WallTimeUsed / (completedThreads * 1.0) * activeThreads));

    percentageUsed /= totalThreads;
    fprintf (logFile, "\n\n%0.5g%% of time used.\n", percentageUsed);

    if (SampleAttributes.IsKeyPresent ("StatisticsFile"))
    {
        valuelist  *list = SampleAttributes.GetKeyValue ("StatisticsFile");
        char *StatFileName = list->value (0);
        FILE *statFile = fopen (StatFileName, "a");

        char *processName;
        if (SampleAttributes.IsKeyPresent ("Processes"))
        {
            valuelist *list = SampleAttributes.GetKeyValue ("Processes");
            processName = list->value (0);
        }
        else
            processName = "PDFa";

        ASUns32 pdflVersion = PDFLGetVersion ();

        fprintf (statFile, "%s|%s|%01d.%01d.%01d|%01d|%01d|%0.5g|%0.5g|%0.5g|%0.5g|%0.5g\n",
                            argv[1], processName, pdflVersion >> 16, (pdflVersion << 16) >> 16, (pdflVersion << 24) >> 24,
                            completedThreads, activeThreads, WallTimeUsed, CPUTimeUsed, Concurrency,
                            (double)(WallTimeUsed / (completedThreads * 1.0) * activeThreads), CPUTimeUsed / completedThreads);
        fclose (statFile);
    }

    /* Shut down the working thread objects */
    for (int index = 0; index < NumberOfWorkers; index++)
        delete (workerClasses[index].NonAPDFL);

    /* If we are using a base thread library, stop it now */
    if (SampleAttributes.GetKeyValueBool ("BaseInit"))
        delete baseInstance;

    /* After all APDFL Libraries are closed, 
    ** Finalize all memory managers
    */
    FinalizeAllMemoryManagers ();


    /* If we built a pause Every List, free it */
    if (pauseEveryList)
        free (pauseEveryList);

    /* write error result to log file
    */
    fprintf (logFile, "The highest error code returned was %01d.\n", errCode);

    /* Close the logfile (If it was not stdout) */
    if (logFileSet)
        fclose (logFile);
    return errCode;
};
