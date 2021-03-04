//Erhan Yalniz - 150117905
#include <stdio.h>
#include <stdlib.h> // Comparing tags represented with bytes (chars).
#include <string.h> // Parsing arguments.
#include <math.h> // 2^x (calculating number of sets), log2(x) (getting block offset)

#define RAM_FILE "ram.txt"

typedef struct {
    char* tag;
    int time;
    int v;
    char* data;
} line;

int L1s, L2s; // Set Index Bits
int L1E, L2E; // Number of Lines per Set
int  L1b, L2b; // Block Size
char* traceFile; // Trace file to test simulated cache.

// Dimensions in order of: Cache, Set, Line.
line** L1I;
line** L1D;
line** L2;

// Values related to Caches:
int numberOfSetsL1;
int blockBytesL1;
int tagBytesL1;
int numberOfSetsL2;
int tagBytesL2;
int blockBytesL2;

// Values related to Simulator:

int timeCounter;

int L1Ihits;
int L1Imisses;
int L1Ievictions;

int L1Dhits;
int L1Dmisses;
int L1Devictions;

int L2hits;
int L2misses;
int L2evictions;

// Parse arguments passed in.
void parseArguments(int argc, char* argv []);

// Print program usage.
void printUsage();

// Initialize caches to given size.
void initializeCaches();

//RAM basic operations:
char* readRAM(int operationAddress, int size);

void writeRAM(int operationAddress, int size, char* data);

//Cache basic operations:

void writeCache(line** Lx, int operationAddress, int size, char* data);

//Trace instructions:

char* loadInstruction(int operationAddress, int size);

char* loadData(int operationAddress, int size);

void storeData(int operationAddress, int size, char* data);

void modifyData(int operationAddress, int size, char* data);

// Execute trace file.
void executeTrace(char* filename);

int main(int argc, char* argv[]) {
    // Parse arguments passed in.
    parseArguments(argc, argv);

    // Debug section.
    printf("L1s: %d L1E: %d L1b: %d\n", L1s, L1E, L1b);
    printf("L2s: %d L2E: %d L2b: %d\n", L2s, L2E, L2b);
    printf("Trace File: %s\n", traceFile);

    // Allocate caches and initialize all to be empty.
    initializeCaches();

    // Execute given trace file.
    executeTrace(traceFile);

    // Show results.
    printf("L1I-hits:%d L1I-misses:%d L1I-evictions:%d\n", L1Ihits, L1Imisses, L1Ievictions);
    printf("L1D-hits:%d L1D-misses:%d L1D-evictions:%d\n", L1Dhits, L1Dmisses, L1Devictions);
    printf("L2-hits:%d L2-misses:%d L2-evictions:%d\n", L2hits, L2misses, L2evictions);

    return 0;
}
	
void parseArguments(int argc, char* argv []) {
    // Parse arguments passed in.
    int i;
    for(i = 1; i < argc; i+=2) {
        if(strlen(argv[i]) == 4) {
            if(argv[i][2] == '1') {
                if(argv[i][3] == 's') {
                    L1s = atoi(argv[i+1]);
                }
                if(argv[i][3] == 'E') {
                    L1E = atoi(argv[i+1]);
                }
                if(argv[i][3] == 'b') {
                    L1b = atoi(argv[i+1]);
                }
            }
            else if(argv[i][2] == '2') {
                if(argv[i][3] == 's') {
                    L2s = atoi(argv[i+1]);
                }
                if(argv[i][3] == 'E') {
                    L2E = atoi(argv[i+1]);
                }
                if(argv[i][3] == 'b') {
                    L2b = atoi(argv[i+1]);
                }
            }
        }
        else if(strcmp("-t", argv[i]) == 0) {
            traceFile = (char*) malloc(strlen(argv[i+1])+1);
            strcpy(traceFile, argv[i+1]);
        }
    }
}

void printUsage() {
    printf("Usage: ./CacheSimulator -L1s <L1s> -L1E <L1E> -L1b <L1b>\n");
    printf("-L2s <L2s> -L2E <L2E> -L2b <L2b>\n");
    printf("-t <tracefile>");
}

