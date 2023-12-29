#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE 200 * 1024 * 1024 // 200 MB
#define MAX_FILENAME_LENGTH 256

void error_exit(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

void create_directory(const char *dirName)
{
    // Create a directory with all permissions
    if (mkdir(dirName, S_IRWXU) != 0)
    {
        error_exit("Error creating directory.");
    }
}

void create_archive(const char *outputFileName, const char *fileNames[], int numFiles)
{
    // Open the output file for writing operation
    FILE *outputFile = fopen(outputFileName, "w");
    if (!outputFile)
    {
        error_exit("Error opening output file for writing.");
    }

    // Write archived files
    for (int i = 0; i < numFiles; ++i)
    {
        FILE *inputFile = fopen(fileNames[i], "r");
        if (!inputFile)
        {
            fclose(outputFile);
            error_exit("Error opening input file for reading.");
        }

        // Write the contents of the input file to the output file
        int ch;
        while ((ch = fgetc(inputFile)) != EOF)
        {
            fputc(ch, outputFile);
        }

        fputc('\n', outputFile);

        // Close the input file
        fclose(inputFile);
    }

    fclose(outputFile);

    printf("Archive created successfully: %s\n", outputFileName);
}

int main(int argc, char *argv[])
{
    // Check if the required arguments are provided
    if (argc < 3)
    {
        error_exit("Usage: tarsau -b|-a -o <output_file> <input_file1> <input_file2> ...");
    }

    if (strcmp(argv[1], "-b") == 0)
    {
        char *outputFileName = "a.sau"; // Default output file name

        for (int i = 2; i < argc; ++i)
        {
            if (strcmp(argv[i], "-o") == 0)
            {
                if (i + 1 < argc)
                {
                    outputFileName = argv[i + 1];
                }
                else
                {
                    error_exit("Missing output file name after -o parameter.");
                }
            }
        }

        // Extract input file names
        const char *fileNames[MAX_FILES];
        int numFiles = 0;

        for (int i = 2; i < argc; ++i)
        {
            if (strcmp(argv[i], "-o") != 0)
            {
                fileNames[numFiles] = argv[i];
                numFiles++;

                if (numFiles >= MAX_FILES)
                {
                    error_exit("Exceeded the maximum number of input files.");
                }
            }
        }

        create_archive(outputFileName, fileNames, numFiles);
    }
    else
    {
        error_exit("Invalid option. Use -b for archive creation or -a for archive extraction.");
    }

    return 0;
}
