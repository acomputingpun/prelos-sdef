#define VERSION_ID 8
#define DEFAULT_MEMORY_PATH "./memory/"

NodeMemory readMemoryFile(int oDepth, int autoDividePeriod);
NodeMemory readMemoryFileFrom(int oDepth, int autoDividePeriod, char* memoryPath);
int writeMemoryFile(NodeMemory nm);
int writeMemoryFileTo(NodeMemory nm, char* memoryPath);