void initializeCaches() {
    int i, j;

    // Calculate needed values for caches:
    numberOfSetsL1 = pow(2, L1s);
    blockBytesL1 = pow(2, L1b);
    tagBytesL1 = ceil((sizeof(int) * 8 - L1s - L1b)/8); // Calculate remaining bytes to tag.
    // If the tag bits cannot be represented with bytes ceil will round up to nearest byte multiples; because we will store tag using chars.
    numberOfSetsL2 = pow(2, L2s);
    blockBytesL2 = pow(2, L2b);
    tagBytesL2 = ceil((sizeof(int) * 8 - L2s - L2b)/8); // Calculate remaining bytes to tag.
    // If the tag bits cannot be represented with bytes ceil will round up to nearest byte multiples; because we will store tag using chars.

    // Allocate sets.
    L1I = (line**) malloc(sizeof(line*) * numberOfSetsL1);
    L1D = (line**) malloc(sizeof(line*) * numberOfSetsL1);
    L2 = (line**) malloc(sizeof(line*) * numberOfSetsL2);

    // Allocate blocks (lines) in L1I and L1D.
    for(i = 0; i < numberOfSetsL1; i++) {
        // Allocate space for all blocks (lines) in current set.
        L1I[i] = (line*) malloc(sizeof(line) * L1E);
        L1D[i] = (line*) malloc(sizeof(line) * L1E);

        // Initialize all blocks (lines) of current set both for L1I and L1D. Initialize all blocks data and tag to 0 as well. (calloc)
        // Last byte to terminate with null.
        for(j = 0; j < L1E; j++) {
            L1I[i][j].tag = (char*) calloc(tagBytesL1*2+1, 1);
            L1I[i][j].time = 0;
            L1I[i][j].v = 0;
            L1I[i][j].data = (char*) calloc(blockBytesL1*2+1, 1);

            L1D[i][j].tag = (char*) calloc(tagBytesL1*2+1, 1);
            L1D[i][j].time = 0;
            L1D[i][j].v = 0;
            L1D[i][j].data = (char*) calloc(blockBytesL1*2+1, 1);
        }
    }

    // Allocate blocks (lines) in L2.
    for(i = 0; i < numberOfSetsL2; i++) {
        // Allocate space for all blocks (lines) in current set.
        L2[i] = (line*) malloc(sizeof(line) * L2E);

        // Initialize all blocks (lines) of current set. Initialize all blocks data and tag to 0 as well. (calloc)
        for(j = 0; j < L1E; j++) {
            L2[i][j].tag = (char*) calloc(tagBytesL2*2+1, 1);
            L2[i][j].time = 0;
            L2[i][j].v = 0;
            L2[i][j].data = (char*) calloc(blockBytesL2*2+1, 1);
        }
    }
}

//RAM basic operations:
char* readRAM(int operationAddress, int size) {
    int i;
    FILE* ramFile = fopen(RAM_FILE, "r");
    char* data = (char*) calloc(size*2+1, 1);
    char buffer[3] = "\0";

    // Skip to the operation address.
    fseek(ramFile, operationAddress*3, SEEK_SET);

    // Read each bytes one by one.
    for(i = 0; i < size; i++){
        fscanf(ramFile, "%s", buffer);
        strcat(data, buffer);
    }

    fclose(ramFile);
    return data;
}

void writeRAM(int operationAddress, int size, char* data) {
    int i;
    FILE* ramFile = fopen(RAM_FILE, "r+");

    // Skip to the operation address.
    fseek(ramFile, operationAddress*3, SEEK_SET);

    // Write each bytes one by one.
    for(i = 0; i < size; i++){
        fprintf(ramFile, "%c%c ", data[i*2], data[i*2+1]);
    }

    fclose(ramFile);
}

//Cache basic operations:

