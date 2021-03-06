#include "MyoPluginPrivatePCH.h"

#include "IMyoPlugin.h"
#include "FMyoPlugin.h"
#include "MyoDelegate.h"
#include "MyoDelegateBlueprint.h"
#include "Slate.h"

#include <iostream>
#include <stdexcept>
#include <vector>

IMPLEMENT_MODULE(FMyoPlugin, MyoPlugin)

#define LOCTEXT_NAMESPACE "MyoPlugin"
#define ORIENTATION_SCALE_PITCH 0.01111111111	//1/90
#define ORIENTATION_SCALE_YAWROLL 0.00555555555 //1/180
#define GYRO_SCALE 0.02222222222				//1/45

//Private API - This is where the magic happens

//String Conversion to UE4
TCHAR* tcharFromStdString(std::string str){
	TCHAR *param = new TCHAR[str.size() + 1];
	param[str.size()] = 0;
	std::copy(str.begin(), str.end(), param);
	return param;
}
FRotator convertOrientationToUE(FRotator rawOrientation)
{
	FRotator convertedOrientation;
	convertedOrientation.Pitch = rawOrientation.Pitch*-1.f;
	convertedOrientation.Yaw = rawOrientation.Yaw*-1.f;
	convertedOrientation.Roll = rawOrientation.Roll;
	return convertedOrientation;
}

//Define each FKey const in a .cpp so we can compile
const FKey EKeysMyo::MyoPoseRest("MyoPoseRest");
const FKey EKeysMyo::MyoPoseFist("MyoPoseFist");
const FKey EKeysMyo::MyoPoseWaveIn("MyoPoseWaveIn");
const FKey EKeysMyo::MyoPoseWaveOut("MyoPoseWaveOut");
const FKey EKeysMyo::MyoPoseFingersSpread("MyoPoseFingersSpread");
const FKey EKeysMyo::MyoPoseThumbToPinky("MyoPoseThumbToPinky");
const FKey EKeysMyo::MyoPoseUnknown("MyoPoseUnknown");
const FKey EKeysMyo::MyoAccelerationX("MyoAccelerationX");
const FKey EKeysMyo::MyoAccelerationY("MyoAccelerationY");
const FKey EKeysMyo::MyoAccelerationZ("MyoAccelerationZ");
const FKey EKeysMyo::MyoOrientationPitch("MyoOrientationPitch");
const FKey EKeysMyo::MyoOrientationYaw("MyoOrientationYaw");
const FKey EKeysMyo::MyoOrientationRoll("MyoOrientationRoll");
const FKey EKeysMyo::MyoGyroX("MyoGyroX");
const FKey EKeysMyo::MyoGyroY("MyoGyroY");
const FKey EKeysMyo::MyoGyroZ("MyoGyroZ");

class DataCollector : public myo::DeviceListener {
public:
	DataCollector(){
		myoDelegate = NULL;
		Enabled = false;
		lastPose = myo::Pose::unknown;
	}

	//Inputmapping
	void PressPose(myo::Pose pose){
		if (pose == myo::Pose::rest)
		{
			FSlateApplication::Get().OnControllerButtonPressed(EKeysMyo::MyoPoseRest, 0, 0);
		}
		else if (pose == myo::Pose::fist)
		{
			FSlateApplication::Get().OnControllerButtonPressed(EKeysMyo::MyoPoseFist, 0, 0);
		}
		else if (pose == myo::Pose::waveIn)
		{
			FSlateApplication::Get().OnControllerButtonPressed(EKeysMyo::MyoPoseWaveIn, 0, 0);
		}
		else if (pose == myo::Pose::waveOut)
		{
			FSlateApplication::Get().OnControllerButtonPressed(EKeysMyo::MyoPoseWaveOut, 0, 0);
		}
		else if (pose == myo::Pose::fingersSpread)
		{
			FSlateApplication::Get().OnControllerButtonPressed(EKeysMyo::MyoPoseFingersSpread, 0, 0);
		}
		else if (pose == myo::Pose::thumbToPinky)
		{
			FSlateApplication::Get().OnControllerButtonPressed(EKeysMyo::MyoPoseThumbToPinky, 0, 0);
		}
		else
		{//unknown
			FSlateApplication::Get().OnControllerButtonPressed(EKeysMyo::MyoPoseUnknown, 0, 0);
		}
	}
	void ReleasePose(myo::Pose pose){
		if (pose == myo::Pose::rest)
		{
			FSlateApplication::Get().OnControllerButtonReleased(EKeysMyo::MyoPoseRest, 0, 0);
		}
		else if (pose == myo::Pose::fist)
		{
			FSlateApplication::Get().OnControllerButtonReleased(EKeysMyo::MyoPoseFist, 0, 0);
		}
		else if (pose == myo::Pose::waveIn)
		{
			FSlateApplication::Get().OnControllerButtonReleased(EKeysMyo::MyoPoseWaveIn, 0, 0);
		}
		else if (pose == myo::Pose::waveOut)
		{
			FSlateApplication::Get().OnControllerButtonReleased(EKeysMyo::MyoPoseWaveOut, 0, 0);
		}
		else if (pose == myo::Pose::fingersSpread)
		{
			FSlateApplication::Get().OnControllerButtonReleased(EKeysMyo::MyoPoseFingersSpread, 0, 0);
		}
		else if (pose == myo::Pose::thumbToPinky)
		{
			FSlateApplication::Get().OnControllerButtonReleased(EKeysMyo::MyoPoseThumbToPinky, 0, 0);
		}
		else
		{//unknown
			FSlateApplication::Get().OnControllerButtonReleased(EKeysMyo::MyoPoseUnknown, 0, 0);
		}
	}

