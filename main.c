#include <stdio.h>
#include <math.h>
#include <portaudio.h>

// set to 2 for stereo input/output
#define CHANNELS (1)
// set to 44100 as default
#define SAMPLE_RATE (44100)
#define FRAME_RATE (128)

typedef float SAMPLE;
typedef struct
{
    float mono_phase;
}   
paTestData;

// REVERSE
// we will use a CIRCULAR buffer:
    // - think circular array, but ALSO sliding window
    // - there is a READ (delayed output) pointer/index and a WRITE (input) pointer/index
    // - both indices loop to start of buffer once end is reached
// Variables for REVERSE
int const revMaxLen = SAMPLE_RATE*5; // max recording length (by setting to SAMPLE_RATE*5, we get 5 seconds)
float buffer[revMaxLen]; // the buffer (an array of floats, since floats are what will be stored)
// for a dynamic buffer: float *buffer = malloc(SAMPLE_RATE * sizeof(float));
int index = 0; // index of input signal in buffer, where audio is being WRITTEN
int reverseIndex = 0; // keeps track of how long user has held down the reverse switch for
char reverseOn = 0; // needs to be mapped to physical hardware input

// input - signal, preferrably the effected one, not the dry one
// toggle - 1 for WRITING FORWARDS, -1 for READING in REVERSE, 0 for OFF
static float reverse(float input, int toggle){
    float output;    
    if (toggle == 1) {
        // WRITE FORWARDS
        buffer[index] = input; // WRITE input signal into reverse buffer
        index++;
        index %= revMaxLen; // ensures circular behavior
    } else if (toggle == -1){
        // READ BACKWARDS
        output = buffer[index]; // READ from saved buffer into output
        index--;
        index += revMaxLen; // since moving in negative direction
        index %= revMaxLen; // ensures circular behavior
    }
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
static int PaPedalCallback( const void *input, // the buffer allocated for input
                                      void *output, // the buffer allocated for output
                                      unsigned long framesPerBuffer,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData ) { // the input signal
    paTestData *data = (paTestData*)userData;
    SAMPLE *out = (SAMPLE*)output;
    const SAMPLE *in = (const SAMPLE*)input;
    unsigned int i;
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) userData;

    if( input == NULL ){
        for( i=0; i<framesPerBuffer; i++ ){
            *out++ = 0;  /* mono - silent */
        }
        gNumNoInputs += 1;
    }
    else{
        for( i=0; i<framesPerBuffer; i++){
            SAMPLE sample = in[i]; // MONO input

            out[i] = 0; // clearing the data stored here
            if (reverseOn) {
                reverseIndex++;
                out[i] = reverse(sample, 1);
            } else if (reverseIndex < 0) {
                reverseIndex--;
                out[i] = reverse(sample, -1);
            }
            

            
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
    int deviceChoice = -1;

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
    for( int i=0; i<numDevices; i++ ){
        deviceInfo = Pa_GetDeviceInfo( i );
        printf("Device %d: %s\n", i, deviceInfo->name); 
        // there are other info that can be obtained, but for testing purposes I just need the name
    }

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

    double srate = SAMPLE_RATE;
    
    unsigned long framesPerBuffer = FRAME_RATE; //could be paFramesPerBufferUnspecified, in which case PortAudio will do its best to manage it for you, but, on some platforms, the framesPerBuffer will change in each call to the callback
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
