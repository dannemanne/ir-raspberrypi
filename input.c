// input.c
//
// Program to record IR signals sent to GPIO ports
//
// After installing bcm2835, you can build this 
// with something like:
// gcc -o input  -l rt input.c -l bcm2835
// sudo ./input
//
// Author: Daniel Viklund
// Copyright (C) 2012 Daniel Viklund

#include <bcm2835.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

// Input on RPi pin GPIO 03
#define SIGNAL_PIN RPI_GPIO_P1_03
#define CTRL_PIN   RPI_GPIO_P1_11

#define GPIO_HIGH  1
#define GPIO_LOW   0

#define SIGNAL_INTERVAL 100000 // Measured in nano seconds
#define SIGNAL_LENGTH   3000  // The number of states stores

#define DB_DATABASE "irdb.sqlite3"

int main(int argc, char **argv)
{

    if( argc <= 1 )
    {
        printf("Usage:\n");
        printf("  %s record [signal-name]            # Will record a ir-signal and save it with the supplied name\n", argv[0]);
        printf("  %s emit [signal-name]              # Will retrieve a pre recorded signal from the database and emit it\n", argv[0]);
        return 0;
    }
    else
    {
        printf("Performing action: %s\n", argv[1]);
    }

    int q_cnt = 5,q_size = 150,ind = 0;
    char **queries = malloc(sizeof(char) * q_cnt * q_size);

    sqlite3_stmt *stmt;
    sqlite3 *handle;
    int retval;

    retval = sqlite3_open( DB_DATABASE, &handle );
    if( retval )
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(handle));
        sqlite3_close(handle);
        return(1);
    }

    char create_table[100] = "CREATE TABLE IF NOT EXISTS signals (sig_name TEXT PRIMARY KEY,intervals TEXT NOT NULL)";
    char query[50] = "SELECT * from signals";
    retval = sqlite3_exec(handle,create_table,0,0,0);
    if(retval)
    {
        printf("Failed to create table!\n");
        return retval;
    }
    else
    {
        printf("Table existed or was created.\n");
    }

    if(strcmp(argv[1], "record") == 0)
    {
        if(argc < 3)
        {
            printf("The record command needs a signal-name to save with.");
            return 0;
        }
//        return record_signal(argv[2]);
        if (!bcm2835_init())
            return 1;

        // Set RPI ctrl pin to be an output
        bcm2835_gpio_fsel(CTRL_PIN, BCM2835_GPIO_FSEL_OUTP);

        // Set it to High to activate the Receiver circuit
        bcm2835_gpio_write(CTRL_PIN, HIGH);

        // Set RPI pin to be an input
        bcm2835_gpio_fsel(SIGNAL_PIN, BCM2835_GPIO_FSEL_INPT);
        //  with a pullup
        bcm2835_gpio_set_pud(SIGNAL_PIN, BCM2835_GPIO_PUD_UP);

        struct timespec start, current;
        int count, recording, scanning, storage[1000], last_sec, last_nsec;
        uint8_t state, last_state; //, storage[SIGNAL_LENGTH];
        char chrStorage[100000], tmpSignal[20]; // Check optimal length later

        recording = 1;
        scanning = 0;
        count = 0;
        last_state = GPIO_HIGH;

        printf("Recording the signal: %s!\n", argv[2]);

//        snprintf(tmpSignal, 10, "%d", 456756789); //Convert integer to decimal string
//        strcat(chrStorage, tmpSignal);
//        strcat(chrStorage, ",");
//        snprintf(tmpSignal, 10, "%d", 134456789); //Convert integer to decimal string
//        strcat(chrStorage, tmpSignal);
//        strcat(chrStorage, ",");
//        snprintf(tmpSignal, 10, "%d", 123987789); //Convert integer to decimal string
//        strcat(chrStorage, tmpSignal);
//        strcat(chrStorage, ",");
//        snprintf(tmpSignal, 10, "%d", 321456789); //Convert integer to decimal string
//        strcat(chrStorage, tmpSignal);
//        strcat(chrStorage, ",");

//        printf("The result is: %s\n", chrStorage);
//        char insertStmt[100000];
//        strcat(insertStmt, "INSERT INTO signals VALUES('");
//        strcat(insertStmt, argv[2]);
//        strcat(insertStmt, "', '");
//        strcat(insertStmt, chrStorage);
//        strcat(insertStmt, "')");
//        retval = sqlite3_exec(handle,insertStmt,0,0,0);
//        if(retval)
//        {
//            printf("Could not save to DB");
//        }
//        else
//        {
//            printf("Saved signal with name: %s\n", argv[2]);
//        }

        while(recording)
        {
            // Reading state from GPIO PIN
            // Default state is GPIO_HIGH
            state = bcm2835_gpio_lev(SIGNAL_PIN);

            clock_gettime(CLOCK_REALTIME, &current);

            if(state != last_state)
            {
                //clock_gettime(CLOCK_REALTIME, &current);

                if(scanning)
                {
                    storage[count] = ( current.tv_sec - last_sec ) * 1000000000 + current.tv_nsec - last_nsec;
                    count++;
                }
                else
                {
                    scanning = 1;
                    printf("Scanning started\n");
                }

                last_sec = current.tv_sec;
                last_nsec = current.tv_nsec;
                last_state = state;
            }
            else if(scanning && current.tv_sec - last_sec > 1)
            {
                recording = 0;
                printf("recording stopped\n");
            }
        }

        for(count = 0; count < SIGNAL_LENGTH; count++)
        {
            if(storage[count])
            {
                printf("%i,", storage[count]);

                snprintf(tmpSignal, 10, "%d", storage[count]); //Convert integer to decimal string
                strcat(chrStorage, tmpSignal);
                strcat(chrStorage, ",");
            }
            else
            {
                count = SIGNAL_LENGTH;
            }
        }

        printf("detected signal looks like this:\n%s", chrStorage);

        char insertStmt[100000];
        strcat(insertStmt, "INSERT INTO signals VALUES('");
        strcat(insertStmt, argv[2]);
        strcat(insertStmt, "', '");
        strcat(insertStmt, chrStorage);
        strcat(insertStmt, "')");
        retval = sqlite3_exec(handle,insertStmt,0,0,0);
        if(retval)
        {
            printf("Could not save to DB");
        }
        else
        {
            printf("Saved signal with name: %s\n", argv[2]);
        }


    }
    else if(strcmp(argv[1], "test") == 0)
    {
        if (!bcm2835_init())
            return 1;

        // Set RPI ctrl pin to be an output
        bcm2835_gpio_fsel(CTRL_PIN, BCM2835_GPIO_FSEL_OUTP);

        // Set it to High to activate the Receiver circuit
        if(argc == 3){
            if(strcmp(argv[2], "high") == 0){
                bcm2835_gpio_write(CTRL_PIN, HIGH);
                printf("setting CTRL to high\n");
                sleep(5);
            }else{
                bcm2835_gpio_write(CTRL_PIN, LOW);
                printf("Setting CTRL to low\n");
                sleep(5);
            }
            return 0;
        }
        bcm2835_gpio_write(CTRL_PIN, LOW);
        printf("2");
        // Set RPI pin to be an input
        bcm2835_gpio_fsel(SIGNAL_PIN, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(SIGNAL_PIN, HIGH);
        printf("3");
        //  with a pullup
        //bcm2835_gpio_set_pud(RPI_GPIO_P1_13, BCM2835_GPIO_PUD_UP);
        int count;
        count = 0;
        //recording = true;
        while(count < 10)
        {
             if(count%2 == 0)
             {
                 bcm2835_gpio_write(SIGNAL_PIN, LOW);
             }else{
                 bcm2835_gpio_write(SIGNAL_PIN, HIGH);
             }
             sleep(1);
             count++;
        }
//        printf("State is %i!\n", bcm2835_gpio_lev(RPI_GPIO_P1_13));

    }
    else if(strcmp(argv[1], "list") == 0)
    {
        printf("Listing signals from Database.\n\n");
        retval = sqlite3_prepare_v2(handle, query, -1, &stmt,0);
        if(retval)
        {
            printf("Failed to read signals from DB: %s\n", sqlite3_errmsg(handle));
            return retval;
        }
        int cols = sqlite3_column_count(stmt);
        printf("Has %i columns.\n", cols);
        while(1)
        {
            printf("Gonna check row\n");
            retval = sqlite3_step(stmt);
            if(retval == SQLITE_ROW)
            {
                int col;
                for(col = 0; col < cols; col++)
                {
                    const char *val = (const char*)sqlite3_column_text(stmt,col);
                    printf("%s = %s\t",sqlite3_column_name(stmt,col),val);
                }
                printf("\n");
            }
            else if(retval == SQLITE_DONE)
            {
                printf("No more results.");
                break;
            }
            else
            {
                printf("Something went wrong\n");
                return retval;
            }
        }
    }
    else if(strcmp(argv[1], "emit") == 0)
    {
        uint8_t last_state;
        char findStmt[100000];
        strcat(findStmt, "SELECT * FROM signals WHERE sig_name = '");
        strcat(findStmt, argv[2]);
        strcat(findStmt, "'");

        char *token;
        char *val;

        if (!bcm2835_init())
            return 1;

        // Set RPI ctrl pin to be an output
        bcm2835_gpio_fsel(CTRL_PIN, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(CTRL_PIN, LOW);

        // Set RPI signal pin to be an output
        bcm2835_gpio_fsel(SIGNAL_PIN, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(SIGNAL_PIN, HIGH);
        last_state = HIGH;

        retval = sqlite3_prepare_v2(handle, findStmt, -1, &stmt,0);
        if(retval)
        {
            printf("Failed to read signals from DB: %s\n", sqlite3_errmsg(handle));
            return retval;
        }

        retval = sqlite3_step(stmt);
        if(retval == SQLITE_ROW)
        {
            val = (char*)sqlite3_column_text(stmt,1);
            printf("%s\n",val);
            token = strtok(val, ",");
            int total_token = 0;
            int token_i;
            int start_sec = 0;
            int start_nsec = 0;
            struct timespec current;
            clock_gettime(CLOCK_REALTIME, &current);

            // Found a signal, starting emit
            while(token != NULL)
            {
                if(start_sec == 0){
                    clock_gettime(CLOCK_REALTIME, &current);
                    start_sec = current.tv_sec;
                    start_nsec = current.tv_nsec;
                }

                if(last_state == HIGH){
                    bcm2835_gpio_write(SIGNAL_PIN, LOW);
                    last_state = LOW;
                }else{
                    bcm2835_gpio_write(SIGNAL_PIN, HIGH);
                    last_state = HIGH;
                }

                token_i = atoi(token);
                while( (start_sec * 1000000000 + start_nsec + token_i + total_token ) > ( current.tv_sec * 1000000000 + current.tv_nsec ) )
                    clock_gettime(CLOCK_REALTIME, &current);

                total_token += token_i;
                token = strtok(NULL, ",");
            }
   //             }
   //             printf("\n");
            }
            else
            {
                printf("Something went wrong\n");
            }

            bcm2835_gpio_write(SIGNAL_PIN, HIGH);
    }
    else
    {
        printf("Invalid syntax!\n");
    }

    return 0;

    // If you call this, it will not actually access the GPIO. Use for testing
    // bcm2835_set_debug(1);

}

int record_signal(char *signal_name)
{

    if (!bcm2835_init())
        return 1;

    // Set RPI pin to be an input
    bcm2835_gpio_fsel(SIGNAL_PIN, BCM2835_GPIO_FSEL_INPT);
    //  with a pullup
    bcm2835_gpio_set_pud(SIGNAL_PIN, BCM2835_GPIO_PUD_UP);

    struct timespec start, current;
    int count, recording, scanning, storage[1000], last_sec, last_nsec;
    uint8_t state, last_state; //, storage[SIGNAL_LENGTH];

    recording = 1;
    scanning = 0;
    count = 0;
    last_state = GPIO_HIGH;

    printf("Recording the signal: %s!\n", signal_name);

    while(recording)
    {
        // Reading state from GPIO PIN
        // Default state is GPIO_HIGH
        state = bcm2835_gpio_lev(SIGNAL_PIN);

        clock_gettime(CLOCK_REALTIME, &current);

        if(state != last_state)
        {
            //clock_gettime(CLOCK_REALTIME, &current);

            if(scanning)
            {
                storage[count] = ( current.tv_sec - last_sec ) * 1000000000 + current.tv_nsec - last_nsec;
                count++;
            }
            else
            {
                scanning = 1;
            }

            last_sec = current.tv_sec;
            last_nsec = current.tv_nsec;
            last_state = state;
        }
        else if(scanning && current.tv_sec - last_sec > 1)
        {
            recording = 0;
        }
    }

        // If state is GPIO_LOW then signal is detected
//        if(state == GPIO_LOW)
//        {
            // If scanning is 0 then this is the first time
            // we detected a signal. Initialize... 
//            if(!scanning)
//            {
//                scanning = 1;
//                clock_gettime(CLOCK_REALTIME, &start);
//            }

            // Store state
//            storage[count] = state;
//            count++;
//        }

        // If state was not low then check if scanning has
        // started. Otherwise we are in default state.
//        else if(scanning)
//        {
            // Store state
//            storage[count] = state;
//            count++;
//        }

        // If we have reached the SIGNAL_LENGTH limit, stop recording
//        if(count >= SIGNAL_LENGTH)
//            recording = 0;

//        if(scanning)
//       {
            // Cannot use sleep to wait for SIGNAL_INTERVAL to pass so
            // instead the process will stay in the loop untill enough 
            // time has passed.
//            clock_gettime(CLOCK_REALTIME, &current);
//            while( ( (current.tv_sec - start.tv_sec) * 1000000000 + current.tv_nsec - start.tv_nsec ) < count * SIGNAL_INTERVAL )
//                clock_gettime(CLOCK_REALTIME, &current);
//        }
//    }

    // Prints the result
    for(count = 0; count < SIGNAL_LENGTH; count++)
    {
        if(storage[count])
        {
            printf("%i,", storage[count]);
        }
        else
        {
            count = SIGNAL_LENGTH;
        }
    }

    return 0;
}
