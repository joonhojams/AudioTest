#include <stdio.h>
#include <math.h>
#include <portaudio.h>

// set to 2 for stereo input/output
#define CHANNELS (1)

typedef float SAMPLE;
typedef struct
{
    float mono_phase;
}   
paTestData;

static float distortion(float input){
    float output;
    input *= 100.0f;

    // output = input + powf(input,3.0f)/3.0f;
    if(input > 0){
        output = 1.0f - expf(-input);
    }else{
        output = -1.0f + expf(input);
    }

    output /= 100.0f;
    printf("%f -> %f\n", input, output);

    return output;
}


// unfortunately I have no idea what this is, stole it from pa_fuzz.c
static int gNumNoInputs = 0;

// a callback function is called by the PortAudio engine whenever audio is captured or is needed.
// MUST return an int and accept the EXACT parameters as shown below.
// From PortAudio docs:
/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/

static int PaPedalCallback( const void *input,
                                      void *output,
                                      unsigned long framesPerBuffer,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData ) {
    paTestData *data = (paTestData*)userData;
    SAMPLE *out = (SAMPLE*)output;
    const SAMPLE *in = (const SAMPLE*)input;
    unsigned int i;
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) userData;

    if( input == NULL )
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *out++ = 0;  /* mono - silent */
        }
        gNumNoInputs += 1;
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            SAMPLE sample = *in++; /* MONO input */
            *out++ = distortion(sample); /* mono - distorted */

            // delay/reverb?
            // *(out + framesPerBuffer - i - 1) += distortion(sample)*0.7f;
        }
    }
    return paContinue;
}

int inputAudioDevice =  -1; // audio device to stream input from
int outputAudioDevice =  -1; // audio device to stream output to
static paTestData data;


// Selecting an audio device if one is not already selected
// Returns an int representing which number the device is according to PortAudio
int deviceSelect(void){
    PaError err;

    // Initialize PortAudio, scans available devices
    err = Pa_Initialize(); 
    if( err != paNoError ) goto error; // goto "jumps" to specified code (line 22)

    // Query for the number of devices found during initialization
    int numDevices;
    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 )
    {
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
        err = numDevices;
        goto error;
    }
    // Get device info
    const   PaDeviceInfo *deviceInfo;
    for( int i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );
        printf("Device %d: %s\n", i, deviceInfo->name); 
        // there are other info that can be obtained, but for testing purposes I just need the name
    }

    int deviceChoice = -1;

    // char ch;
    // while ((ch = getchar()) != '\n' && ch != EOF); // clearing input
    while(deviceChoice == -1){
        scanf("%d", &deviceChoice);
    }
    return deviceChoice;

    error:
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    return -1;
}


// gcc main.c -o main -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio
// ./main
int main(void);
int main(void) {
    printf("Hello from PortAudio setup!\n");

    PaError err;
    PaStream *stream;
    
    // Initialize PortAudio, scans available devices
    err = Pa_Initialize(); 
    if( err != paNoError ) goto error; // goto "jumps" to specified code (line 22)

    if(inputAudioDevice == -1 || inputAudioDevice == paNoDevice){
        inputAudioDevice = deviceSelect(); 
    }
    if(outputAudioDevice == -1 || outputAudioDevice == paNoDevice){
        outputAudioDevice = deviceSelect(); 
    }

    double srate = Pa_GetDeviceInfo(inputAudioDevice)->defaultSampleRate;
    
    unsigned long framesPerBuffer = paFramesPerBufferUnspecified ; //could be paFramesPerBufferUnspecified, in which case PortAudio will do its best to manage it for you, but, on some platforms, the framesPerBuffer will change in each call to the callback
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    // bzero( &inputParameters, sizeof( inputParameters ) ); //not necessary if you are filling in all the fields
    inputParameters.channelCount = CHANNELS;
    inputParameters.device = inputAudioDevice;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputAudioDevice)->defaultLowInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field
    // bzero( &outputParameters, sizeof( outputParameters ) ); //not necessary if you are filling in all the fields
    outputParameters.channelCount = CHANNELS;
    outputParameters.device = outputAudioDevice;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputAudioDevice)->defaultLowOutputLatency ;
    outputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field

    // Open an audio I/O stream.
    err = Pa_OpenStream(
                    &stream,
                    &inputParameters,
                    &outputParameters,
                    srate,
                    framesPerBuffer,
                    paNoFlag, //flags that can be used to define dither, clip settings and more
                    PaPedalCallback, //your callback function
                    &data ); //data to be passed to callback. In C++, it is frequently (void *)this
    if( err != paNoError ) goto error;

    // Start audio I/O stream
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    // Looks like this is the easiest way to keep program running (like a while loop)
    char ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
    printf("Hit ENTER to stop program.\n");
    getchar();

    // Stop audio I/O stream
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    // Close audio I/O stream to save resources
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    // Terminate PortAudio
    err = Pa_Terminate(); 
    if( err != paNoError ){
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    }
    return 0;

    error: // the line specified in goto (line 14)
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    return -1;
}
