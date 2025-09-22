#include <stdio.h>
#include <math.h>
#include <portaudio.h>

int inputAudioDevice =  -1; // audio device to stream input from
int outputAudioDevice =  -1; // audio device to stream output to

/* (from PortAudio docs)
Blocking I/O works in much the same way as the callback method except that instead of providing a function to provide (or consume) audio data, 
you must feed data to (or consume data from) PortAudio at regular intervals, usually inside a loop. 
The example below, excepted from patest_read_write_wire.c, shows how to open the default device, and pass data from its input to its output 
for a set period of time. Note that we use the default high latency values to help avoid underruns since we are usually reading and writing 
audio data from a relatively low priority thread, and there is usually extra buffering required to make blocking I/O work.

Note that not all API's implement Blocking I/O at this point, so for maximum portability or performance, you'll still want to use callbacks.
*/

int blocking(void){
    PaError err;

    /* -- initialize PortAudio -- */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    /* -- setup input and output -- */

    if(inputAudioDevice == -1){
        inputAudioDevice = deviceSelect(); 
    }
    if(outputAudioDevice == -1){
        outputAudioDevice = deviceSelect(); 
    }

    double srate = Pa_GetDeviceInfo(inputAudioDevice)->defaultSampleRate;
    PaStream *stream;
    unsigned long framesPerBuffer = paFramesPerBufferUnspecified ;
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    inputParameters.device = inputAudioDevice;
    inputParameters.channelCount = Pa_GetDeviceInfo(inputAudioDevice)->maxInputChannels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.device = outputAudioDevice;
    outputParameters.channelCount = Pa_GetDeviceInfo(outputAudioDevice)->maxInputChannels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    /* -- setup stream -- */
    err = Pa_OpenStream(
            &stream,
            &inputParameters,
            &outputParameters,
            srate,
            framesPerBuffer,
            paClipOff,      /* we won't output out of range samples so don't bother clipping them */
            NULL, /* no callback, use blocking API */
            NULL ); /* no callback, so no callback userData */
    if( err != paNoError ) goto error;
    /* -- start stream -- */
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    printf("Wire on. Will run one minute.\n"); fflush(stdout);
    /* -- Here's the loop where we pass data from input to output -- */
    for( i=0; i<(60*srate)/framesPerBuffer; ++i )
    {
    err = Pa_WriteStream( stream, sampleBlock, framesPerBuffer );
    if( err ) goto xrun;
    err = Pa_ReadStream( stream, sampleBlock, framesPerBuffer );
    if( err ) goto xrun;
    }
    /* -- Now we stop the stream -- */
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    /* -- don't forget to cleanup! -- */
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    Pa_Terminate();
    return 0;

    error:
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