	void onConnect(myo::Myo *myo, uint64_t timestamp,
		myo::FirmwareVersion firmwareVersion){
		UE_LOG(LogClass, Log, TEXT("Myo %d  has connected."), identifyMyo(myo));
		if (myoDelegate)
		{
			myoDelegate->MyoOnConnect(identifyMyo(myo), timestamp);
		}
	}

	void onDisconnect(myo::Myo *myo, uint64_t timestamp){
		UE_LOG(LogClass, Log, TEXT("Myo %d  has disconnected."), identifyMyo(myo));
		if (myoDelegate)
		{
			myoDelegate->MyoOnDisconnect(identifyMyo(myo), timestamp);
		}
	}

	void onArmRecognized(myo::Myo *myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection){
		int myoIndex = identifyMyo(myo) - 1;
		m_data[myoIndex].arm = arm;

		//internal tracking
		if (arm == myo::armLeft)
			leftMyo = myoIndex + 1;
		if (arm == myo::armRight)
			rightMyo = myoIndex + 1;

		if (myoDelegate)
		{
			myoDelegate->MyoOnArmRecognized(identifyMyo(myo), timestamp, arm, xDirection);
		}
	}
	void onArmLost(myo::Myo *myo, uint64_t timestamp){
		int myoIndex = identifyMyo(myo) - 1;
		m_data[myoIndex].arm = myo::armUnknown;

		//internal tracking, -1 means its invalid
		if (leftMyo == myoIndex + 1)
			leftMyo = -1;
		if (rightMyo == myoIndex + 1)
			rightMyo = -1;

		if (myoDelegate)
		{
			myoDelegate->MyoOnArmLost(identifyMyo(myo), timestamp);
		}
	}

	void onPair(myo::Myo* myo, uint64_t timestamp,
		myo::FirmwareVersion firmwareVersion){
		// The pointer address we get for a Myo is unique - in other words, it's safe to compare two Myo pointers to
		// see if they're referring to the same Myo.

		// Add the Myo pointer to our list of known Myo devices. This list is used to implement identifyMyo() below so
		// that we can give each Myo a nice short identifier.
		knownMyos.push_back(myo);
		MyoDeviceData data;
		m_data.push_back(data);

		// Now that we've added it to our list, get our short ID for it and print it out.
		if (myoDelegate!=NULL)
		{
			myoDelegate->MyoOnPair(identifyMyo(myo), timestamp);
		}
	}

