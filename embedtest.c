/**
 * @file embedtest.c
 * Implementation of using shared data buffer to transmit and receive
 * 
 * @par Copyright
 */


/*
Note:
- It is simple file, I did not create makefile. In order to build this application by gcc, please execute the below command in terminal (Ubuntu 20)
gcc -Wall -pthread embedtest.c -o embedtest
- Mutex is being use to protect the shared buffer, others can be considered if requirements are more complex.
- In order to test the implementation, I also write all generated data to a file (/tmp/EmbedGenerateTest). If it is the same with removed data
(/tmp/EmbedTest), the implementation works perfectly.
*/

#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHARATERS_MAX       50
#define BUFF_SIZE           64
#define FILE_PATH           "/tmp/EmbedTest"
#define GENERATE_FILE_PATH  "/tmp/EmbedGenerateTest"

//////////////////////////////////////////////////////////////////////////////////////////////////
/// @struct data_t
///
/// This struct implements buffer data
//////////////////////////////////////////////////////////////////////////////////////////////////
struct data_t
{
    unsigned int mCurrentIndex;
    unsigned int mLength;
    char mData[BUFF_SIZE];
};


// mutex to protect data
pthread_mutex_t lock;
struct data_t DataBuff;

//////////////////////////////////////////////////////////////////////////////////////////////////
/// generateRandonString(const char *buff, unsigned int length)
/// This function is to generate random buffer with input length
///
/// @param      buf      This is a pointer to get generated data
///             length   Length of data
/// @return     void
//////////////////////////////////////////////////////////////////////////////////////////////////
void generateRandonString(const char *buff, unsigned int length)
{
    char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char *ptr = buff;
    while (length-- > 0) {
        unsigned int index = rand() % (sizeof(charset) - 1);
        *ptr++ = charset[index];
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////
/// generateData(void *arg)
/// This thread is to generate data and write to the shared buffer
///
/// @param      arg     UNUSED
/// @return     void*
//////////////////////////////////////////////////////////////////////////////////////////////////
void *generateData(void *arg)
{
    printf ("generateData() thread starts\n");
    while(1)
    {

        // generate randon number
        unsigned int uiGenerateNum = (unsigned int)(rand() % CHARATERS_MAX);

        // if size == 0, re-generate
        if (uiGenerateNum == 0) continue;

        char pData[uiGenerateNum];
        generateRandonString(pData, uiGenerateNum);

        bool isAppended = false;
        while (isAppended == false)
        {
            // acquire mutex
            pthread_mutex_lock(&lock);

            // check length
            if (uiGenerateNum > (BUFF_SIZE - DataBuff.mLength))
            {
                // release mutex
                pthread_mutex_unlock(&lock);
                continue;
            }

            // print out
            printf("Appending. Size = %d\n", uiGenerateNum);

            // Write all generated data to a file, then compare with removed data
            FILE *pGenerateFile = fopen(GENERATE_FILE_PATH, "a");

            if (pGenerateFile == NULL)
            {
                printf("unable to open file");

                // release mutex
                pthread_mutex_unlock(&lock);

                continue;
            }

            // For testing
            printf("data = ");
            for (int i = 0; i < uiGenerateNum; i++)
            {
                fprintf(pGenerateFile, "%c", pData[i]);
                printf("%c", pData[i]);
            }
            fclose(pGenerateFile);
            printf("\n");

            unsigned int uiGenerateIndex = DataBuff.mCurrentIndex + DataBuff.mLength;
            // append data
            for (int i = 0; i < uiGenerateNum; i++)
            {
                // reset index if overflow
                if ((uiGenerateIndex) >= BUFF_SIZE) uiGenerateIndex = 0;

                // copy data
                DataBuff.mData[uiGenerateIndex] = pData[i];

                // increase index
                ++uiGenerateIndex;
                //printf("index = %d", uiGenerateIndex);
            }

            // update mLength
            DataBuff.mLength += uiGenerateNum;

            // print out
            printf("Appended. Length = %d\n", DataBuff.mLength);
            
            // release mutex
            pthread_mutex_unlock(&lock);

            // reset status
            isAppended = true;
        }


        // add sleep to see the data easier
        usleep(2000000);
    }
    pthread_exit(NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/// removeData(void *arg)
/// This thread is to remove data from the shared buffer
///
/// @param      arg     UNUSED
/// @return     void*
//////////////////////////////////////////////////////////////////////////////////////////////////
void *removeData(void *arg)
{
    printf ("removeData() thread starts\n");
    while(1)
    {
        // generate randon number
        unsigned int uiRemoveNum = (unsigned int)(rand() % CHARATERS_MAX);

        // do nothing
        if (uiRemoveNum == 0) continue;

        // acquire mutex
        pthread_mutex_lock(&lock);

        // check length
        if (uiRemoveNum > DataBuff.mLength)
        {
            // release mutex
            pthread_mutex_unlock(&lock);
            continue;
        }

        // open file
        FILE *pFile = fopen(FILE_PATH, "a");
        if (pFile == NULL)
        {
            printf("unable to open file");

            // release mutex
            pthread_mutex_unlock(&lock);

            continue;
        }

        printf("Removing. Size = %d, buff = ", uiRemoveNum);

        unsigned int uiRemoveIndex = DataBuff.mCurrentIndex ;
        for (int i = 0; i < uiRemoveNum; i++)
        {
            // reset index if overflow
            if (uiRemoveIndex >= BUFF_SIZE)
            {
                uiRemoveIndex = 0;
            }

            // write down to the file
            fprintf(pFile, "%c", DataBuff.mData[uiRemoveIndex]);

            // print out
            printf("%c", DataBuff.mData[uiRemoveIndex]);

            // clear data
            DataBuff.mData[uiRemoveIndex] = 0;

            // increase index
            ++uiRemoveIndex;
        }

        // new line
        //fprintf(pFile, "\n");
        printf("\n");

        // close file
        fclose(pFile);

        // update mCurrentIndex and mLength
        DataBuff.mCurrentIndex = uiRemoveIndex;
        DataBuff.mLength -= uiRemoveNum;
        printf("Current Index = %d\n", DataBuff.mCurrentIndex);

        // print out
        printf("Removed. Length = %d\n", DataBuff.mLength);

        // release mutex
        pthread_mutex_unlock(&lock);

        // add sleep to see the data easier
        usleep(2000000);
    }

    pthread_exit(NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/// main()
///
//////////////////////////////////////////////////////////////////////////////////////////////////
int main (void)
{
    printf ("Hello Embed\n");

    // reset data buffer
    pthread_mutex_lock(&lock);
    memset(DataBuff.mData, 0, sizeof(BUFF_SIZE));
    DataBuff.mCurrentIndex = 0;
    DataBuff.mLength = 0;
    pthread_mutex_unlock(&lock);

    int rc;

    pthread_t pThreadGenerate;
    rc = pthread_create(&pThreadGenerate, NULL, generateData, NULL);
    if (rc) {
        printf ("Unable to create thread\n");
        return -1;
    }

    pthread_t pThreadRemove;
    rc = pthread_create(&pThreadRemove, NULL, removeData, NULL);
    if (rc) {
        printf ("Unable to create thread\n");
        return -1;
    }


    while(1);
    
    return 0;
}