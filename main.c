/**
 * applefand
 * Copyright (C) 2010 Kenny Parnell <k.parnell@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/
 */

#include <glob.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sensors/sensors.h>

#define CTL_FILE "/sys/devices/platform/applesmc.768/fan1_min"
#define MIN_SPEED 2000
#define MAX_SPEED 4500
#define STEP 50
#define GAP 3
#define DEFAULT_DAEMON 0;
#define DEFAULT_INTERVAL 5;
#define DEFAULT_THRESHOLD 65

void usage(char *argv[]) {
    printf("usage: %s [options]\n", argv[0]);
    printf("\t-h\tshow this message\n");
    printf("\t-d\trun as background daemon\n");
    printf("\t-i\tset interval to check fan\n");
    printf("\t-t\ttemperature threshold in terms of C\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    static int daemonize, ch, interval, threshold, temp, core, bump;
    pid_t pid, sid;
    FILE *fp;
    char line[130];

    daemonize = DEFAULT_DAEMON;
    interval = DEFAULT_INTERVAL;
    threshold = DEFAULT_THRESHOLD;

    // parse args
    while ((ch = getopt(argc, argv, "hdi:t:")) != -1) {
        switch(ch) {
            case 'h':
                usage(argv);
                break;
            case 'd':
                daemonize = 1;
                break;
            case 'i':
                interval = atoi(optarg);
                break;
            case 't':
                threshold = atoi(optarg);
                break;
        }
    }

    // daemonize if needed
    if (daemonize) {
        pid = fork();

        if (pid < 0) {
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        umask(0);

        sid = setsid();

        if (sid < 0) {
            exit(EXIT_FAILURE);
        }
    }

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    while(1) {
        sleep(interval);
        bump = 0;
        fp = popen("sensors 2>&1", "r");
        while (fgets(line, sizeof line, fp)) {
            core = -1;
            temp = 0;
            sscanf(line, "Core %i:      +%i", &core, &temp);
            if (core != -1 && temp > threshold) {
                bump = 1;
                break;
            }
        }
        pclose(fp);

        if ((bump && (temp - threshold < GAP))
                || (!bump && (threshold - temp < GAP))) {
            printf("Inside Gap\n");
            continue;
        }

        fp = fopen(CTL_FILE, "r");
        fscanf(fp, "%i", &temp);
        fclose(fp);
        fp = fopen(CTL_FILE, "w");
        if (bump) {
            temp = (temp + STEP < MAX_SPEED) ? (temp + STEP) : MAX_SPEED;
            fprintf(fp, "%i", temp);
            printf("Going Up to %i!\n", temp);
        } else {
            temp = (temp - STEP > MIN_SPEED) ? (temp - STEP) : MIN_SPEED;
            fprintf(fp, "%i", temp);
            printf("Going Down to %i!\n", temp);
        }
        fclose(fp);
    }

    return EXIT_SUCCESS;
}