	// Called when a paired Myo has provided new orientation data, which is represented
	// as a unit quaternion.
	void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat){
		int myoIndex = identifyMyo(myo)-1;

		m_data[myoIndex].quaternion.X = quat.x();
		m_data[myoIndex].quaternion.Y = quat.y();
		m_data[myoIndex].quaternion.Z = quat.z();
		m_data[myoIndex].quaternion.W = quat.w();
		m_data[myoIndex].orientation = convertOrientationToUE(FRotator(m_data[myoIndex].quaternion));

		if (myoDelegate)
		{
			myoDelegate->MyoOnOrientationData(myoIndex + 1, timestamp, m_data[myoIndex].quaternion);	//quaternion - raw
			myoDelegate->MyoOnOrientationData(myoIndex + 1, timestamp, m_data[myoIndex].orientation);	//overloaded rotator - in UE format

			//InputMapping - only supports controller 1 for now
			if (myoIndex == 0)
			{
				//Scale input mapping to -1.0-> 1.0 range
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoOrientationPitch, 0, m_data[myoIndex].orientation.Pitch * ORIENTATION_SCALE_PITCH);
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoOrientationYaw, 0, m_data[myoIndex].orientation.Yaw * ORIENTATION_SCALE_YAWROLL);
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoOrientationRoll, 0, m_data[myoIndex].orientation.Roll * ORIENTATION_SCALE_YAWROLL);
			}
		}
	}

	// Called when a paired Myo has provided new accelerometer data in units of g. 
	void onAccelerometerData(myo::Myo *myo, uint64_t timestamp, const myo::Vector3< float > &accel){
		int myoIndex = identifyMyo(myo) - 1;
		m_data[myoIndex].acceleration.X = accel.x();
		m_data[myoIndex].acceleration.Y = accel.y();
		m_data[myoIndex].acceleration.Z = accel.z();
		if (myoDelegate)
		{
			myoDelegate->MyoOnAccelerometerData(myoIndex + 1, timestamp, m_data[myoIndex].acceleration);

			//InputMapping - only supports controller 1 for now
			if (myoIndex == 0)
			{
				//No scaling needed, 1.0 = 1g.
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoAccelerationX, 0, m_data[myoIndex].acceleration.X);
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoAccelerationY, 0, m_data[myoIndex].acceleration.Y);
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoAccelerationZ, 0, m_data[myoIndex].acceleration.Z);
			}
		}
	}

	//ang/s
	void onGyroscopeData(myo::Myo *myo, uint64_t timestamp, const myo::Vector3< float > &gyro){
		int myoIndex = identifyMyo(myo) - 1;
		m_data[myoIndex].gyro.X = gyro.x();
		m_data[myoIndex].gyro.Y = gyro.y();
		m_data[myoIndex].gyro.Z = gyro.z();
		if (myoDelegate)
		{
			myoDelegate->MyoOnGyroscopeData(myoIndex + 1, timestamp, m_data[myoIndex].gyro);

			//InputMapping - only supports controller 1 for now
			if (myoIndex == 0)
			{
				//scaled down by 1/45. Fast flicks should be close to 1.0, slower gyro motions may need scaling up if used in input mapping
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoGyroX, 0, m_data[myoIndex].gyro.X * GYRO_SCALE);
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoGyroY, 0, m_data[myoIndex].gyro.Y * GYRO_SCALE);
				FSlateApplication::Get().OnControllerAnalog(EKeysMyo::MyoGyroZ, 0, m_data[myoIndex].gyro.Z * GYRO_SCALE);
			}
		}
	}

	// Called whenever the Myo detects that the person wearing it has changed their pose, for example,
	// making a fist, or not making a fist anymore.
	void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose){
		int myoIndex = identifyMyo(myo) - 1;
		m_data[myoIndex].pose = (int)pose.type();

		//debug
		UE_LOG(LogClass, Log, TEXT("Myo %d switched to pose %s."), identifyMyo(myo), tcharFromStdString(pose.toString()));
		//FString debugmsg = FString::Printf(TEXT("Myo %d switched to pose %s."), identifyMyo(myo), tcharFromStdString(pose.toString()));
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, debugmsg);

		if (myoDelegate)
		{
			myoDelegate->MyoOnPose(myoIndex + 1, timestamp, (int32)pose.type());

			//InputMapping - only supports controller 1 for now
			if (myoIndex == 0)
			{
				//release the old pose, press new pose
				ReleasePose(lastPose);
				PressPose(pose);

				//update lastPose
				lastPose = pose;
			}

		}
	}

	// This is a utility function implemented for this sample that maps a myo::Myo* to a unique ID starting at 1.
	// It does so by looking for the Myo pointer in knownMyos, which onPair() adds each Myo into as it is paired.
	size_t identifyMyo(myo::Myo* myo) {
		// Walk through the list of Myo devices that we've seen pairing events for.
		for (size_t i = 0; i < knownMyos.size(); ++i) {
			// If two Myo pointers compare equal, they refer to the same Myo device.
			if (knownMyos[i] == myo) {
				return i + 1;
			}
		}

		return 0;
	}

	// We store each Myo pointer that we pair with in this list, so that we can keep track of the order we've seen
	// each Myo and give it a unique short identifier (see onPair() and identifyMyo() above).
	std::vector<myo::Myo*> knownMyos;
	int leftMyo;
	int rightMyo;
	std::vector<MyoDeviceData>  m_data;	//up to n devices supported
	MyoDelegate* myoDelegate;
	bool Enabled;
	myo::Pose lastPose;
};

