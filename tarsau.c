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

    // Write the description
    write_organization_section(outputFile, fileNames, numFiles);

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

    //eof marker
    fputc(0x1A, outputFile);

    fclose(outputFile);

    printf("Archive created successfully: %s\n", outputFileName);
}

void write_organization_section(FILE *outputFile, const char *fileNames[], int numFiles)
{
    // Write first 10 byte
    fprintf(outputFile, "%-10d", numFiles);

    for (int i = 0; i < numFiles; ++i)
    {
        struct stat fileStat;
        if (stat(fileNames[i], &fileStat) != 0)
        {
            error_exit("Error getting file information.");
        }

        // Write file information 
        fprintf(outputFile, "|%s,%o,%ld|", fileNames[i], fileStat.st_mode & 0777, fileStat.st_size);
    }

    fprintf(outputFile, "\n");
}

char** extractFileNames(const char *input, int *numFiles) {
    const char *delimiter = "||";
    const char *innerDelimiter = ",";

    char buffer[256]; 
    strncpy(buffer, input + 10, sizeof(buffer) - 1);  
    buffer[sizeof(buffer) - 1] = '\0';  

    char *token = strtok(buffer, delimiter);
    char **fileNames = malloc(MAX_FILES * sizeof(char*));
    *numFiles = 0;

    while (token != NULL && *numFiles < MAX_FILES) {
        // Check if the token starts with "texts/"
        if (strncmp(token, "texts/", 6) == 0) {
            // Extract the file name
            const char *fileNameToken = strstr(token, "texts/") + 6;
            char *endToken = strchr(fileNameToken, ',');
            if (endToken != NULL) {
                // Calculate the length of the file name
                size_t fileNameLength = endToken - fileNameToken;
                
                // Allocate memory for the file name and copy it
                fileNames[*numFiles] = malloc((fileNameLength + 1) * sizeof(char));
                strncpy(fileNames[*numFiles], fileNameToken, fileNameLength);
                fileNames[*numFiles][fileNameLength] = '\0';
                (*numFiles)++;
            }
        } 

        token = strtok(NULL, delimiter);
    }

    return fileNames;
}

void writeToFiles(const char *organizationSection, const char *textContent) {
    int numFiles;
    char **fileNames = extractFileNames(organizationSection, &numFiles);

    // Open and write to each file
    FILE *file;
    const char *line = textContent;

    for (int i = 0; i < numFiles; i++) {
        char fullPath[256];  
        snprintf(fullPath, sizeof(fullPath), "%s", fileNames[i]);  
        file = fopen(fullPath, "w");

        if (file != NULL) {
            // Write a line to the file
            const char *newline = strchr(line, '\n');
            size_t lineLength = (newline != NULL) ? (size_t)(newline - line) : strlen(line);
            fwrite(line, sizeof(char), lineLength, file);

            fclose(file);
            printf("Content written to %s\n", fullPath);

            // Move to the next line
            line = (newline != NULL) ? (newline + 1) : NULL;
        } else {
            fprintf(stderr, "Error opening %s for writing\n", fullPath);
        }
    }

    // Free memory
    for (int i = 0; i < numFiles; i++) {
        free(fileNames[i]);
    }
    free(fileNames);
}

void extract_archive(const char *archiveFileName, const char *extractDir) {
    FILE *archiveFile = fopen(archiveFileName, "r");
    if (archiveFile == NULL) {
        fprintf(stderr, "Error opening archive file: %s\n", archiveFileName);
        return;
    }

    // Read organization section
    char organizationSection[256];  
    fgets(organizationSection, sizeof(organizationSection), archiveFile);

    // Skip an empty line
    fgets(organizationSection, sizeof(organizationSection), archiveFile);

    // Create the extraction directory if it doesn't exist
    mkdir(extractDir, 0777);

    // Extract file names
    int numFiles;
    char **fileNames = extractFileNames(organizationSection, &numFiles);

    // Read the content of each file and write it to the corresponding file
    for (int i = 0; i < numFiles; i++) {
        char *content = NULL;
        size_t contentSize = 0;

        // Read content from the archive line by line
        ssize_t bytesRead;
        while ((bytesRead = getline(&content, &contentSize, archiveFile)) != -1) {
            // If a newline character is present at the end, remove it
            if (bytesRead > 0 && content[bytesRead - 1] == '\n') {
                content[bytesRead - 1] = '\0';
            }

            char *fullPath = malloc(strlen(extractDir) + strlen(fileNames[i]) + 2); 
            sprintf(fullPath, "%s/%s", extractDir, fileNames[i]);

            FILE *file = fopen(fullPath, "w");
            if (file != NULL) {
                // Write content to the file
                fprintf(file, "%s", content);

                fclose(file);
                printf("Content written to %s\n", fullPath);

                free(fullPath);
                free(content);
                break;  // Move to the next file after writing the content
            } else {
                fprintf(stderr, "Error opening %s for writing\n", fullPath);
                free(fullPath);
            }
        }
    }

    // Free allocated memory
    for (int i = 0; i < numFiles; i++) {
        free(fileNames[i]);
    }
    free(fileNames);

    fclose(archiveFile);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        error_exit("Usage: tarsau -b|-a -o <output_file> <input_file1> <input_file2> ...");
    }

    if (strcmp(argv[1], "-b") == 0)
    {
        char *outputFileName = "a.sau"; 

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
    } else if (strcmp(argv[1], "-a") == 0) {
        //Archive extraction (-a)
        if (argc != 4) {
            error_exit("Usage: tarsau -a <archive_file.sau> <extract_directory>");
        }

        const char *archiveFileName = argv[2];
        const char *extractDirName = argv[3];

        // Check if the archive file exists
        struct stat archiveStat;
        if (stat(archiveFileName, &archiveStat) != 0 || !S_ISREG(archiveStat.st_mode)) {
            error_exit("Archive file is inappropriate or corrupt!");
        }

        // Extract the archive
        extract_archive(archiveFileName, extractDirName);
    }
    else
    {
        error_exit("Invalid option. Use -b for archive creation or -a for archive extraction.");
    }

    return 0;
}
