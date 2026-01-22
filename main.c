#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    while (1)
    {
        time_t rawtime;
        struct tm * timeinfo;
        char timestamp[20];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp),"%Y-%m-%d %H:%M:%S",timeinfo);

        FILE *pFileCpu;
        char *path = "/sys/class/hwmon/hwmon1/temp1_input";
        int temp_raw;
        pFileCpu = fopen(path, "r");
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

        FILE *pFileGpu;
        char buffer[10];
        pFileGpu = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits", "r");
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