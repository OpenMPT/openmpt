// 3DL2.H
//

#ifndef _3DL2_H_INCLUDED
#define _3DL2_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#pragma pack(push, 4)

// I3DL2 listener property set {DA0F0520-300A-11D3-8A2B-0060970DB011}
DEFINE_GUID(DSPROPSETID_I3DL2_ListenerProperties,
    0xDA0F0520,
    0x300A,
    0x11D3,
    0x8A, 0x2B, 0x00, 0x60, 0x97, 0x0D, 0xB0, 0x11);

typedef enum
{
// sets all I3DL2 listener properties
    DSPROPERTY_I3DL2LISTENER_ALL,
// room effect level at low frequencies
    DSPROPERTY_I3DL2LISTENER_ROOM,
// room effect high-frequency level re. low frequency level
    DSPROPERTY_I3DL2LISTENER_ROOMHF,
// like DS3D flRolloffFactor but for room effect
    DSPROPERTY_I3DL2LISTENER_ROOMROLLOFFFACTOR,
// reverberation decay time at low-frequencies
    DSPROPERTY_I3DL2LISTENER_DECAYTIME,
// high-frequency to low-frequency decay time ratio
    DSPROPERTY_I3DL2LISTENER_DECAYHFRATIO,
// early reflections level relative to room effect
    DSPROPERTY_I3DL2LISTENER_REFLECTIONS,
// delay time of first reflection
    DSPROPERTY_I3DL2LISTENER_REFLECTIONSDELAY,
// late reverberation level relative to room effect
    DSPROPERTY_I3DL2LISTENER_REVERB, 
// late reverberation delay time relative to first reflection
    DSPROPERTY_I3DL2LISTENER_REVERBDELAY,
// reverberation diffusion (echo density)
    DSPROPERTY_I3DL2LISTENER_DIFFUSION,
// reverberation density (modal density)
    DSPROPERTY_I3DL2LISTENER_DENSITY,
// reference high frequency
    DSPROPERTY_I3DL2LISTENER_HFREFERENCE
} DSPROPERTY_I3DL2_LISTENERPROPERTY;

// use this structure for DSPROPERTY_I3DL2LISTENER_ALL
// - all levels are hundredths of decibels (mB)
// - all times are in seconds (s)
typedef struct _I3DL2_LISTENERPROPERTIES
{
    LONG lRoom;                    // [-10000, 0]      default: -10000 mB
    LONG lRoomHF;                  // [-10000, 0]      default: 0 mB
    FLOAT flRoomRolloffFactor;     // [0.0, 10.0]      default: 0.0
    FLOAT flDecayTime;             // [0.1, 20.0]      default: 1.0 s
    FLOAT flDecayHFRatio;          // [0.1, 2.0]       default: 0.5
    LONG lReflections;             // [-10000, 1000]   default: -10000 mB
    FLOAT flReflectionsDelay;      // [0.0, 0.3]       default: 0.02 s
    LONG lReverb;                  // [-10000, 2000]   default: -10000 mB
    FLOAT flReverbDelay;           // [0.0, 0.1]       default: 0.04 s
    FLOAT flDiffusion;             // [0.0, 100.0]     default: 100.0 %
    FLOAT flDensity;               // [0.0, 100.0]     default: 100.0 %
    FLOAT flHFReference;           // [20.0, 20000.0]  default: 5000.0 Hz
} I3DL2_LISTENERPROPERTIES, *LPI3DL2_LISTENERPROPERTIES;

// property ranges and defaults:

#define I3DL2LISTENER_MINROOM                       (-10000)
#define I3DL2LISTENER_MAXROOM                       0
#define I3DL2LISTENER_DEFAULTROOM                   (-10000)

#define I3DL2LISTENER_MINROOMHF                     (-10000)
#define I3DL2LISTENER_MAXROOMHF                     0
#define I3DL2LISTENER_DEFAULTROOMHF                 0

#define I3DL2LISTENER_MINROOMROLLOFFFACTOR          0.0f
#define I3DL2LISTENER_MAXROOMROLLOFFFACTOR          10.0f
#define I3DL2LISTENER_DEFAULTROOMROLLOFFFACTOR      0.0f

