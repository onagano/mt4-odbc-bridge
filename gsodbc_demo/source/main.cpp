#include "stdhead.h"

void testMSSQL();
void testORACLE();
void testVFP();
void testIndex();

void main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("Usage of text:\n");
        printf("\ttest mssql | oracle | vfp | index | all\n");
        printf("\tplease, verify login information in mssql.cpp, oracle.cpp and index.cpp\n");
        printf("\tand re-compile if needed.\n");
    }
    int i;
    for (i = 1; i < argc; i++)
    {
        if (!stricmp(argv[i], "mssql"))
            testMSSQL();
        if (!stricmp(argv[i], "oracle"))
            testORACLE();
        if (!stricmp(argv[i], "vfp"))
            testVFP();
        if (!stricmp(argv[i], "index"))
            testIndex();
        if (!stricmp(argv[i], "all"))
        {
            testMSSQL();
            testORACLE();
            testVFP();
            testIndex();
        }
    }
}