void writeCache(line** Lx, int operationAddress, int size, char* data) {
    int i, setIndex, tag, min = 0, setNumber;
    char* tagStr;

    if(Lx == L1I) {
        setIndex = (operationAddress >> (L1b)) & (numberOfSetsL1 - 1);
        tag = operationAddress >> (L1b + L1s);
        setNumber = L1E;
    }
    else if(Lx == L1D) {
        setIndex = (operationAddress >> (L1b)) & (numberOfSetsL1 - 1);
        tag = operationAddress >> (L1b + L1s);
        setNumber = L1E;
    }
    else if(Lx == L2) {
        setIndex = (operationAddress >> (L2b)) & (numberOfSetsL2 - 1);
        tag = operationAddress >> (L2b + L2s);
        setNumber = L2E;
    }

    tagStr = (char*) malloc(tagBytesL1*2+1);
    sprintf(tagStr, "%x", tag);
    // Search for a not-valid block. (empty line)
    for(i = 0; i < setNumber; i++) {
        if(Lx[setIndex][i].v == 0) {        
            strcpy(Lx[setIndex][i].tag, tagStr);
            Lx[setIndex][i].time = timeCounter;
            Lx[setIndex][i].v = 1;
            strcpy(Lx[setIndex][i].data, data);
            // No need to evict already written to cache.
            return;
        }
    }
        
    // Evict with FIFO policy.
        
    // First find the line with minimum time.
    for(i=1; i < setNumber; i++) {
        if(Lx[setIndex][min].time > Lx[setIndex][i].time) {
            min = i;
        }
    }
    
    // Overwrite the old cache line with new data.
    strcpy(Lx[setIndex][min].tag, tagStr);
    Lx[setIndex][min].time = timeCounter;
    Lx[setIndex][min].v = 1;
    strcpy(Lx[setIndex][min].data, data);

    if(Lx == L1I) {
        L1Ievictions++;
    }
    if(Lx == L1D) {
        L1Devictions++;
    }
    if(Lx == L2) {
        L2evictions++;
    }
}

//Trace instructions:
char* loadInstruction(int operationAddress, int size) {
    int i, buffer;
    
    int hitL1I = 0, hitL2 = 0;
    char* result = (char*) malloc(size*2+1);

    int setIndex = (operationAddress >> (L1b)) & (numberOfSetsL1 - 1);
    int tag = operationAddress >> (L1b + L1s);

    // Search inside cache L1I in set with index "setIndex" for tag hits.
    for(i = 0; i < L1E; i++){
        if(L1I[setIndex][i].v == 0) {
            continue;
        }
        buffer = strtol(L1I[setIndex][i].tag, NULL, 16);
        if(buffer == tag) {
            hitL1I = 1;
            strcpy(result, L1I[setIndex][i].data);
            break;
        }
    }

    setIndex = (operationAddress >> (L2b)) & (numberOfSetsL2 - 1);
    tag = operationAddress >> (L2b + L2s);

    // Search inside cache L2 in set with index "setIndex" for tag hits.
    for(i = 0; i < L1E; i++){
        if(L2[setIndex][i].v == 0) {
            continue;
        }
        buffer = strtol(L2[setIndex][i].tag, NULL, 16);
        if(buffer == tag) {
            hitL2 = 1;
            strcpy(result, L2[setIndex][i].data);
            break;
        }
    }

    timeCounter++; // Instruction complete increase global time.

    // Immediately load from ram to corresponding cache/s if missed.
    if(!hitL1I) {
        result = readRAM(operationAddress, size);
        writeCache(L1I, operationAddress, size, result);
    }
    if(!hitL2) {
        result = readRAM(operationAddress, size);
        writeCache(L2, operationAddress, size, result);
    }

    // Increment hit/miss global counters.
    L1Ihits += hitL1I;
    L1Imisses += hitL1I ? 0 : 1;
    L2hits += hitL2;
    L2misses += hitL2 ? 0 : 1;

    // Only give the size amount of bytes. (2 hexadecimals)
    strncpy(result, result, size*2);
    result[size*2+1] = '\0';
    return result;
}