#define I3DL2LISTENER_MINDECAYTIME                  0.1f
#define I3DL2LISTENER_MAXDECAYTIME                  20.0f
#define I3DL2LISTENER_DEFAULTDECAYTIME              1.0f

#define I3DL2LISTENER_MINDECAYHFRATIO               0.1f
#define I3DL2LISTENER_MAXDECAYHFRATIO               2.0f
#define I3DL2LISTENER_DEFAULTDECAYHFRATIO           0.5f

#define I3DL2LISTENER_MINREFLECTIONS                (-10000)
#define I3DL2LISTENER_MAXREFLECTIONS                1000
#define I3DL2LISTENER_DEFAULTREFLECTIONS            (-10000)

#define I3DL2LISTENER_MINREFLECTIONSDELAY           0.0f
#define I3DL2LISTENER_MAXREFLECTIONSDELAY           0.3f
#define I3DL2LISTENER_DEFAULTREFLECTIONSDELAY       0.02f

#define I3DL2LISTENER_MINREVERB                     (-10000)
#define I3DL2LISTENER_MAXREVERB                     2000
#define I3DL2LISTENER_DEFAULTREVERB                 (-10000)

#define I3DL2LISTENER_MINREVERBDELAY                0.0f
#define I3DL2LISTENER_MAXREVERBDELAY                0.1f
#define I3DL2LISTENER_DEFAULTREVERBDELAY            0.04f

#define I3DL2LISTENER_MINDIFFUSION                  0.0f
#define I3DL2LISTENER_MAXDIFFUSION                  100.0f
#define I3DL2LISTENER_DEFAULTDIFFUSION              100.0f

#define I3DL2LISTENER_MINDENSITY                    0.0f
#define I3DL2LISTENER_MAXDENSITY                    100.0f
#define I3DL2LISTENER_DEFAULTDENSITY                100.0f

#define I3DL2LISTENER_MINHFREFERENCE                20.0f
#define I3DL2LISTENER_MAXHFREFERENCE                20000.0f
#define I3DL2LISTENER_DEFAULTHFREFERENCE            5000.0f

// I3DL2 buffer property set {DA0F0521-300A-11D3-8A2B-0060970DB011}
DEFINE_GUID(DSPROPSETID_I3DL2_BufferProperties,
    0xDA0F0521,
    0x300A,
    0x11D3,
	0x8A, 0x2B, 0x00, 0x60, 0x97, 0x0D, 0xB0, 0x11);

typedef enum
{
// sets all I3DL2 buffer properties
    DSPROPERTY_I3DL2BUFFER_ALL,
// sets both obstruction properties
    DSPROPERTY_I3DL2BUFFER_OBSTRUCTIONALL,
// sets both occlusion properties
    DSPROPERTY_I3DL2BUFFER_OCCLUSIONALL,
// additional direct path level correction
    DSPROPERTY_I3DL2BUFFER_DIRECT,
// additional direct path high-frequency re. low-frequency level correction
    DSPROPERTY_I3DL2BUFFER_DIRECTHF,
// additional room effect level correction
    DSPROPERTY_I3DL2BUFFER_ROOM,
// additional room effect high-frequency re. low-frequency level correction
    DSPROPERTY_I3DL2BUFFER_ROOMHF,
// like DS3D flRolloffFactor but for room effect
    DSPROPERTY_I3DL2BUFFER_ROOMROLLOFFFACTOR,
// main obstruction control (attenuation at high frequencies)
    DSPROPERTY_I3DL2BUFFER_OBSTRUCTION,
// obstruction low-frequency re. high-frequency ratio
    DSPROPERTY_I3DL2BUFFER_OBSTRUCTIONLFRATIO,
// main occlusion control (attenuation at high frequencies)
    DSPROPERTY_I3DL2BUFFER_OCCLUSION,
// occlusion low-frequency re. high-frequency ratio
    DSPROPERTY_I3DL2BUFFER_OCCLUSIONLFRATIO
} DSPROPERTY_I3DL2_BUFFERPROPERTY;

