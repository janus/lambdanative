/*
LambdaNative - a cross-platform Scheme framework
Copyright (c) 2009-2013, University of British Columbia
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the following
disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials
provided with the distribution.

* Neither the name of the University of British Columbia nor
the names of its contributors may be used to endorse or
promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// glue MoMu C++ into C subsystem

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// audio

#import "config.h"

#import "mo_audio.h"
#import <MediaPlayer/MediaPlayer.h>
#import <AudioToolbox/AudioToolbox.h>

int iphone_audioroute = 0;

extern "C" int iphone_realtime_audio_init(double srate, unsigned int framesize){
  bool result;
  result = MoAudio::init(srate, framesize, 2);
  return (!result?0:1);
}

extern "C" int iphone_realtime_audio_start(MoCallback cb){
  bool result;
  result = MoAudio::start(cb, NULL);
  return (!result?0:1);
}

extern "C" int iphone_realtime_audio_stop(){
  MoAudio::stop();
  return 1;
}

extern "C" void iphone_setvolume(double v){
  [[MPMusicPlayerController applicationMusicPlayer] setVolume:v];
}

extern "C" double iphone_getvolume(){
  Float32 volume=0;
  UInt32 dataSize = sizeof(Float32);
  AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareOutputVolume,&dataSize,&volume);
  return (double)volume;
}

extern "C" int iphone_headphonepresent(){
  UInt32 routeSize = sizeof (CFStringRef);
  CFStringRef route;
  OSStatus error = AudioSessionGetProperty (kAudioSessionProperty_AudioRoute, &routeSize, &route);
  if (!error && (route != NULL)) {
    NSString* routeStr = (NSString*)route;
    NSRange headphoneRange = [routeStr rangeOfString : @"Head"];
    if (headphoneRange.location != NSNotFound) return 1;
  }
  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// accelerometer

#import "mo_accel.h"

extern "C" double ios_accel_getx() { return MoAccel::getX(); }
extern "C" double ios_accel_gety() { return MoAccel::getY(); }
extern "C" double ios_accel_getz() { return MoAccel::getZ(); }

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// location

#import "mo_location.h"

extern "C" double ios_location_getlatitude(){
  CLLocation *tmp = MoLocation::getLocation();
  return tmp.coordinate.latitude;
}

extern "C" double ios_location_getlongitude(){
  CLLocation *tmp = MoLocation::getLocation();
  return tmp.coordinate.longitude;
}

extern "C" double ios_location_getaltitude(){
  CLLocation *tmp = MoLocation::getLocation();
  return tmp.altitude;
}

extern "C" double ios_location_getaccuracy(){
  CLLocation *tmp = MoLocation::getLocation();
  return tmp.horizontalAccuracy;
}

extern "C" int ios_location_gettimestamp(){
  CLLocation *tmp = MoLocation::getLocation();
  return (int)[tmp.timestamp timeIntervalSince1970];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// launch url

extern "C" void ios_launch_url(char *urlchar){
  NSString* urlString = [NSString stringWithUTF8String: urlchar];
  NSURL *url = [NSURL URLWithString:urlString];
  [[UIApplication sharedApplication] openURL:url];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// gyroscope

#ifdef USE_GYROSCOPE
#import <CoreMotion/CoreMotion.h>
#endif

static double ios_gyro(int idx)
{
  double value=1.0;
#ifdef USE_GYROSCOPE
  static int needsinit=1;
  static CMMotionManager *motionManager;
  if (needsinit) {
    motionManager = [[CMMotionManager alloc] init];
    motionManager.deviceMotionUpdateInterval = 1.0/60.0;
    if  (motionManager.isDeviceMotionAvailable) {
      [motionManager startDeviceMotionUpdates];
    }
    needsinit=0;
  }
  CMDeviceMotion *deviceMotion = motionManager.deviceMotion;      
  CMAttitude *attitude = deviceMotion.attitude;
  if (idx==0) value=(double)attitude.yaw; 
  if (idx==1) value=(double)attitude.pitch; 
  if (idx==2) value=(double)attitude.roll; 
#endif
  return value;
}

extern "C" double ios_gyro_yaw() { return ios_gyro(0); }
extern "C" double ios_gyro_pitch() { return ios_gyro(1); }
extern "C" double ios_gyro_roll() { return ios_gyro(2); }

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// device orientation

/*
extern "C" int iphone_device_orientation(){
  static int orientation=0;
  static int start_flag=1;
   UIDevice *dev = [UIDevice currentDevice];
   if (start_flag) {
     [dev beginGeneratingDeviceOrientationNotifications];
     start_flag=0;
  }
  switch ([dev orientation]) {
    case UIDeviceOrientationPortrait:
      orientation=0; break;
    case UIDeviceOrientationLandscapeLeft:
      orientation=1; break;
    case UIDeviceOrientationLandscapeRight:
      orientation=2; break;
    case UIDeviceOrientationPortraitUpsideDown:
      orientation=3; break;
    case UIDeviceOrientationUnknown:
    case UIDeviceOrientationFaceUp:
    case UIDeviceOrientationFaceDown:
    default:
      break;
  }
  return orientation;
}
*/

// eof
