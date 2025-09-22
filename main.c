#include <stdio.h>
#include <math.h>
#include <portaudio.h>

// a callback function is called by the PortAudio engine whenever audio is captured or is needed
// MUST return an int and accept the EXACT parameters as shown below
static int PaStreamCallback( const void *input,
                                      void *output,
                                      unsigned long frameCount,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData ) ;

                                      
typedef struct
{
    float mono_phase;
}   
paTestData;

int inputAudioDevice =  -1; // audio device to stream input from
int outputAudioDevice =  -1; // audio device to stream output to
static paTestData data;

// gcc main.c -o main -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio
// ./main

int main(void) {
    printf("Hello from PortAudio setup!\n");

    PaError err;

    // Initialize PortAudio, scans available devices
    err = Pa_Initialize(); 
    if( err != paNoError ) goto error; // goto "jumps" to specified code (line 22)

    if(inputAudioDevice == -1){
        inputAudioDevice = deviceSelect(); 
    }
    if(outputAudioDevice == -1){
        outputAudioDevice = deviceSelect(); 
    }

    double srate = Pa_GetDeviceInfo(inputAudioDevice)->defaultSampleRate;
    PaStream *stream;
    unsigned long framesPerBuffer = paFramesPerBufferUnspecified ; //could be paFramesPerBufferUnspecified, in which case PortAudio will do its best to manage it for you, but, on some platforms, the framesPerBuffer will change in each call to the callback
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    bzero( &inputParameters, sizeof( inputParameters ) ); //not necessary if you are filling in all the fields
    inputParameters.channelCount = Pa_GetDeviceInfo(inputAudioDevice)->maxInputChannels;
    inputParameters.device = inputAudioDevice;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputAudioDevice)->defaultLowInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field
    bzero( &outputParameters, sizeof( outputParameters ) ); //not necessary if you are filling in all the fields
    outputParameters.channelCount = Pa_GetDeviceInfo(outputAudioDevice)->maxOutputChannels;
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
                    PaStreamCallback, //your callback function
                    &data ); //data to be passed to callback. In C++, it is frequently (void *)this
    if( err != paNoError ) goto error;

    // Start audio I/O stream
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;



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



// Selecting an audio device if one is not already selected
// Returns an int representing which number the device is according to PortAudio
int deviceSelect(){
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
        printf("Device %s: %s", (i, deviceInfo->name)); 
        // there are other info that can be obtained, but for testing purposes I just need the name
    }

    const deviceChoice = -1;
    while(deviceChoice == -1){
        scanf("%d", &deviceChoice);
    }
    return deviceChoice;

    error:
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    return -1;
}