char* loadData(int operationAddress, int size) {
    int i, buffer;
    
    int hitL1D = 0, hitL2 = 0;
    char* result = (char*) malloc(size*2+1);

    int setIndex = (operationAddress >> (L1b)) & (numberOfSetsL1 - 1);
    int tag = operationAddress >> (L1b + L1s);

    // Search inside cache L1D in set with index "setIndex" for tag hits.
    for(i = 0; i < L1E; i++){
        if(L1D[setIndex][i].v == 0) {
            continue;
        }
        buffer = strtol(L1D[setIndex][i].tag, NULL, 16);
        if(buffer == tag) {
            hitL1D = 1;
            strcpy(result, L1D[setIndex][i].data);
            break;
        }
    }

    setIndex = (operationAddress >> (L2b)) & (numberOfSetsL2 - 1);
    tag = operationAddress >> (L2b + L2s);

    // Search inside cache L2 in set with index "setIndex" for tag hits.
    for(i = 0; i < L1E; i++){
        if(L2[setIndex][i].v == 0) {
            continue;
        }
        buffer = strtol(L2[setIndex][i].tag, NULL, 16);
        if(buffer == tag) {
            hitL2 = 1;
            strcpy(result, L2[setIndex][i].data);
            break;
        }
    }

    timeCounter++; // Instruction complete increase global time.

    // Immediately load from ram (newly stored value) to corresponding cache/s if missed.
    if(!hitL1D) {
        result = readRAM(operationAddress, size);
        writeCache(L1D, operationAddress, size, result);
    }
    if(!hitL2) {
        result = readRAM(operationAddress, size);
        writeCache(L2, operationAddress, size, result);
    }

    // Increment hit/miss global counters.
    L1Dhits += hitL1D;
    L1Dmisses += hitL1D ? 0 : 1;
    L2hits += hitL2;
    L2misses += hitL2 ? 0 : 1;

    // Only give the size amount of bytes. (2 hexadecimals)
    strncpy(result, result, size*2);
    result[size*2+1] = '\0';
    return result;
}

void storeData(int operationAddress, int size, char* data) {
    int i, buffer;
    
    int hitL1D = 0, hitL2 = 0;

    int setIndex = (operationAddress >> (L1b)) & (numberOfSetsL1 - 1);
    int tag = operationAddress >> (L1b + L1s);

    // Search inside cache L1D in set with index "setIndex" for tag hits.
    for(i = 0; i < L1E; i++){
        if(L1D[setIndex][i].v == 0) {
            continue;
        }
        buffer = strtol(L1D[setIndex][i].tag, NULL, 16);
        if(buffer == tag) {
            hitL1D = 1;
            // Overwrite corresponding hexademicals.
            strncpy(L1D[setIndex][i].data, data, size*2);
            break;
        }
    }

    setIndex = (operationAddress >> (L2b)) & (numberOfSetsL2 - 1);
    tag = operationAddress >> (L2b + L2s);

    // Search inside cache L2 in set with index "setIndex" for tag hits.
    for(i = 0; i < L2E; i++){
        if(L2[setIndex][i].v == 0) {
            continue;
        }
        buffer = strtol(L2[setIndex][i].tag, NULL, 16);
        if(buffer == tag) {
            hitL2 = 1;
            // Overwrite corresponding hexademicals.
            strncpy(L2[setIndex][i].data, data, size*2);
            break;
        }
    }

    timeCounter++; // Instruction complete increase global time.

    // Just write to RAM directly if misses. Don't load to cache. (No Write Allocate policy)
    // If this instruction was a hit then data is already in cache so write to RAM right after. (Write Through policy)
    writeRAM(operationAddress, size, data);

    // Increment hit/miss global counters.
    L1Dhits += hitL1D;
    L1Dmisses += hitL1D ? 0 : 1;
    L2hits += hitL2;
    L2misses += hitL2 ? 0 : 1;
}

void modifyData(int operationAddress, int size, char* data) {
    loadData(operationAddress, size);
    timeCounter--; // Revert back time by 1 still because unlike counter being incremented on load the instruction is not complete.
    storeData(operationAddress, size, data);
}


void executeTrace(char* filename) {
    FILE* fptr = fopen(filename, "r");
    char line[49] = "\0", instruction, data[17] = "\0";
    int operationAddress, size;

    while(fgets(line, sizeof(line), fptr)) {
        sscanf(line, "%c %x, %d, %s", &instruction, &operationAddress, &size, data);
        switch(instruction) {
            case 'I':
                loadInstruction(operationAddress, size);
                break;
            
            case 'L':
                loadData(operationAddress, size);
                break;
            
            case 'S':
                storeData(operationAddress, size, data);
                break;
            
            case 'M':
                modifyData(operationAddress, size, data);
                break;
        }
    }

    fclose(fptr);
}