//Init and Runtime
void FMyoPlugin::StartupModule()
{
	// Instantiate the PrintMyoEvents class we defined above, and attach it as a listener to our Hub.
	collector = new DataCollector;

	try {
		const unsigned int pairingCount = 2;

		//This will throw an exception if the bluetooth dongle is missing! You cannot do any hub actions after.
		hub = new myo::Hub("com.plugin.unrealengine4");

		UE_LOG(LogClass, Error, TEXT("Pairing with %d Myo armbands."), pairingCount);
		hub->addListener(collector);

		collector->Enabled = true;

		//Register all input mapping keys and axes
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoPoseRest, LOCTEXT("MyoPoseRest", "Myo Pose Rest"), FKeyDetails::GamepadKey));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoPoseFist, LOCTEXT("MyoPoseFist", "Myo Pose Fist"), FKeyDetails::GamepadKey));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoPoseWaveIn, LOCTEXT("MyoPoseWaveIn", "Myo Pose Wave In"), FKeyDetails::GamepadKey));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoPoseWaveOut, LOCTEXT("MyoPoseWaveOut", "Myo Pose Wave Out"), FKeyDetails::GamepadKey));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoPoseFingersSpread, LOCTEXT("MyoPoseFingersSpread", "Myo Pose FingersSpread"), FKeyDetails::GamepadKey));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoPoseThumbToPinky, LOCTEXT("MyoPoseThumbToPinky", "Myo Pose Thumb To Pinky"), FKeyDetails::GamepadKey));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoPoseUnknown, LOCTEXT("MyoPoseUnknown", "Myo Pose Unknown"), FKeyDetails::GamepadKey));

		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoAccelerationX, LOCTEXT("MyoAccelerationX", "Myo Acceleration X"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoAccelerationY, LOCTEXT("MyoAccelerationY", "Myo Acceleration Y"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoAccelerationZ, LOCTEXT("MyoAccelerationZ", "Myo Acceleration Z"), FKeyDetails::FloatAxis));

		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoOrientationPitch, LOCTEXT("MyoOrientationPitch", "Myo Orientation Pitch"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoOrientationYaw, LOCTEXT("MyoOrientationYaw", "Myo Orientation Yaw"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoOrientationRoll, LOCTEXT("MyoOrientationRoll", "Myo Orientation Roll"), FKeyDetails::FloatAxis));

		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoGyroX, LOCTEXT("MyoGyroX", "Myo Gyro X"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoGyroY, LOCTEXT("MyoGyroY", "Myo Gyro Y"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(EKeysMyo::MyoGyroZ, LOCTEXT("MyoGyroZ", "Myo Gyro Z"), FKeyDetails::FloatAxis));
	}
	catch (const std::exception& e) {
		UE_LOG(LogClass, Error, TEXT("Error: %s"), e.what());
		hub = NULL;
	}
}

void FMyoPlugin::ShutdownModule()
{
	collector->myoDelegate = NULL;
	delete collector;
	if(hub != NULL) delete hub;
}


//Public API Implementation

/** Public API **/
void FMyoPlugin::VibrateDevice(int deviceId, int vibrationType)
{
	if (deviceId < 1) return;

	myo::Myo* myo = collector->knownMyos[deviceId - 1];
	myo->vibrate(static_cast<myo::Myo::VibrationType>(vibrationType));
}
 
//Freshest Data
void FMyoPlugin::LatestData(int deviceId, MyoDeviceData& data)
{
	if (deviceId < 1) return;

	data = collector->m_data[deviceId - 1];
}

void FMyoPlugin::WhichArm(int deviceId, int& arm)
{
	arm = collector->m_data[deviceId - 1].arm;
}

void FMyoPlugin::LeftMyoId(bool& available, int& deviceId)
{
	if (collector->leftMyo == -1)
		available = false;
	else{
		available = true;
		deviceId = collector->leftMyo;
	}
}
void FMyoPlugin::RightMyoId(bool& available, int& deviceId)
{
	if (collector->rightMyo == -1)
		available = false;
	else{
		available = true;
		deviceId = collector->rightMyo;
	}
}

bool FMyoPlugin::IsHubEnabled()
{
	return collector->Enabled;
}

void FMyoPlugin::SetDelegate(MyoDelegate* newDelegate){ 
	collector->myoDelegate = newDelegate;

	//Emit disabled event if we didnt manage to create the hub
	if (!collector->Enabled){
		collector->myoDelegate->MyoDisabled();
	}
}

void FMyoPlugin::MyoTick(float DeltaTime)
{
	/**
	* We're trying to emulate a single tick;
	* So run the hub for only 1ms, this will add roughly 1-2ms to render loop time
	* While this is not ideal and a separate loop would be an improvement, 1ms is an 
	* acceptable frame lag for simplicity.
	*/
	if (collector->Enabled){
		hub->run(1);
	}
}

#undef LOCTEXT_NAMESPACE