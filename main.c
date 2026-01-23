#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

bool isFileACpuInfo(char *path)
{
    char name_file_path[1024];
    snprintf(name_file_path, sizeof(name_file_path), "%s/name", path);
    FILE *file = fopen(name_file_path, "r");
    if (file == NULL)
    {
        return false;
    }

    char line[256];
    if (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "k10temp") == 0)
        {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

void find_cpu_temperature_path(char *path)
{
    DIR *pDir = opendir("/sys/class/hwmon/");
    if (pDir == NULL)
    {
        perror("Error while opening directory /sys/class/hwmon/ check if your system stores cpu temp there");
        path = "\0";
        return;
    }
    struct dirent *entry;
    bool found = false;
    while ((entry  = readdir(pDir)))
    {
        if (entry->d_name[0] == '.') continue;

        char folder_path[1024];
        snprintf(folder_path, sizeof(folder_path), "/sys/class/hwmon/%s", entry->d_name);
        if (isFileACpuInfo(folder_path))
        {
            snprintf(path, 1024, "%s/temp1_input", folder_path);
            found = true;
            break;
        }
    }
    closedir(pDir);
    if (!found)
    {
        perror("Not found cpu file");
        path="\0";
        return;
    }


}



int main(void)
{
    char path[2048] = "/sys/class/hwmon/hwmon1/temp1_input";
    find_cpu_temperature_path(path);

    if (path[0]=='\0')
        return 1;

    printf("Started using path : %s\n", path);

    while (1)
    {
        time_t rawtime;
        char timestamp[20];
        time(&rawtime);
        struct tm * timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp),"%Y-%m-%d %H:%M:%S",timeinfo);

        int temp_raw;
        FILE* pFileCpu = fopen(path, "r");
        if (pFileCpu == NULL)
        {
            perror("Error while opening the file");
            return 1;
        }
        if (fscanf(pFileCpu,"%d", &temp_raw)!=1)
        {
            fprintf(stderr, "Error while reading from file\n");
            fclose(pFileCpu);
            return 1;
        }
        fclose(pFileCpu);

        char buffer[10];
        FILE* pFileGpu = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits", "r");
        if (pFileGpu == NULL)
            return 1;
        if (fgets(buffer, sizeof(buffer), pFileGpu)!=NULL)
            buffer[strcspn(buffer, "\r\n")] = 0;
        pclose(pFileGpu);
        FILE *log = fopen("log.csv", "a");
        fprintf(log,"%s, %.2f, %s\n",timestamp,temp_raw/1000.0, buffer);
        fclose(log);
        sleep(10);
    }

    return 0;
}