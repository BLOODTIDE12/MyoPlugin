#pragma once

#include "GameFramework/Actor.h"
#include "MyoDelegateBlueprint.h"
#include "MyoPluginActor.generated.h"


/**
 * Placeable Actor that receives Myo input and pose updates. 
 * Use or Extend to receive event notifications.
 */
UCLASS()
class AMyoPluginActor : public AActor, public MyoDelegateBlueprint
{
	GENERATED_UCLASS_BODY()
	/*
	* (BUG) NB: blueprint functions and events do not support u/int64!
	* Timestamp behaviour currently undefined
	*/

	/**
	* Event on a Myo detecting a pose.
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	* @param pose (out): 0 = rest, 1 = fist, 2 = waveIn, 3 = waveOut, 4 = fingersSpread, 5 = reserved1, 6 = thumbToPinky, 7 = unknown
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnPose(int32 myoId, int32 timestamp, int32 pose);

	/**
	* On arm detection, typically after a wave out gesture
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	* @param arm (out) 0 = right, 1 = left, 2 = unknown
	* @param direction (out) 0 = toward wrist, 1 = toward elbow, 2 = unknown
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnArmRecognized(int32 myoId, int32 timestamp, int32 arm, int32 direction);

	/**
	* Called when a myo has been removed from arm.
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnArmLost(int32 myoId, int32 timestamp);

	/**
	* Event on a Myo connecting
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnConnect(int32 myoId, int32 timestamp);

	/**
	* Event on a Myo disconnecting
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnDisconnect(int32 myoId, int32 timestamp);

	/**
	* Event on a Myo pairing
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnPair(int32 myoId, int32 timestamp);

	/**
	* Event on a Myo receiving orientation data, typically each frame.
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	* @param orientation (out) orientation in rotator format.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnOrientationData(int32 myoId, int32 timestamp, FRotator orientation);

	/**
	* Event on a Myo receiving acceleration data, typically each frame.
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	* @param acceleration (out) acceleration in units of g.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnAccelerometerData(int32 myoId, int32 timestamp, FVector acceleration);

	/**
	* Event on a Myo receiving gyroscope data, typically each frame.
	* @param myoId (out) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param timestamp (out) truncated from 64bit
	* @param gyro (out) gyroscope vector in deg/s.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void OnGyroscopeData(int32 myoId, int32 timestamp, FVector gyro);

	/**
	* Called when a problem occurs such as bluetooth usb device not being detected (unplugged).
	* If you receive this event further Myo events will not work until you plug in the hub and restart the application.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = MyoEvents)
		void DeviceDisabled();

	/**
	* Vibrate the specified Myo with type intensity
	* @param myoId (in) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param type (in) Short=0,Medium=1,Long=2
	*/
	UFUNCTION(BlueprintCallable, Category = MyoFunctions)
		void VibrateDevice(int32 myoId, int32 type);

	/**
	* Check if Myo Hub is enabled, if its not it will remain so until restart. 
	*/
	UFUNCTION(BlueprintCallable, Category = MyoFunctions)
		bool IsHubEnabled();

	/**
	* Returns the latest Myo data available for polling
	* @param myoId (in) starting from 1, based on pairing priority. Call Which Arm (myoId) to figure out which arm it belongs to.
	* @param pose (out) 0 = rest, 1 = fist, 2 = waveIn, 3 = waveOut, 4 = fingersSpread, 5 = reserved1, 6 = thumbToPinky, 7 = unknown
	* @param acceleration (out) acceleration in units of g.
	* @param orientation (out) orientation in rotator format.
	* @param gyro (out) gyroscope vector in deg/s.
	* @param arm (out) 0 = right, 1 = left, 2 = unknown
	*/
	UFUNCTION(BlueprintCallable, Category = MyoFunctions)
		void LatestData(int32 myoId, int32& pose, FVector& Acceleration, FRotator& Rotation, FVector& Gyro, int32& Arm);
	
	/** 
	* Determine Which Arm the Myo is on.
	* @param myoId (in) starting from 1, based on pairing priority.
	* @param arm (out) 0 = right, 1 = left, 2 = unknown
	*/
	UFUNCTION(BlueprintCallable, Category = MyoFunctions)
		void WhichArm(int32 myoId, int32& Arm);

	/**
	* Get the Myo ID which is currently recognized on the left arm, returns available = false and MyoID = -1 when no myo is recognized for the left arm.
	* @param available (out) whether left arm has recognized a myo
	* @param myoId (out) starting from 1, based on pairing priority, returns -1 when unavailable.
	*/
	UFUNCTION(BlueprintCallable, Category = MyoFunctions)
	virtual void LeftMyoId(bool& available, int32& myoId);

	/**
	* Get the Myo ID which is currently recognized on the right arm, returns available = false and MyoID = -1 when no myo is recognized for the right arm.
	* @param available (out) whether left arm has recognized a myo
	* @param myoId (out) starting from 1, based on pairing priority, returns -1 when unavailable.
	*/
	UFUNCTION(BlueprintCallable, Category = MyoFunctions)
	virtual void RightMyoId(bool& available, int32& myoId);

	/**
	* Convenience function to re-convert orientation given in UE format back into Myo format.
	* @param orientation (in) in orientation, UE format.
	* @param converted (out) out orientation, raw myo format.
	*/
	UFUNCTION(BlueprintCallable, Category = MyoFunctions)
	virtual void ConvertToRawOrientation(FRotator orientation, FRotator& converted);


	//Required for setting delegate (startup) and forwarding Tick to the Myo Delegate.
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};


