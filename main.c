#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

#define SLEEP_TIME 10
#define PATH_LEN 4096


bool is_file_a_cpu_info(char *path)
{
    char name_file_path[PATH_LEN];
    snprintf(name_file_path, PATH_LEN, "%s/name", path);
    FILE *file = fopen(name_file_path, "r");
    if (file == NULL)
    {
        return false;
    }

    char line[256];
    if (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "k10temp") == 0 || strcmp(line, "coretemp") == 0)
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
        path[0] = '\0';
        return;
    }
    struct dirent *entry;
    bool found = false;
    while ((entry  = readdir(pDir)))
    {
        if (entry->d_name[0] == '.') continue;

        char folder_path[PATH_LEN];
        snprintf(folder_path, sizeof(folder_path), "/sys/class/hwmon/%s", entry->d_name);
        if (is_file_a_cpu_info(folder_path))
        {
            snprintf(path, PATH_LEN+32, "%s/temp1_input", folder_path);
            found = true;
            break;
        }
    }
    closedir(pDir);
    if (!found)
    {
        perror("Not found cpu file");
        path[0] = '\0';
        return;
    }
}

double get_cpu_temperature(const char *path)
{
    int temp_raw;
    FILE* pFileCpu = fopen(path, "r");
    if (pFileCpu == NULL)
    {
        perror("Error while opening the file");
        return -1;
    }
    if (fscanf(pFileCpu,"%d", &temp_raw)!=1)
    {
        fprintf(stderr, "Error while reading from file\n");
        fclose(pFileCpu);
        return -1;
    }
    fclose(pFileCpu);
    return (double)temp_raw/1000.0;

}
void get_gpu_temperature(char *output_buffer,const int buffer_size)
{
    FILE* pFileGpu = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits", "r");
    if (pFileGpu == NULL)
    {
        snprintf(output_buffer, buffer_size, "N/A");
        return;
    }
    if (fgets(output_buffer, buffer_size, pFileGpu)!=NULL)
        output_buffer[strcspn(output_buffer, "\r\n")] = 0;
    else
    {
        snprintf(output_buffer, buffer_size, "N/A");
    }
    pclose(pFileGpu);
}


int main(void)
{
    char path[PATH_LEN+32] = "/sys/class/hwmon/hwmon1/temp1_input";
    find_cpu_temperature_path(path);

    if (path[0]=='\0')
        return 1;

    printf("Started using path : %s\n", path);

    while (true)
    {
        time_t raw_time;
        char timestamp[20];
        time(&raw_time);
        struct tm * timeinfo = localtime(&raw_time);
        strftime(timestamp, sizeof(timestamp),"%Y-%m-%d %H:%M:%S",timeinfo);


        char gpu_temp[16];
        get_gpu_temperature(gpu_temp, sizeof(gpu_temp));

        const double cpu_temp = get_cpu_temperature(path);
        if (cpu_temp > 0 && cpu_temp < 115) {
            FILE *log = fopen("log.csv", "a");
            if (log == NULL)
            {
                perror("Error while opening log file");
            }
            else
            {
                fprintf(log, "%s, %.2f, %s\n", timestamp, cpu_temp, gpu_temp);
                fclose(log);
            }
        } else {
            fprintf(stderr, "Błędny odczyt temperatury (%.2f)\n", cpu_temp);
        }
        sleep(SLEEP_TIME);
    }
    return 0;
}