// use this structure for DSPROPERTY_I3DL2BUFFER_OBSTRUCTIONALL
// - all levels are hundredths of decibels (mB)
typedef struct _I3DL2_OBSTRUCTIONPROPERTIES
{
    LONG lHFLevel;              // [-10000, 0] default: 0 mB
    FLOAT flLFRatio;            // [0.0, 1.0]  default: 0.0
} I3DL2_OBSTRUCTIONPROPERTIES, *LPI3DL2_OBSTRUCTIONPROPERTIES;

// use this structure for DSPROPERTY_I3DL2BUFFER_OCCLUSIONALL
// - all levels are hundredths of decibels (mB)
typedef struct _I3DL2_OCCLUSIONPROPERTIES
{
    LONG lHFLevel;              // [-10000, 0] default: 0 mB
    FLOAT flLFRatio;            // [0.0, 1.0]  default: 0.25
} I3DL2_OCCLUSIONPROPERTIES, *LPI3DL2_OCCLUSIONPROPERTIES;

// use this structure for DSPROPERTY_I3DL2BUFFER_ALL
// - all levels are hundredths of decibels (mB)
typedef struct _I3DL2_BUFFERPROPERTIES
{
    LONG lDirect;                // [-10000, 1000] default: 0 mB
    LONG lDirectHF;              // [-10000, 0]    default: 0 mB
    LONG lRoom;                  // [-10000, 1000] default: 0 mB
    LONG lRoomHF;                // [-10000, 0]    default: 0 mB
    FLOAT flRoomRolloffFactor;   // [0.0, 10.0]    default: 0.0
    I3DL2_OBSTRUCTIONPROPERTIES Obstruction;
    I3DL2_OCCLUSIONPROPERTIES Occlusion;
} I3DL2_BUFFERPROPERTIES, *LPI3DL2_BUFFERPROPERTIES;

// property ranges and defaults:

#define I3DL2BUFFER_MINDIRECT                  (-10000)
#define I3DL2BUFFER_MAXDIRECT                  1000
#define I3DL2BUFFER_DEFAULTDIRECT              0

#define I3DL2BUFFER_MINDIRECTHF                (-10000)
#define I3DL2BUFFER_MAXDIRECTHF                0
#define I3DL2BUFFER_DEFAULTDIRECTHF            0

#define I3DL2BUFFER_MINROOM                    (-10000)
#define I3DL2BUFFER_MAXROOM                    1000
#define I3DL2BUFFER_DEFAULTROOM                0

#define I3DL2BUFFER_MINROOMHF                  (-10000)
#define I3DL2BUFFER_MAXROOMHF                  0
#define I3DL2BUFFER_DEFAULTROOMHF              0

#define I3DL2BUFFER_MINROOMROLLOFFFACTOR       0.0f
#define I3DL2BUFFER_MAXROOMROLLOFFFACTOR       10.f
#define I3DL2BUFFER_DEFAULTROOMROLLOFFFACTOR   0.0f

#define I3DL2BUFFER_MINOBSTRUCTION             (-10000)
#define I3DL2BUFFER_MAXOBSTRUCTION             0
#define I3DL2BUFFER_DEFAULTOBSTRUCTION         0

#define I3DL2BUFFER_MINOBSTRUCTIONLFRATIO      0.0f
#define I3DL2BUFFER_MAXOBSTRUCTIONLFRATIO      1.0f
#define I3DL2BUFFER_DEFAULTOBSTRUCTIONLFRATIO  0.0f

#define I3DL2BUFFER_MINOCCLUSION               (-10000)
#define I3DL2BUFFER_MAXOCCLUSION               0
#define I3DL2BUFFER_DEFAULTOCCLUSION           0

#define I3DL2BUFFER_MINOCCLUSIONLFRATIO        0.0f
#define I3DL2BUFFER_MAXOCCLUSIONLFRATIO        1.0f
#define I3DL2BUFFER_DEFAULTOCCLUSIONLFRATIO    0.25f

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
