#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define MAX_LOG_NAME_LENGTH 32
#define DEFAULT_LOG_FILE "log.csv"
#define DEFAULT_SLEEP_TIME 10
#define MAX_TEMP 115
#define MIN_TEMP 0
#define TEMP_SUFFIX "/temp1_input"
#define NAME_SUFFIX "/name"
#define FULL_PATH_MAX (PATH_MAX + sizeof(TEMP_SUFFIX) + sizeof(NAME_SUFFIX))
bool is_file_a_cpu_info(char* path)
{
    char name_file_path[PATH_MAX];
    snprintf(name_file_path, PATH_MAX, "%s/name", path);
    FILE* file = fopen(name_file_path, "r");
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

void find_cpu_temperature_path(char* path)
{
    DIR* pDir = opendir("/sys/class/hwmon/");
    if (pDir == NULL)
    {
        perror("Error while opening directory /sys/class/hwmon/ check if your system stores cpu temp there");
        path[0] = '\0';
        return;
    }
    struct dirent* entry;
    bool found = false;
    while ((entry = readdir(pDir)))
    {
        if (entry->d_name[0] == '.')
            continue;

        char folder_path[PATH_MAX];
        snprintf(folder_path, sizeof(folder_path) - sizeof(TEMP_SUFFIX), "/sys/class/hwmon/%s", entry->d_name);
        if (is_file_a_cpu_info(folder_path))
        {
            snprintf(path, FULL_PATH_MAX, "%s" TEMP_SUFFIX, folder_path);
            found = true;
            break;
        }
    }
    closedir(pDir);
    if (!found)
    {
        perror("Not found cpu file");
        path[0] = '\0';
    }
}

double get_cpu_temperature(const char* path)
{
    int temp_raw;
    FILE* pFileCpu = fopen(path, "r");
    if (pFileCpu == NULL)
    {
        perror("Error while opening the file");
        return -1;
    }
    if (fscanf(pFileCpu, "%d", &temp_raw) != 1)
    {
        fprintf(stderr, "Error while reading from file\n");
        fclose(pFileCpu);
        return -1;
    }
    fclose(pFileCpu);
    return (double)temp_raw / 1000.0;
}
void get_gpu_temperature(char* output_buffer, const int buffer_size)
{
    FILE* pFileGpu = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits", "r");
    if (pFileGpu == NULL)
    {
        snprintf(output_buffer, buffer_size, "N/A");
        return;
    }
    if (fgets(output_buffer, buffer_size, pFileGpu) != NULL)
    {
        output_buffer[strcspn(output_buffer, "\r\n")] = 0;
    }
    else
    {
        snprintf(output_buffer, buffer_size, "N/A");
    }
    pclose(pFileGpu);
}
void print_usage(char* name)
{
    printf("Usage: ./%s [-i delay in seconds] [-f log file name]\n", name);
    printf("Default delay is %d seconds\n", DEFAULT_SLEEP_TIME);
    printf("Default log file is %s\n", DEFAULT_LOG_FILE);
}

int main(int argc, char** argv)
{
    long sleep_time = DEFAULT_SLEEP_TIME;
    char log_filename[MAX_LOG_NAME_LENGTH] = DEFAULT_LOG_FILE;
    int opt;


    while ((opt = getopt(argc, argv, "i:f:h")) != -1)
    {
        switch (opt)
        {
        case 'i':
            {
                char* endptr;
                long val = strtol(optarg, &endptr, 10);
                if (endptr == optarg || *endptr != '\0')
                {
                    fprintf(stderr, "Invalid argument '%s'\n", optarg);
                    return 1;
                }
                if (val < 0)
                {
                    val = DEFAULT_SLEEP_TIME;
                }
                sleep_time = (int)val;
                break;
            }
        case 'f':
            strncpy(log_filename, optarg, sizeof(log_filename) - 1);
            log_filename[sizeof(log_filename) - 1] = '\0';
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    char path[FULL_PATH_MAX] = "/sys/class/hwmon/hwmon1/temp1_input";
    find_cpu_temperature_path(path);

    if (path[0] == '\0')
        return 1;

    printf("Started using path : %s\n", path);

    // If file doesn't exist we add headers explaining each column
    FILE* check_log = fopen(log_filename, "r");
    if (check_log == NULL)
    {
        FILE* init_log = fopen(log_filename, "w");
        if (init_log != NULL)
        {
            fprintf(init_log, "Timestamp, CPU_TEMP_C, GPU_TEMP_C\n");
            fclose(init_log);
        }
    }
    else
    {
        fclose(check_log);
    }

    while (true)
    {
        time_t raw_time;
        char timestamp[20];
        time(&raw_time);
        const struct tm* time_info = localtime(&raw_time);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);


        char gpu_temp[16];
        get_gpu_temperature(gpu_temp, sizeof(gpu_temp));

        const double cpu_temp = get_cpu_temperature(path);
        if (cpu_temp > MIN_TEMP && cpu_temp < MAX_TEMP)
        {
            FILE* log = fopen(log_filename, "a");
            if (log == NULL)
            {
                perror("Error while opening log file");
            }
            else
            {
                fprintf(log, "%s, %.2f, %s\n", timestamp, cpu_temp, gpu_temp);
                fclose(log);
            }
        }
        else
        {
            fprintf(stderr, "Error while reading cpu_temp (%.2f)\n", cpu_temp);
        }
        sleep(sleep_time);
    }
    return 0;
}
