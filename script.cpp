

/*
		THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
				http://dev-c.com
			(C) Alexander Blade 2015
*/

/*
		F4					activate
		NUM2/8/4/6			navigate thru the menus and lists (numlock must be on)
		NUM5 				select
		NUM0/BACKSPACE/F4 	back
		NUM9/3 				use vehicle boost when active
		NUM+ 				use vehicle rockets when active

		NETWORK_CREATE_SYNCHRONISED_SCENE

		NETWORK_ADD_PED_TO_SYNCHRONISED_SCENE
*/


#include <sys/ppu_thread.h>
#include <string.h>
#include <ppu_asm_intrinsics.h>
#include <sys/sys_time.h>
#include <sys/time_util.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/process.h>
#include <sys/memory.h>
#include <sys/timer.h>
#include <sys/return_code.h>
#include <sys/prx.h>
#include <stddef.h>
#include <math.h>
#include <cmath>
#include <stdarg.h>
#include <cellstatus.h>
#include <typeinfo>
#include <algorithm>
#include <vector>
#include <pthread.h>
#include <locale.h>
#include <cell/error.h>
#include <sys/paths.h>
#include <time.h>
#include <net\if_dl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cell/cell_fs.h>
#include <cell/sysmodule.h>
#include <stdio.h>
#include <string.h>
#include <cell/fs/cell_fs_errno.h>
#include <cell/fs/cell_fs_file_api.h>
#include <netdb.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <ppu_intrinsics.h>
#include <stdlib.h>
#include <cfloat>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <np.h>
#include <string>
#include <xstring>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fastmath.h>
#include <sys/random_number.h>
#include <spu_printf.h>
#include "Dialog.h"
#include "Sockets.h"
#include "Enums.h"
#include "Natives.h"


SYS_MODULE_INFO(Menyoo, 0, 1, 1);
SYS_MODULE_START(ENTRYPOINT);

#pragma warning(disable : 4244 4305) // double <-> float conversions
//features/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NativesTable_s** g_Natives;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *Memcpy(void *dst0, const void *src0, size_t len0)
{
	char *dst = (char *)dst0;
	char *src = (char *)src0;

	void *save = dst0;

	while (len0--)
		*dst++ = *src++;

	return save;
}
int write_process(void* destination, const void* source, size_t size)
{
	Memcpy(destination, source, size);
}
/*int write_process(void* destination, const void* source, size_t size)
{
	system_call_4(905, (uint64_t)sys_process_getpid(), (uint64_t)destination, size, (uint64_t)source);
	return_to_user_prog(int);
}*/
void PatchInJump(uint64_t Address, int Destination, bool Linked)
{
	int FuncBytes[4];													// Use this data to copy over the address.
	Destination = *(int*)Destination;									// Get the actual destination address (pointer to void).
	FuncBytes[0] = 0x3D600000 + ((Destination >> 16) & 0xFFFF);			// lis %r11, dest>>16
	if (Destination & 0x8000)											// if bit 16 is 1...
		FuncBytes[0] += 1;
	FuncBytes[1] = 0x396B0000 + (Destination & 0xFFFF);					// addi %r11, %r11, dest&OxFFFF
	FuncBytes[2] = 0x7D6903A6;											// mtctr %r11
	FuncBytes[3] = 0x4E800420;											// bctr
	if (Linked)
		FuncBytes[3] += 1;												// bctrl
	//memcpy for cex
	write_process((void*)Address, FuncBytes, 4 * 4);
}
unsigned int FindNativeTableAddress()
{
	unsigned int  l_uiNativeTableAddress = 0;
	unsigned int l_uiStartAddress = 0x390000;
	unsigned int l_uiAddress = 0;
	for (unsigned int i = 0; i < 0x10000; i++)
	{
		if (*(unsigned int*)(l_uiStartAddress + i) == 0x3C6072BD)
			if (*(unsigned int*)(l_uiStartAddress + i + 8) == 0x6063E002)
			{
				l_uiAddress = *(unsigned int*)(l_uiStartAddress + i + 0x10);
				l_uiAddress &= 0xFFFFFF;
				l_uiAddress += (l_uiStartAddress + i + 0x10) - 1;
				break;
			}
	}
	l_uiNativeTableAddress = (*(unsigned int*)(l_uiAddress + 0x24) << 16) + (*(unsigned int*)(l_uiAddress + 0x2C) & 0xFFFF);
	l_uiNativeTableAddress -= 0x10000;
	return l_uiNativeTableAddress;
}
int NativeToAddress(int Native, bool PatchInJump = true)
{
	int Hash = Native & 0xFF;
	int Table = *(int*)(FindNativeTableAddress() + (Hash * 4));
	while (Table != 0)
	{
		int NativeCount = *(int*)(Table + 0x20);
		for (int i = 0; i < NativeCount; i++)
		{
			if (*(int*)((Table + 0x24) + (i * 4)) == Native)
			{
				if (PatchInJump)
				{
					int NativeLocation = *(int*)(*(int*)((Table + 4) + (i * 4)));
					for (int i = 0; i < 50; ++i)
					{
						short CurrentPlace = *(short*)(NativeLocation + (i * 4));
						if (CurrentPlace == 0x4AE6 || CurrentPlace == 0x4AA8 || CurrentPlace == 0x4AE4 || CurrentPlace == 0x4AE5)
						{
							return (((*(int*)(NativeLocation + (i * 4))) - 0x48000001) + (NativeLocation + (i * 4))) - 0x4000000;
						}
					}
				}
				else
				{
					return *(int*)((Table + 4) + (i * 4));
				}

			}
		}
		Table = *(int*)(Table);
	}
}
void sleep(usecond_t time)  //1 second = 1000
{
	sys_timer_usleep(time * 1000);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace SUB {
	enum {
		// Define submenu (enum) indices here.
		CLOSED,
		MAINMENU,
		SETTINGS,
		SETTINGS_COLOURS,
		SETTINGS_COLOURS2,
		// Others:
		PLAYEROPTIONS,
		VEHICLEOPTIONS,
		VEHICLE_SPAWNER,
		VEHICLE_WEAPONS,
		VEHICLE_EDITOR,
		VEHICLE_PRIMARY,
		VEHICLE_SECONDARY,
		VEHICLE_SPAWNER_SETTINGS,
		OBJECTMENU,
		WEAPONOPTIONS,
		MODELOPTIONS,
		PEDOPTIONS,
		TELEPORTATIONS,
		WORLDOPTIONS,
		MISCELLANEOUS,
		NETWORK_MAIN,
		NETWORK_PLAYERS,
		ALL_NETWORK,
		NETWORK_ALL_ANIM,
		NETWORK_PLAYERS_OPTIONS,
		NETWORK_PLAYER_OPTIONS,
		NETWORK_VEHICLE_OPTIONS,
		NETWORK_WEAPON_OPTIONS,
		NETWORK_TELEPORT_OPTIONS,
		NETWORK_NAUGHTY_OPTIONS,
		NETWORK_ATTACH_OPTIONS,
		NETWORK_ANIMATION_OPTIONS,
		ANIMATIONOPTIONS,
		RECOVERYMENU,
		POJECTILEMENU,
		OUTFITDESIGNER,
		PARTICLEFX,
		VEHICLE_PAINT,
		ADVANCE_OPTIONS,
		NETWORK_MISC,
		NETWORK_MESSAGE,
		NAME_EDITOR,
		STORED_OBJECTS,



	};
};
namespace {
	// Declare/Initialise global variables here.
	char str[50];
	char Talk[100];
	float NewsbarX;
	bool controllerinput_bool = 1, menujustopened = 1, null, keyboardActive = 0;
	int keyboardAction, *keyboardVar = 0;
	float *keyboardVarFloat = 0;
	namespace COL { enum COL { TITLEBOX, BACKGROUND, TITLETEXT, OPTIONTEXT, OPTIONCOUNT, SELECTEDTEXT, BREAKS, SELECTIONHIGH }; }
	namespace FON { enum FON { TITLE = 10, OPTION, SELECTEDTEXT, BREAKS }; }
	//int *settings_font, inull;
	int fam;
	float settings_cash;
	float rpm;
	float torque;
	float primaryR;
	float primaryG;
	float primaryB;
	float secondaryR;
	float secondaryG;
	float secondaryB;
	RGBA *settings_rgba;
	RGBA *primary_rgb;
	RGBA line = { 255, 0, 0, 200 };
	RGBA BG = { 0, 0, 0, 200 };
	RGBA titletext = { 255, 0, 0, 255 };
	RGBA optiontext = { 120, 120, 120, 255 };
	RGBA optioncount = { 120, 120, 120, 255 };
	RGBA selectedtext = { 120, 120, 120, 255 };
	RGBA optionbreaks = { 255, 0, 0, 240 };
	RGBA scroller = { 255, 0, 0, 110 };
	int font_title = 4, font_options = 4, font_selection = 4, font_breaks = 1, screen_res_x, screen_res_y, *settings_rgba2, vehiclespawnerActiveLineIndex, objectspawnerActiveLineIndex, animationActiveLineIndex,
		featureRequestControlAction = 0, SuperIndex = 0, SportsIndex = 0, SportsClassicIndex = 0, CoupesIndex = 0, MuscleIndex = 0, OffroadIndex = 0,
		SUVsIndex = 0, SedansIndex = 0, CompactIndex = 0, VansIndex = 0, TrucksIndex = 0, ServicesIndex = 0, TrailersIndex = 0, TrainsIndex = 0,
		EmergencyIndex = 0, MotocyclesIndex = 0, CyclesIndex = 0, PlanesIndex = 0, HelicoptersIndex = 0, BoatsIndex = 0, weatherTypeIndex = 4, sportAnimTypeIndex = 0, animalAnimTypeIndex = 0, danceAnimTypeIndex = 0, naughtyAnimTypeIndex = 0, miscAnimTypeIndex = 0, sittingAnimTypeIndex = 0, precisionlvlIndex = 0, animationBaseIndex = 0, vehicleWeaponIndex = 0, bountyTypeIndex = 0,
		propObjectTypeIndex = 0, modelTypeIndex = 0, SFMMPG, SFMIB, instructCount, spawnedObjectCount = 0, messagingTypeIndex = 0,
		featureAnimFlag, featureAnimaAction = 0,  moneyPropTypeIndex = 0, levelTypeIndex = 0, spoilerTypeIndex = 0, exhaustTypeIndex = 0, frontBumberTypeIndex = 0, rearBumberTypeIndex = 0, sideSkirtTypeIndex = 0, hoodTypeIndex = 0, frameTypeIndex = 0, grilleTypeIndex = 0, fenderTypeIndex = 0, rightFenderTypeIndex = 0, roofTypeIndex = 0, engineTypeIndex = 0, transmissionTypeIndex = 0, 
		breakTypeIndex = 0, hornsTypeIndex = 0, SuspensionTypeIndex = 0, armorTypeIndex = 0, windowtentTypeIndex = 0, methodsTypeIndex = 0, colorTypeIndex = 0,menuPositionIndex = 0, ComponentTypeIndex = 0;
	float menuPos = 0.658f, OptionY, setcoordsx, setcoordsy, setcoordsz, titleX = 0.16f, titleY = 0.1255f, titleHight = 0.06f, lineY = 0.157f, 
		precisionlvlAmount = 1.0, onlineAnimeDuriation = -1 ,featureAnimSpeed, featureanimSpeedMuli, fly_yaw, fly_rotation, fly_pitch, fly_roll = 0.0f, WaterAir = 1.732711;
	bool setxAxis, setyAxis, setzAxis, featureRequestControlOfEnt = 0, featureModderProtection,featureSuperJump = 0, featureSuperRun = 0, featureInvisibility = 0, featureTalkingPlayers, featureUnlimitedAmmo = 0,
		featureInvincibility = 0, featureSeatBelt = 0 ,featureNeverWanted = 0, featureFreezeVeh = 0,featureVehAttack = 0, featurePedAttack = 0,featureCreateVehicle = 0, featureVehWarpInSpawned = 1, featureInvincibleVehicleOnSpawn = 0,
		featureMaxAllUpgrades = 0, featureNoCollision = 0, featureVehInvisibility = 0,featureToggleDoors = 0, featureNeedForSpeed = 0,featureHandlingFly = 0, featureAttachToVehicle,featureFreezeVehicle = 0, instructionsSetupThisFrame = 0, featureCreatePed = 0,
		featureInvinciblePedOnSpawn = 0, featureMakePedPresistent = 0, NewsBarBool = 1, featureSpeedoSpeedText = 0, SpawnObject = 0, featureRapePlayer = 0, featureMakeExplodeAll = 0, Rape = 0,
		featureDoAnim = 0,featureSkinChanger = 0, featureIsCustomSkin = 0, featureRagDoll = 0, featurePropGun = 0, weedgun1 = 0, featureSuperMan = 0, 
		featureVehicleWeapons = 0, featureAttachObjVeh = 0, featureCashGunToggle = 0, featureWaterDrive = 0, featureJetAttack = 0, DriveWater = 0, featurenoclip = 0,
		featureTurbo = 0, featureXenon = 0, featureUNK17 = 0, featureUNK19 = 0, featureUNK21 = 0, tryAgain = 0, featureGhostRider = 0, featureBlackOut = 0;
	char *featureAnimDic, *featureAnimName, *featureCustomSkin, *featureCustomObjectSpawner, *MMSpreview = "Send With Iridium";
	int pedHandle, vehToSpawnHash, vehToSpawnHash1;
	float Position[3];
	Vehicle myVeh, cam_gta2;
	Object tmpobj, SpawnedObj, curObject;;
	Entity emptySlot, featureReqEnt, featureAnimPed;
	Hash vehicleCustomHash, pedCustomHash, hash_obj, attackVehHash, attackPedHash, freezeVehHash;
	DWORD waitTimeMain = 0, waitTimetwo = 0, waitTimeGuns = 0, waitTimeSix = 0, waitTimeFive = 0, waitTimeinstructions = 0;
	Player selectedPlayer, featureExplodeClientCount = 0;
	struct selectedOptions
	{
		bool featureMoneyDrop, featureStopTask, featureHydratePlayer, featureESPTracer, featureExplodePlayer, featurePropMoneyDrop, 
		featurewantedlevel, featureRedBoxes, featureNetworkExp;
	};
	selectedOptions individualPlayer[18];


	// Booleans for loops go here:
	bool loop_RainbowBoxes = 0, featureGravityGun = 0, featureDisplayFPS = 0, featureSnapGround = 0, featureFreezeObj = 0, featurePlayerInfo = 0, featureSpeedometerSkin = 0, 
		featureCashGun = 0, featureForgeGun = 0, featureDeleteGun = 0, featureTeleportGun = 0, featureESPAll = 0, featureInnerForce = 0,
		featureWeaponLaser = 0, featureObjectHash = 0, featureCarJump = 0, featureExplosiveBullets = 0, featureCashAll = 0, playerinfoSelected = 0;
}
namespace {
	// Declare subroutines here.



}
namespace {
	// Define subroutines here.

	void VectorToFloat(Vector3 unk, float *Out)
	{
		Out[0] = unk.x;
		Out[1] = unk.y;
		Out[2] = unk.z;
	}
	int RandomRGB()
	{
		return (GET_RANDOM_INT_IN_RANGE(0, 255));
	}
	bool Check_self_in_vehicle()
	{
		if (IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0)) return true; else return false;
	}
	int FindFreeCarSeat(Vehicle vehicle)
	{
		int max = GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(vehicle) - 2;
		for (static int tick = -1; tick <= max; tick++)
		{
			if (IS_VEHICLE_SEAT_FREE(vehicle, tick))
			{
				return tick;
			}
		}

		return -1;
	}
	void offset_from_entity(int entity, float X, float Y, float Z, float * Out)
	{
		VectorToFloat(GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(entity, X, Y, Z), Out);
	}
	void RequestControlOfEnt(Entity entity)
	{
		NETWORK_REQUEST_CONTROL_OF_ENTITY(entity);
		if (!NETWORK_HAS_CONTROL_OF_ENTITY(entity))
			WAIT(0);
		NETWORK_REQUEST_CONTROL_OF_ENTITY(entity);
	}
	void RequestControlOfid(int netid)
	{
		NETWORK_REQUEST_CONTROL_OF_NETWORK_ID(netid);
		if (!NETWORK_HAS_CONTROL_OF_NETWORK_ID(netid))
			WAIT(0);
		NETWORK_REQUEST_CONTROL_OF_NETWORK_ID(netid);
	}
	//this set entity coords request control first 
	void _SET_ENTITY_COORDS_NO_OFFSET(Entity entity, float xPos, float yPos, float zPos, BOOL xAxis, BOOL yAxis, BOOL zAxis) { featureReqEnt = entity; setcoordsx = xPos, setcoordsy = yPos, setcoordsz = zPos; setxAxis = xAxis, setxAxis = yAxis, setxAxis = zAxis, featureRequestControlAction = 1; featureRequestControlOfEnt = true; }
	//this requestes control to the action you set your feature on
	void RequestControlOfEntM(Entity entity, requestcontrolActions action) { featureReqEnt = entity; featureRequestControlAction = action; featureRequestControlOfEnt = true; }
	//this request control and requests the anmation Dictionary
	void TaskPlayAnimationM(Ped ped, char *animDictionary, char *animationName, float speed = 8.0f, float speedmulitiplier = 0.0f, int flag = 9) { featureAnimPed = ped; featureAnimDic = animDictionary, featureAnimName = animationName; featureAnimSpeed = speed, featureanimSpeedMuli = speedmulitiplier; featureAnimFlag = flag; featureDoAnim = true; }
	BOOL IsPlayerFriend(Player player)
	{
		int handle[76];
		NETWORK_HANDLE_FROM_PLAYER(player, &handle[0], 13);
		return NETWORK_IS_FRIEND(&handle[0]);
	}
	void TOGGLE_ALL_DOORS(Vehicle vehicle, bool toggle)
	{
		if (toggle)
			for (int i = 0; i <= 6; i++)
				SET_VEHICLE_DOOR_OPEN(vehicle, i, 0, 0);
		else
			SET_VEHICLE_DOORS_SHUT(vehicle, 0);
	}
	Object PlaceObject(Hash ObjHash, float X, float Y, float Z, float Pitch, float Roll, float Yaw, bool Presistant)
	{
		tmpobj = CREATE_OBJECT(ObjHash, X, Y, Z, 1, 1, 0);
		RequestControlOfEnt(tmpobj);
		SET_ENTITY_ROTATION(tmpobj, Pitch, Roll, Yaw, 2, 1);
		if (Presistant)
			SET_ENTITY_AS_MISSION_ENTITY(tmpobj, 1, 1);
		FREEZE_ENTITY_POSITION(tmpobj, 1);
		SET_ENTITY_LOD_DIST(tmpobj, 99999999);
		SET_MODEL_AS_NO_LONGER_NEEDED(ObjHash);
		//SET_OBJECT_AS_NO_LONGER_NEEDED(&tmpobj);
		return tmpobj;
	}
	Object CREATE_n_ATTACH(Hash hash, Entity entity, Bone bone, float x, float y, float z, float xRot, float yRot, float zRot, BOOL collision, int flag, int flag2)
	{
		Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PLAYER_PED_ID(), 0.0, 0.0, 20.0);
		Vehicle veh = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
		switch (flag) { case 0: curObject = CREATE_OBJECT(hash, coords.x, coords.y, coords.z, 1, 0, 0); break; /*case 1: curObject = CreateVehicleM(hash, coords.x, coords.y, coords.z); break; case 2: curObject = CreatePedM(hash, coords.x, coords.y, coords.z); break;*/ }
		NETWORK_REQUEST_CONTROL_OF_ENTITY(curObject);
		switch (flag2) {
		case 0: /*SET_ENTITY_AS_MISSION_ENTITY(curObject, 1, 1);*/ ATTACH_ENTITY_TO_ENTITY(curObject, entity, IS_ENTITY_A_PED(entity) ? GET_PED_BONE_INDEX(GET_PLAYER_PED(selectedPlayer), bone) : bone, x, y, z, xRot, yRot, zRot, 0, 0, collision, 0, 2, 1); break;
		}
		return curObject;
	}
	Vehicle CreateVehicleM(Hash hash, float x, float y, float z)
	{
		Vehicle vehicle = 0;
		if (!HAS_MODEL_LOADED(hash))
			REQUEST_MODEL(hash);
		else
		{
			vehicle = CREATE_VEHICLE(hash, x, y, z, 0.0, 1, 0);
			SET_MODEL_AS_NO_LONGER_NEEDED(hash);
		}
		return vehicle;
	}
	Ped CreatePedM(Hash hash, float x, float y, float z)
	{
		Ped ped = 0;
		if (!HAS_MODEL_LOADED(hash))
			REQUEST_MODEL(hash);
		else
		{
			ped = CREATE_PED(26, hash, x, y, z, 0.0, 1, 0);
			SET_MODEL_AS_NO_LONGER_NEEDED(hash);
		}
		return ped;
	}
	//this radnoms apperance doesnt not crash you console
	void RandomAppearance(Ped ped) { for (int i = 0; i < 12; i++) { for (int j = 0; j < 100; j++) { int drawable = GET_RANDOM_INT_IN_RANGE(0, 10); int texture = GET_RANDOM_INT_IN_RANGE(0, 10); if (IS_PED_COMPONENT_VARIATION_VALID(ped, i, drawable, texture)) { SET_PED_COMPONENT_VARIATION(ped, i, drawable, texture, 0); break; } } } }
	bool RequestModel(Hash model)
	{
		if (HAS_MODEL_LOADED(model))
			return true;
		else if (!HAS_MODEL_LOADED(model))
			REQUEST_MODEL(model);
		else
			return false;
		return false;
	}
	int SetTunable(int index, int value, int wouldRead) {
		int EntryPoint = (*(int*)0x1E70374) + 4 + (index * 4);
		switch (wouldRead)
		{
		case 0: *(int*)EntryPoint = value; break; //write
		case 1: return *(int*)EntryPoint; break; //read
		case 2: /*return &*(int*)EntryPoint;*/ break; //to address //returns int *
		}
	}
	int SetGlobal(unsigned int globalId, int value, int wouldRead) {
		static unsigned int** arr = (unsigned int**)0x1E70370;
		switch (wouldRead)
		{
		case 0: arr[(globalId >> 18) & 0x3F][globalId & 0x3FFFF] = value; /*arr[globalId / 0x40000][globalId % 0x40000] = value;*/ break; //write
		case 1: return arr[(globalId >> 18) & 0x3F][globalId & 0x3FFFF]; /*return arr[globalId / 0x40000][globalId % 0x40000];*/ break; //read
		case 2: /*&arr[(globalId >> 18) & 0x3F][globalId & 0x3FFFF];*/ /*&return &arr[globalId / 0x40000][globalId % 0x40000];*/ break; //to address //returns int *
		}
	}
	void setupdraw()
	{
		SET_TEXT_FONT(0);
		SET_TEXT_SCALE(0.4f, 0.4f);
		SET_TEXT_COLOUR(255, 255, 255, 255);
		SET_TEXT_WRAP(0.0f, 1.0f);
		SET_TEXT_CENTRE(0);
		SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		SET_TEXT_EDGE(0, 0, 0, 0, 0);
		SET_TEXT_OUTLINE();
	}
	void drawstring(char* text, float X, float Y)
	{
		_SET_TEXT_ENTRY("STRING");
		ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
		_DRAW_TEXT(X, Y);
	}
	void drawinteger(int text, float X, float Y)
	{
		_SET_TEXT_ENTRY("NUMBER");
		ADD_TEXT_COMPONENT_INTEGER(text);
		_DRAW_TEXT(X, Y);
	}
	void drawfloat(float value, int decimal_places, float X, float Y)
	{
		_SET_TEXT_ENTRY("NUMBER");
		ADD_TEXT_COMPONENT_FLOAT(value, decimal_places);
		_DRAW_TEXT(X, Y);
	}
	void PlaySoundFrontend(char* sound_dict, char* sound_name)
	{
		PLAY_SOUND_FRONTEND(-1, sound_name, sound_dict);
	}
	void PlaySoundFrontend_default(char* sound_name)
	{
		PLAY_SOUND_FRONTEND(-1, sound_name, "HUD_FRONTEND_DEFAULT_SOUNDSET");
	}
	bool Check_compare_string_length(char* unk1, size_t max_length)
	{
		if (strlen(unk1) <= max_length) return true; else return false;
	}
	char* AddStrings(char* string1, char* string2)
	{
		memset(str, 0, sizeof(str));
		strcpy(str, "");
		strcpy(str, string1);
		strcat(str, string2);
		return str;
	}
	char* AddInt_S(char* string1, int int2)
	{
		char Return[50];
		sprintf(Return, "%i", int2);
		return AddStrings(string1, Return);

		/*std::string string2 = string1;
		string2 += std::to_string(int2);

		char * Char = new char[string2.size() + 1];
		std::copy(string2.begin(), string2.end(), Char);
		Char[string2.size()] = '\0';

		static char* Return = Char;
		delete[] Char;
		return Return;*/
	}
	float func_444(float fParam0, float fParam1, float fParam2)
	{
		float fVar0;

		if (fParam1 == fParam2)
		{
			return fParam1;
		}
		fVar0 = fParam2 - fParam1;
		fParam0 -= TO_FLOAT(ROUND(fParam0 - fParam1 / fVar0)) * fVar0;
		if (fParam0 < fParam1)
		{
			fParam0 += fVar0;
		}
		return fParam0;
	}
	void AddInstruction(ScaleformButtons button, char *text)
	{
		if (!instructionsSetupThisFrame)
		{
			if (!HAS_SCALEFORM_MOVIE_LOADED(SFMIB))
				SFMIB = REQUEST_SCALEFORM_MOVIE("instructional_buttons");
			DRAW_SCALEFORM_MOVIE_FULLSCREEN(SFMIB, 255, 255, 255, 0);
			_PUSH_SCALEFORM_MOVIE_FUNCTION(SFMIB, "CLEAR_ALL");
			_POP_SCALEFORM_MOVIE_FUNCTION_VOID();
			_PUSH_SCALEFORM_MOVIE_FUNCTION(SFMIB, "SET_CLEAR_SPACE");
			_PUSH_SCALEFORM_MOVIE_FUNCTION_PARAMETER_INT(200);
			_POP_SCALEFORM_MOVIE_FUNCTION_VOID();
			instructCount = 0;
			instructionsSetupThisFrame = true;
		}
		_PUSH_SCALEFORM_MOVIE_FUNCTION(SFMIB, "SET_DATA_SLOT");
		_PUSH_SCALEFORM_MOVIE_FUNCTION_PARAMETER_INT(instructCount);
		_PUSH_SCALEFORM_MOVIE_FUNCTION_PARAMETER_INT(button);
		_BEGIN_TEXT_COMPONENT("STRING");
		ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
		_END_TEXT_COMPONENT();
		_POP_SCALEFORM_MOVIE_FUNCTION_VOID();
		instructCount++;
	}
	void instructionsClose()
	{
		_PUSH_SCALEFORM_MOVIE_FUNCTION(SFMIB, "DRAW_INSTRUCTIONAL_BUTTONS");
		_POP_SCALEFORM_MOVIE_FUNCTION_VOID();
		_PUSH_SCALEFORM_MOVIE_FUNCTION(SFMIB, "SET_BACKGROUND_COLOUR");
		_PUSH_SCALEFORM_MOVIE_FUNCTION_PARAMETER_INT(0);
		_PUSH_SCALEFORM_MOVIE_FUNCTION_PARAMETER_INT(0);
		_PUSH_SCALEFORM_MOVIE_FUNCTION_PARAMETER_INT(0);
		_PUSH_SCALEFORM_MOVIE_FUNCTION_PARAMETER_INT(80);
		_POP_SCALEFORM_MOVIE_FUNCTION_VOID();
	}
	void DrawSprite(char * Streamedtexture, char * textureName, float x, float y, float width, float height, float heading, int R, int G, int B, int A)
	{
		if (!HAS_STREAMED_TEXTURE_DICT_LOADED(Streamedtexture))
			REQUEST_STREAMED_TEXTURE_DICT(Streamedtexture, 0);
		else
			DRAW_SPRITE(Streamedtexture, textureName, x, y, width, height, heading, R, G, B, A);
	}
	char * _GET_PLAYER_NAME(Player player)
	{
		char* Name = GET_PLAYER_NAME(player);
		if (!strcmp(Name, "**Invalid**"))
			return "---";
		else
			return Name;
	}
	int getHost()
	{
		return NETWORK_GET_HOST_OF_SCRIPT("freemode", -1, 0);
	}
	char* getHostName()
	{
		return _GET_PLAYER_NAME(getHost());
	}
	bool isHost()
	{
		int Host = getHost();
		int Local = PLAYER_ID();
		if (!strcmp(_GET_PLAYER_NAME(Host), _GET_PLAYER_NAME(Local)))
			return true;
		else return false;
	}
	void Chat(char* Text, int Index)
	{
		SET_TEXT_FONT(4);
		SET_TEXT_SCALE(0.500, 0.500);
		SET_TEXT_COLOUR(255, 255, 255, 255);
		SET_TEXT_OUTLINE();
		SET_TEXT_WRAP(0, 1);
		_SET_TEXT_ENTRY("STRING");
		_ADD_TEXT_COMPONENT_STRING(Text);
		_DRAW_TEXT(0.070, 0.220 + (Index * 0.028));
	}
	void SpeedoText(char *text, float X, float Y)
	{
		SET_TEXT_FONT(6);
		SET_TEXT_SCALE(0.800f, 0.800f);
		SET_TEXT_COLOUR(255, 255, 255, 255);
		SET_TEXT_WRAP(0.0f, 1.0f);
		SET_TEXT_CENTRE(0);
		SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		SET_TEXT_OUTLINE();
		_SET_TEXT_ENTRY("STRING");
		_ADD_TEXT_COMPONENT_STRING(text);
		_DRAW_TEXT(X, Y);
	}
	int _STAT_GET_INT(char *stat, bool save) {
		int stat_get = 0;
		if (!strcmp(_GET_TEXT_SUBSTRING(stat, 0, 5), "MPPLY"))
			STAT_GET_INT(GET_HASH_KEY(stat), &stat_get, save);
		else
		{
			char sbuf[50];
			int stat_r;
			STAT_GET_INT(GET_HASH_KEY("MPPLY_LAST_MP_CHAR"), &stat_r, 1);
			snprintf(sbuf, sizeof(sbuf), "MP%i_%s", stat_r, stat);
			STAT_GET_INT(GET_HASH_KEY(sbuf), &stat_get, save);
		}
		return stat_get;
	}
	void _STAT_SET_INT(char *stat, int value, bool save) {
		if (!strcmp(_GET_TEXT_SUBSTRING(stat, 0, 5), "MPPLY"))
			STAT_SET_INT(GET_HASH_KEY(stat), value, save);
		else {
			char sbuf[60];
			int stat_r;
			STAT_GET_INT(GET_HASH_KEY("MPPLY_LAST_MP_CHAR"), &stat_r, 1);
			snprintf(sbuf, sizeof(sbuf), "MP%i_%s", stat_r, stat);
			STAT_SET_INT(GET_HASH_KEY(sbuf), value, save);
		}
	}
	void _STAT_SET_BOOL(char *stat, int value, bool save) {
		if (!strcmp(_GET_TEXT_SUBSTRING(stat, 0, 5), "MPPLY"))
			STAT_SET_BOOL(GET_HASH_KEY(stat), value, save);
		else {
			char sbuf[60];
			int stat_r;
			STAT_GET_INT(GET_HASH_KEY("MPPLY_LAST_MP_CHAR"), &stat_r, 1);
			snprintf(sbuf, sizeof(sbuf), "MP%i_%s", stat_r, stat);
			STAT_SET_BOOL(GET_HASH_KEY(sbuf), value, save);
		}
	}
	void _STAT_SET_FLOAT(char *stat, float value, bool save) {
		if (!strcmp(_GET_TEXT_SUBSTRING(stat, 0, 5), "MPPLY"))
			STAT_SET_FLOAT(GET_HASH_KEY(stat), value, save);
		else {
			char sbuf[60];
			int stat_r;
			STAT_GET_INT(GET_HASH_KEY("MPPLY_LAST_MP_CHAR"), &stat_r, 1);
			snprintf(sbuf, sizeof(sbuf), "MP%i_%s", stat_r, stat);
			STAT_SET_FLOAT(GET_HASH_KEY(sbuf), value, save);
		}
	}
	void StartKeyboard(keyboardActions action, char *defaultText, int maxLength)
	{
		DISPLAY_ONSCREEN_KEYBOARD(0, "FMMC_KEY_TIP8", "", defaultText, "", "", "", maxLength);
		keyboardAction = action;
		keyboardActive = true;
	}
	int StringToInt(char* text)
	{
		static int tmp;
		if (text == "") return NULL;
		if (STRING_TO_INT(text, &tmp)) return NULL;

		return tmp;
	}
	void FloatingHelpText1(char* text)
	{
		char buf[500];
		snprintf(buf, sizeof(buf), "%s", text);
		_SET_TEXT_COMPONENT_FORMAT("STRING");
		ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(buf);
		_DISPLAY_HELP_TEXT_FROM_STRING_LABEL(0, 0, 1, -1);
	}
	void ShowSubtitle(char* text)
	{
		_SET_TEXT_ENTRY_2("STRING");
		ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
		_DRAW_SUBTITLE_TIMED(2000, 1);
	}
	void ShowSubtitleFloat(float text, int decimal_places)
	{
		_SET_TEXT_ENTRY_2("NUMBER");
		ADD_TEXT_COMPONENT_FLOAT(text, decimal_places);
		_DRAW_SUBTITLE_TIMED(2000, 1);
	}
	void ShowNotification(char* text)
	{
		_SET_NOTIFICATION_TEXT_ENTRY("STRING");
		ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
		_SET_NOTIFICATION_MESSAGE_CLAN_TAG_2("CHAR_SOCIAL_CLUB", "CHAR_SOCIAL_CLUB", 1, 7, "~r~Iridium", "~r~By: Joshk326", 1, "___DEV", 8);
		_DRAW_NOTIFICATION(0, 1);
	}
	void PrintError_InvalidInput()
	{
		ShowSubtitle("~r~Error: Invalid Input");
	}
	class menu
	{
	public:
		static unsigned int currentsub;
		static unsigned int currentop;
		static unsigned int currentop_w_breaks;
		static unsigned int totalop;
		static unsigned int printingop;
		static unsigned int breakcount;
		static unsigned int totalbreaks;
		static unsigned int breakscroll;
		static int currentsub_ar_index;
		static int currentsub_ar[20];
		static int currentop_ar[20];
		static int SetSub_delayed;
		static unsigned long int livetimer;
		static bool bit_centre_title, bit_centre_options, bit_centre_breaks;

		static void update_status_two();
		static void update_status_five();
		static void update_status_six();
		static void update_vehicle_guns();
		static void update_features();
		static void update_menu_actions();
		static void submenu_switch();
		static void DisableControls()
		{
			HIDE_HELP_TEXT_THIS_FRAME();
			SET_CINEMATIC_BUTTON_ACTIVE(1);
			DISABLE_CONTROL_ACTION(0, INPUT_NEXT_CAMERA);
			DISABLE_CONTROL_ACTION(0, INPUT_VEH_SELECT_NEXT_WEAPON);
			DISABLE_CONTROL_ACTION(0, INPUT_VEH_CIN_CAM);
			SET_INPUT_EXCLUSIVE(2, INPUT_FRONTEND_X);
			SET_INPUT_EXCLUSIVE(2, INPUT_FRONTEND_ACCEPT);
			SET_INPUT_EXCLUSIVE(2, INPUT_FRONTEND_CANCEL);
			DISABLE_CONTROL_ACTION(0, INPUT_HUD_SPECIAL);
			SET_INPUT_EXCLUSIVE(2, INPUT_FRONTEND_DOWN);
			SET_INPUT_EXCLUSIVE(2, INPUT_FRONTEND_UP);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_ACCEPT);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_CANCEL);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_LEFT);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_RIGHT);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_DOWN);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_UP);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_RDOWN);
			DISABLE_CONTROL_ACTION(2, INPUT_FRONTEND_ACCEPT);
			HIDE_HUD_COMPONENT_THIS_FRAME(10);
			HIDE_HUD_COMPONENT_THIS_FRAME(6);
			HIDE_HUD_COMPONENT_THIS_FRAME(7);
			HIDE_HUD_COMPONENT_THIS_FRAME(9);
			HIDE_HUD_COMPONENT_THIS_FRAME(8);
			SET_INPUT_EXCLUSIVE(2, INPUT_FRONTEND_LEFT);
			SET_INPUT_EXCLUSIVE(2, INPUT_FRONTEND_RIGHT);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_UNARMED);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_MELEE);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_HANDGUN);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_SHOTGUN);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_SMG);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_AUTO_RIFLE);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_SNIPER);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_HEAVY);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_WEAPON_SPECIAL);
			DISABLE_CONTROL_ACTION(0, INPUT_WEAPON_WHEEL_NEXT);
			DISABLE_CONTROL_ACTION(0, INPUT_WEAPON_WHEEL_PREV);
			DISABLE_CONTROL_ACTION(0, INPUT_WEAPON_SPECIAL_TWO);
			DISABLE_CONTROL_ACTION(0, INPUT_DIVE);
			DISABLE_CONTROL_ACTION(0, INPUT_MELEE_ATTACK_LIGHT);
			DISABLE_CONTROL_ACTION(0, INPUT_MELEE_ATTACK_HEAVY);
			DISABLE_CONTROL_ACTION(0, INPUT_MELEE_BLOCK);
			DISABLE_CONTROL_ACTION(0, INPUT_ARREST);
			DISABLE_CONTROL_ACTION(0, INPUT_VEH_HEADLIGHT);
			DISABLE_CONTROL_ACTION(0, INPUT_VEH_RADIO_WHEEL);
			DISABLE_CONTROL_ACTION(0, INPUT_CONTEXT);
			DISABLE_CONTROL_ACTION(0, INPUT_RELOAD);
			DISABLE_CONTROL_ACTION(0, INPUT_DIVE);
			DISABLE_CONTROL_ACTION(0, INPUT_VEH_CIN_CAM);
			DISABLE_CONTROL_ACTION(0, INPUT_JUMP);
			DISABLE_CONTROL_ACTION(0, INPUT_VEH_SELECT_NEXT_WEAPON);
			DISABLE_CONTROL_ACTION(0, INPUT_VEH_FLY_SELECT_NEXT_WEAPON);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_CHARACTER_FRANKLIN);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_CHARACTER_MICHAEL);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_CHARACTER_TREVOR);
			DISABLE_CONTROL_ACTION(0, INPUT_SELECT_CHARACTER_MULTIPLAYER);
			DISABLE_CONTROL_ACTION(0, INPUT_CHARACTER_WHEEL);
		}
		static void update_status_text()
		{
			GET_SCREEN_RESOLUTION(&screen_res_x, &screen_res_y); // Get screen res
			if (menu::currentsub != SUB::CLOSED)
			{
				char *dword = "CommonMenu";
				if (!HAS_STREAMED_TEXTURE_DICT_LOADED(dword)) { REQUEST_STREAMED_TEXTURE_DICT(dword, 0); }
				background();
				optionhi();
			}
		}
		static void background()
		{

			float temp;
			if (totalop > 14) temp = 14; else temp = (float)totalop; // Calculate last option number to draw rect

			// Calculate Y Coord
			float bg_Y = ((temp * 0.035f) / 2.0f) + 0.159f;
			float bg_length = temp * 0.035f;

			// Draw Background Top (aka title box)
			DRAW_RECT(titleX + menuPos, titleY, 0.23f, titleHight, BG.R, BG.G, BG.B, BG.A);

			// Draw Line
			DRAW_RECT(0.16f + menuPos, lineY, 0.23f, 0.003f, line.R, line.G, line.B, line.A);
			
			// Draw background
			//DRAW_RECT(0.850f, 0.235f, 0.23f, 0.003f, BG.R, BG.G, BG.B, BG.A);
			DRAW_RECT(0.16f + menuPos, bg_Y, 0.23f, bg_length, BG.R, BG.G, BG.B, BG.A);

			// Draw scroller indicator rect
			if (totalop > 14) temp = 14.0f; else temp = (float)totalop;
			float scr_rect_Y = ((temp + 1.0f) * 0.035f) + 0.1415f;//0.5385
			DRAW_RECT(0.16f + menuPos, scr_rect_Y, 0.23f, 0.0345f, BG.R, BG.G, BG.B, BG.A);

			//Player Info
			if (featurePlayerInfo)
			{
				//marker
				Vector3 playerPosition = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(GET_PLAYER_PED(playerinfoSelected ? selectedPlayer : currentop - 1), 0.0, 0.0, 2.0);
				DRAW_MARKER(MarkerTypeChevronUpx2, playerPosition.x, playerPosition.y, playerPosition.z - 0.5f, 0.0f, 0.0f, 0.0f, 180.0f, 0.0f, 0.0f, 0.75f, 0.75f, 0.75f, 255, 0, 0, 255, true, true, 2, false, false, false, false);


				//line
				Vector3 LocalPed = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
				Vector3 TargetPed = GET_ENTITY_COORDS(GET_PLAYER_PED(playerinfoSelected ? selectedPlayer : currentop - 1), 1);
				DRAW_LINE(LocalPed.x, LocalPed.y, LocalPed.z, TargetPed.x, TargetPed.y, TargetPed.z, 255, 0, 0, 255);

				//text
				Vector3 myCoords = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
				Vector3 Coords = GET_ENTITY_COORDS(GET_PLAYER_PED(playerinfoSelected ? selectedPlayer : currentop - 1), 1);
				int health = GET_ENTITY_HEALTH(GET_PLAYER_PED(playerinfoSelected ? selectedPlayer : currentop - 1));
				int money = _STAT_GET_INT("MP0_TOTAL_CASH", -1);
				int Distance = GET_DISTANCE_BETWEEN_COORDS(myCoords.x, myCoords.y, myCoords.z, Coords.x, Coords.y, Coords.z, 0);
				int car = GET_VEHICLE_PED_IS_IN(GET_PLAYER_PED(playerinfoSelected ? selectedPlayer : currentop - 1), 0);
				int ISVehicle = IS_PED_IN_VEHICLE(GET_PLAYER_PED(playerinfoSelected ? selectedPlayer : currentop - 1), car, 1);
				int street = 0;
				int XCoord = Coords.x;
				int YCoord = Coords.y;
				int ZCoord = Coords.z;
				float speed = GET_ENTITY_SPEED(GET_PLAYER_PED(playerinfoSelected ? selectedPlayer : currentop - 1));
				int convertedspeed = (int)speed;
				char healthtext[60];
				char moneytext[60];
				char speedtext[60];
				char vehicletext[60];
				char entityXcoordstext[60];
				char entityYcoordstext[60];
				char entityZcoordstext[60];
				char distancetext[60];
				char modmenu[50];
				int armor = GET_PLAYER_MAX_ARMOUR(playerinfoSelected ? selectedPlayer : currentop - 1);
				switch (armor) {
				case 2027:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Limbo"); break;
				case 1327:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Independence"); break;
				case 1234:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Lexicon"); break;
				case 5759:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Trinity SPRX"); break;
				case 7777:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Predator SPRX"); break;
				case 631:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~GNXKS"); break;
				case 69:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Tesseract"); break;
				case 97:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Serendipity"); break;
				case 23:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Metropolis"); break;
				case 1996:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Power SPRX"); break;
				case 2001:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Project Eke"); break;
				case 2000:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~ICE"); break;
				case 337:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~GenocideFreeze"); break;
				case 4269:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~Cojones"); break;
				default:
					snprintf(modmenu, sizeof(modmenu), "Menu: ~r~N/A", armor); break;
				}
				sprintf(healthtext, "Health: ~r~ %d / 328", health);
				sprintf(moneytext, "Money: ~r~$%d", money);
				sprintf(vehicletext, "In Vehicle: ~r~ %d", ISVehicle);
				sprintf(speedtext, "Speed: ~r~ %d", convertedspeed);
				sprintf(distancetext, "Distance: ~r~ %d", Distance);
				sprintf(entityXcoordstext, "X: ~r~ %d", XCoord);
				sprintf(entityYcoordstext, "Y: ~r~ %d", YCoord);
				sprintf(entityZcoordstext, "Z: ~r~ %d", ZCoord);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.5f, 0.5f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.B, optiontext.G, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring("Player Info:", -0.038f + menuPos, 0.090f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(healthtext, -0.038f + menuPos, 0.130f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(moneytext, -0.038f + menuPos, 0.160f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(!strcmp(vehicletext, "In Vehicle: ~r~ 0") ? (char *)"In Vehicle: ~r~ On Foot" : (char *)"In Vehicle: ~r~ Yes", -0.038f + menuPos, 0.190f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(!strcmp(speedtext, "Speed: ~r~ 1") ? (char *)"Speed: ~r~ Walk" : !strcmp(speedtext, "Speed: ~r~ 0") ? (char *)"Speed: ~r~ Idle" : speedtext, -0.038f + menuPos, 0.220f);// plus 30
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(distancetext, -0.038f + menuPos, 0.250f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(entityXcoordstext, -0.038f + menuPos, 0.280f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(entityYcoordstext, -0.038f + menuPos, 0.310f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(entityZcoordstext, -0.038f + menuPos, 0.340f);
				setupdraw();
				SET_TEXT_FONT(4);
				SET_TEXT_SCALE(0.35f, 0.35f);
				SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
				SET_TEXT_CENTRE(0);
				drawstring(modmenu, -0.038f + menuPos, 0.370f);
				DRAW_RECT(-0.008f + menuPos, 0.2395f, 0.1f, 0.33f, BG.R, BG.G, BG.B, BG.A);
				DRAW_RECT(-0.008f + menuPos, 0.125f, 0.1f, 0.003f, line.R, line.G, line.B, line.A);
			}

			// Draw scroller indicator
			char * dword = "CommonMenu";
			if ((totalop > 14) && HAS_STREAMED_TEXTURE_DICT_LOADED(dword))
			{
				char * dword2 = "shop_arrows_upANDdown";
				char * dword3 = "arrowright";
				Vector3 texture_res = GET_TEXTURE_RESOLUTION(dword, dword2);

				temp = ((14.0f + 1.0f) * 0.035f) + 0.14190; //0.1259f

				if (currentop == 1)	DRAW_SPRITE(dword, dword3, 0.16f + menuPos, temp, texture_res.x / (float)screen_res_x, texture_res.y / (float)screen_res_y, 270.0f, BG.R, BG.G, BG.B, 255);
				else if (currentop == totalop) DRAW_SPRITE(dword, dword3, 0.16f + menuPos, temp, texture_res.x / (float)screen_res_x, texture_res.y / (float)screen_res_y, 90.0f, BG.R, BG.G, BG.B, 255);
				else DRAW_SPRITE(dword, dword2, 0.16f + menuPos, temp, texture_res.x / (float)screen_res_x, texture_res.y / (float)screen_res_y, 0.0f, BG.R, BG.G, BG.B, 255);

			}

			// Draw option count
			temp = scr_rect_Y - 0.0124f;
			setupdraw();
			SET_TEXT_FONT(0);
			SET_TEXT_SCALE(0.0f, 0.26f);
			SET_TEXT_JUSTIFICATION(2);
			SET_TEXT_WRAP(0.0f, 0.2540f + menuPos);
			SET_TEXT_CENTRE(0);
			SET_TEXT_COLOUR(optioncount.R, optioncount.G, optioncount.B, optioncount.A);

			_SET_TEXT_ENTRY("CM_ITEM_COUNT");
			ADD_TEXT_COMPONENT_INTEGER(currentop); // ! currentop_w_breaks 
			ADD_TEXT_COMPONENT_INTEGER(totalop); // ! totalop - totalbreaks
			_DRAW_TEXT(0.2540f + menuPos, temp);
		}
		static void optionhi()
		{
			float Y_coord;
			if (currentop > 14) Y_coord = 14.0f; else Y_coord = (float)currentop;

			Y_coord = (Y_coord * 0.035f) + 0.1415f;
			DRAW_RECT(0.16f + menuPos, Y_coord, 0.23f, 0.025f, scroller.R, scroller.G, scroller.B, scroller.A);
		}
		static bool isBinds()
		{
			// Open menu - RB + LB / NUM4 + NUM6 R1 + Dpad Left
			return ((IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_RB) && IS_DISABLED_CONTROL_PRESSED(2, INPUT_SCRIPT_PAD_LEFT)));
		}
		static void while_closed()
		{
			if (isBinds())
			{
				PlaySoundFrontend("FocusIn", "HintCamSounds");
				currentsub = 1;
				currentsub_ar_index = 0;
				currentop = 1;
				instructionsClose();
			}
		}
		static void while_opened()
		{
			totalop = printingop; printingop = 0;
			totalbreaks = breakcount; breakcount = 0; breakscroll = 0;

			if (IS_PAUSE_MENU_ACTIVE()) SetSub_closed();

			DISPLAY_AMMO_THIS_FRAME(0);
			DISPLAY_CASH(0);
			SET_RADAR_ZOOM(0);

			DisableControls();

			//instruction buttons
			AddInstruction(BUTTON_A, "Select");
			AddInstruction(BUTTON_DPAD_UP_DOWN, "Scroll");
			AddInstruction(BUTTON_B, "Back");
			instructionsClose();

			// Scroll up
			if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_SCRIPT_PAD_UP))
			{
				if (currentop == 1) Bottom(); else Up();
			}

			// Scroll down
			if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_SCRIPT_PAD_DOWN))
			{
				if (currentop == totalop) Top(); else Down();
			}

			// B press
			if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_SCRIPT_RRIGHT))
			{
				if (currentsub == SUB::MAINMENU) SetSub_closed(); else SetSub_previous();
			}

			// Binds press
			if (currentsub != SUB::MAINMENU && isBinds())
			{
				SetSub_closed();
			}
		}
		static void Up()
		{
			currentop--; currentop_w_breaks--;
			PlaySoundFrontend_default("NAV_UP_DOWN");
			breakscroll = 1;
		}
		static void Down()
		{
			currentop++; currentop_w_breaks++;
			PlaySoundFrontend_default("NAV_UP_DOWN");
			breakscroll = 2;
		}
		static void Bottom()
		{
			currentop = totalop; currentop_w_breaks = totalop;
			PlaySoundFrontend_default("NAV_UP_DOWN");
			breakscroll = 3;
		}
		static void Top()
		{
			currentop = 1; currentop_w_breaks = 1;
			PlaySoundFrontend_default("NAV_UP_DOWN");
			breakscroll = 4;
		}
		static void SetSub_previous()
		{
			currentsub = currentsub_ar[currentsub_ar_index]; // Get previous submenu from array and set as current submenu
			currentop = currentop_ar[currentsub_ar_index]; // Get last selected option from array and set as current selected option
			currentsub_ar_index--; // Decrement array index by 1
			printingop = 0; // Reset option print variable
			PlaySoundFrontend_default("BACK"); // Play sound
		}
		static void SetSub_new(int sub_index)
		{
			currentsub_ar_index++; // Increment array index
			currentsub_ar[currentsub_ar_index] = currentsub; // Store current submenu index in array
			currentsub = sub_index; // Set new submenu as current submenu (Static_1)

			currentop_ar[currentsub_ar_index] = currentop; // Store currently selected option in array
			currentop = 1; currentop_w_breaks = 1; // Set new selected option as first option in submenu

			printingop = 0; // Reset currently printing option var
		}
		static void SetSub_closed()
		{
			featurePlayerInfo = 0;
			ENABLE_ALL_CONTROL_ACTIONS(2);
			PlaySoundFrontend_default("BACK");
			currentsub = SUB::CLOSED;
			SET_SCALEFORM_MOVIE_AS_NO_LONGER_NEEDED(&SFMMPG);
		}
	};
	unsigned int menu::currentsub = 0; unsigned int menu::currentop = 0; unsigned int menu::currentop_w_breaks = 0; unsigned int menu::totalop = 0; unsigned int menu::printingop = 0; unsigned int menu::breakcount = 0; unsigned int menu::totalbreaks = 0; unsigned int menu::breakscroll = 0; int menu::currentsub_ar_index = 0; int menu::currentsub_ar[20] = {}; int menu::currentop_ar[20] = {}; int menu::SetSub_delayed = 0; unsigned long int menu::livetimer; bool menu::bit_centre_title = 0, menu::bit_centre_options = 0, menu::bit_centre_breaks = 1;
	bool CheckAJPressed()
	{
		if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_SCRIPT_RDOWN)) return true; else return false;
	}
	bool CheckRPressed()
	{
		if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_RIGHT)) return true; else return false;
	}
	bool CheckRJPressed()
	{
		if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_FRONTEND_RIGHT)) return true; else return false;
	}
	bool CheckLPressed()
	{
		if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_LEFT)) return true; else return false;
	}
	bool CheckLJPressed()
	{
		if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_FRONTEND_LEFT)) return true; else return false;
	}
	bool CheckSJPressed() //square
	{
		if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_SCRIPT_RLEFT)) return true; else return false;
	}
	bool IsOptionPressed()
	{
		if (CheckAJPressed())
		{
			PlaySoundFrontend_default("SELECT");
			return true;
		}
		else return false;
	}
	bool IsOptionRPressed()
	{
		if (CheckRPressed())
		{
			PlaySoundFrontend_default("NAV_LEFT_RIGHT");
			return true;
		}
		else return false;
	}
	bool IsOptionRJPressed()
	{
		if (CheckRJPressed())
		{
			PlaySoundFrontend_default("NAV_LEFT_RIGHT");
			return true;
		}
		else return false;
	}
	bool IsOptionLPressed()
	{
		if (CheckLPressed())
		{
			PlaySoundFrontend_default("NAV_LEFT_RIGHT");
			return true;
		}
		else return false;
	}
	bool IsOptionLJPressed()
	{
		if (CheckLJPressed())
		{
			PlaySoundFrontend_default("NAV_LEFT_RIGHT");
			return true;
		}
		else return false;
	}
	bool IsOptionSJPressed()
	{
		if (CheckSJPressed())
		{
			PlaySoundFrontend_default("NAV_LEFT_RIGHT");
			return true;
		}
		else return false;
	}
	int selectedOption()
	{
		if (IsOptionPressed())
			return (int)menu::currentop;
		else return 0;
	}
	void AddTitle(char* text)
	{
		setupdraw();
		SET_TEXT_FONT(font_title);

		SET_TEXT_COLOUR(titletext.R, titletext.G, titletext.B, titletext.A);

		if (menu::bit_centre_title)
		{
			SET_TEXT_CENTRE(1);
			OptionY = 0.016f + menuPos; // X coord
		}
		else OptionY = 0.066f + menuPos; // X coord

		if (Check_compare_string_length(text, 32))//14
		{
			SET_TEXT_SCALE(0.75f, 0.75f);
			drawstring(text, OptionY, 0.1f);
		}
		else drawstring(text, OptionY, 0.13f);

	}
	void nullFunc() { return; }
	void AddOption(char* text, bool &option_code_bool = null, void(&Func)() = nullFunc, int submenu_index = -1, char *desc = NULL)
	{
		char* tempChar;
		if (font_options == 2 || font_options == 7) tempChar = "  ------"; // Font unsafe
		else tempChar = "  ~r~->>"; // Font safe

		if (menu::printingop + 1 == menu::currentop && (font_selection == 2 || font_selection == 7)) tempChar = "  ------"; // Font unsafe
		else tempChar = "  ~r~->> "; // Font safe

		menu::printingop++;

		OptionY = 0.0f;
		if (menu::currentop <= 14)
		{
			if (menu::printingop > 14) return;
			else OptionY = ((float)(menu::printingop) * 0.035f) + 0.125f;
		}
		else
		{
			if (menu::printingop < (menu::currentop - 13) || menu::printingop > menu::currentop) return;
			else OptionY = ((float)(menu::printingop - (menu::currentop - 14))* 0.035f) + 0.125f;
		}

		setupdraw();
		SET_TEXT_FONT(font_options);
		SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
		if (menu::printingop == menu::currentop)
		{
			SET_TEXT_FONT(font_selection);
			SET_TEXT_COLOUR(selectedtext.R, selectedtext.G, selectedtext.B, selectedtext.A);
			if (IsOptionPressed())
			{
				if (&option_code_bool != &null) option_code_bool = true;
				Func();
				if (submenu_index != -1) menu::SetSub_delayed = submenu_index;
			}
		}

		if (submenu_index != -1) text = AddStrings(text, tempChar);
		if (menu::bit_centre_options)
		{
			SET_TEXT_CENTRE(1);
			drawstring(text, 0.16f + menuPos, OptionY);
		}
		else { drawstring(text, 0.066f + menuPos, OptionY); setupdraw(); SET_TEXT_FONT(font_options); SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A); SET_TEXT_SCALE(0.34f, 0.34f); SET_TEXT_JUSTIFICATION(2); SET_TEXT_WRAP(0.0f, 0.2540f + menuPos);	SET_TEXT_CENTRE(0); drawstring(desc, 0.2540f + menuPos, OptionY); }
	
	}
	void OptionStatus(int status) 
	{
		char * texture = "commonmenu";
		char *textureName = "common_medal";
		if (OptionY < 0.6325f && OptionY > 0.1425f)
		{

			if (status == 0) {// Off RED
				DrawSprite(texture, textureName, 0.255f + menuPos, 0.0133f + OptionY, 0.026, 0.026, 0.0, 255, 102, 102, 250);
				//"mpleaderboard", "leaderboard_globe_icon"
			}
			else if (status == 1) {// ON GREEN
				DrawSprite(texture, textureName, 0.255f + menuPos, 0.0133f + OptionY, 0.026, 0.026, 0.0, 102, 255, 102, 250);
			}
			else {//YELLOW
				DrawSprite(texture, textureName, 0.255f + menuPos, 0.0133f + OptionY, 0.026, 0.026, 0.0, 255, 255, 102, 250);
			}

		}
	}
	void AddToggle(char* text, bool &loop_variable, bool &extra_option_code_ON = null, bool &extra_option_code_OFF = null)
	{
		AddOption(text);

		if (menu::printingop == menu::currentop) {
			if (IsOptionPressed()) {
				loop_variable = !loop_variable;
				if (loop_variable && &extra_option_code_ON != &null) extra_option_code_ON = true;
				else if (!loop_variable && &extra_option_code_OFF != &null) extra_option_code_OFF = true;
			}
		}

		if (!loop_variable) OptionStatus(0); // Display OFF
		else if (loop_variable) OptionStatus(1); // Display ON
	}
	void AddKeyboard(char* text, keyboardActions action, char *defaultText = "", int maxLength = 60,  bool squareKey = false, bool &extra_option_code = null)
	{
		AddOption(text);

		if (menu::printingop == menu::currentop) {
			if (squareKey) {
				if (IsOptionSJPressed()) {
					StartKeyboard(action, defaultText, maxLength);
				}
			}
			else
			{
				if (IsOptionPressed()) {
					StartKeyboard(action, defaultText, maxLength);
				}
			}
		}
		if (&extra_option_code != &null) extra_option_code = true;
	}
	void AddLocal(char* text, Void condition, bool &option_code_ON, bool &option_code_OFF)
	{
		AddOption(text);
		if (menu::printingop == menu::currentop) {
			if (IsOptionPressed()) {
				if (condition == 0 && &option_code_ON != &null) option_code_ON = true;
				else if (condition != 0 && &option_code_OFF != &null) option_code_OFF = true;
			}
		}

		if (!condition) OptionStatus(0); // Display OFF
		else if (condition) OptionStatus(1); // Display ON
	}
	void AddBreak(char* text)
	{
		menu::printingop++; menu::breakcount++;

		OptionY = 0.0f;
		if (menu::currentop <= 14)
		{
			if (menu::printingop > 14) return;
			else OptionY = ((float)(menu::printingop) * 0.035f) + 0.125f;
		}
		else
		{
			if (menu::printingop < (menu::currentop - 13) || menu::printingop > menu::currentop) return;
			else OptionY = ((float)(menu::printingop - (menu::currentop - 14))* 0.035f) + 0.125f;
		}

		setupdraw();
		SET_TEXT_FONT(font_breaks);
		SET_TEXT_COLOUR(optionbreaks.R, optionbreaks.G, optionbreaks.B, optionbreaks.A);
		if (menu::printingop == menu::currentop)
		{
			switch (menu::breakscroll)
			{
			case 1:
				menu::currentop_w_breaks = menu::currentop_w_breaks + 1;
				menu::currentop--; break;
			case 2:
				menu::currentop_w_breaks = menu::currentop - menu::breakcount;
				menu::currentop++; break;
			case 3:
				menu::currentop_w_breaks = menu::totalop - (menu::totalbreaks - 1);
				menu::currentop--; break;
			case 4:
				menu::currentop_w_breaks = 1;
				menu::currentop++; break;
			}

		}
		if (menu::bit_centre_breaks)
		{
			SET_TEXT_CENTRE(1);
			drawstring(text, 0.16f + menuPos, OptionY);
		}
		else
		{
			drawstring(text, 0.066f + menuPos, OptionY);
		}

	}
	void AddNumber(char* text, float value, int decimal_places, bool &A_PRESS = null, bool &RIGHT_PRESS = null, bool &LEFT_PRESS = null, bool &SQUARE_PRESS = null)
	{
		AddOption(text, null);

		if (OptionY < 0.6325 && OptionY > 0.1425)
		{
			SET_TEXT_FONT(0);
			SET_TEXT_SCALE(0.275f, 0.275f);
			SET_TEXT_JUSTIFICATION(2);
			SET_TEXT_WRAP(0.0f, 0.2540f + menuPos);
			SET_TEXT_CENTRE(0);

			drawfloat(value, decimal_places, 0.2540f + menuPos, OptionY);
		}

		if (menu::printingop == menu::currentop)
		{
			if (IsOptionPressed() && &A_PRESS != &null) *&A_PRESS = true;
			else if (IsOptionRJPressed() && &RIGHT_PRESS != &null) RIGHT_PRESS = true;
			else if (IsOptionRPressed() && &RIGHT_PRESS != &null) RIGHT_PRESS = true;
			else if (IsOptionLJPressed() && &LEFT_PRESS != &null) LEFT_PRESS = true;
			else if (IsOptionLPressed() && &LEFT_PRESS != &null) LEFT_PRESS = true;
			else if (IsOptionSJPressed() && &SQUARE_PRESS != &null) { StartKeyboard(KB_DEFAULT, "", 60); /*keyboardVar = value;*/ }

		}

	}
	void AddItem(char *text, char **Item, int *Itemcount, int minimum, int maximum, bool isToggle = false, bool pState = NULL, bool isLabelString = false)
	{
		AddOption(text, null);
		if (isToggle)
		{
			if (IsOptionPressed()) {
				pState = !pState;
				if (!pState) OptionStatus(0); // Display OFF
				else if (pState) OptionStatus(1); // Display ON
			}
		}
		if (menu::printingop == menu::currentop)
		{
			if (OptionY < 0.6325 && OptionY > 0.1425)
			{
				if (isToggle)
				{
					setupdraw();
					SET_TEXT_FONT(font_selection);
					SET_TEXT_SCALE(0.34f, 0.34f);
					SET_TEXT_COLOUR(selectedtext.R, selectedtext.G, selectedtext.B, selectedtext.A);
					SET_TEXT_JUSTIFICATION(2);
					SET_TEXT_WRAP(0.0f, 0.2540f + menuPos);
					SET_TEXT_CENTRE(0);

					drawstring(isLabelString ?  _GET_LABEL_TEXT(GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(GET_HASH_KEY(Item[*Itemcount]))) : Item[*Itemcount], 0.2540f + menuPos, OptionY);
					
				}
				else
				{
					setupdraw();
					SET_TEXT_FONT(font_selection);
					SET_TEXT_SCALE(0.34f, 0.34f);
					SET_TEXT_COLOUR(selectedtext.R, selectedtext.G, selectedtext.B, selectedtext.A);
					SET_TEXT_JUSTIFICATION(2);
					SET_TEXT_WRAP(0.0f, 0.2540f + menuPos);
					SET_TEXT_CENTRE(0);

					drawstring(isLabelString ? _GET_LABEL_TEXT(GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(GET_HASH_KEY(Item[*Itemcount]))) : Item[*Itemcount], 0.2540f + menuPos, OptionY);
				}
			}
			if (IsOptionRJPressed())
			{
				if (*Itemcount >= maximum)
					*Itemcount = minimum;
				else
					*Itemcount = *Itemcount + 1;
			}
			else if (IsOptionLJPressed())
			{
				if (*Itemcount <= minimum)
					*Itemcount = maximum;
				else
					*Itemcount = *Itemcount - 1;
			}
		}
		else
		{
			if (OptionY < 0.6325 && OptionY > 0.1425)
			{
				if (isToggle)
				{
					setupdraw();
					SET_TEXT_FONT(font_selection);
					SET_TEXT_SCALE(0.34f, 0.34f);
					SET_TEXT_COLOUR(selectedtext.R, selectedtext.G, selectedtext.B, selectedtext.A);
					SET_TEXT_JUSTIFICATION(2);
					SET_TEXT_WRAP(0.0f, 0.2540f + menuPos);
					SET_TEXT_CENTRE(0);

					drawstring(isLabelString ? _GET_LABEL_TEXT(GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(GET_HASH_KEY(Item[*Itemcount]))) : Item[*Itemcount], 0.2540f + menuPos, OptionY);
				}
				else
				{
					setupdraw();
					SET_TEXT_FONT(font_selection);
					SET_TEXT_SCALE(0.34f, 0.34f);
					SET_TEXT_COLOUR(selectedtext.R, selectedtext.G, selectedtext.B, selectedtext.A);
					SET_TEXT_JUSTIFICATION(2);
					SET_TEXT_WRAP(0.0f, 0.2540f + menuPos);
					SET_TEXT_CENTRE(0);

					drawstring(isLabelString ?  _GET_LABEL_TEXT(GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(GET_HASH_KEY(Item[*Itemcount]))) : Item[*Itemcount], 0.2540f + menuPos, OptionY);
				}
			}
		}
	}
	void AddTeleport(char* text, Entity entity, float X, float Y, float Z, int flag, bool &extra_option_code = null)
	{
		AddOption(text, null);
		if (menu::printingop == menu::currentop)
		{
			if (IsOptionPressed())
			{
				RequestControlOfEnt(entity);
				//ptfx code here
				switch (flag)
				{
				case 0: _SET_ENTITY_COORDS_NO_OFFSET(entity, X, Y, Z, 0, 0, 1); break;
				case 1: if (!IS_PED_IN_ANY_VEHICLE(GET_PLAYER_PED(selectedPlayer), 0)) ShowSubtitle("~r~Error: Player not in vehicle"); _SET_ENTITY_COORDS_NO_OFFSET(entity, X, Y, Z, 0, 0, 1); break;
				case 2: break; //no code here plz
				case 3: break;
				case 4: break;
				}
				//LOAD_ALL_OBJECTS_NOW(); 
				if (&extra_option_code != &null) extra_option_code = true;
			}
		}
	}
	/*void AddObject(char* object, int flag, bool &extra_option_code = null)
	{
		AddOption(object, null);
		if (menu::printingop == menu::currentop)
		{
			if (IsOptionPressed())
			{
				//ptfx code here
				switch (flag)
				{
				case 0: ObjectCreater(GET_HASH_KEY(object)); break;
				case 1: break;
				case 2: break; //no code here plz
				case 3: break;
				case 4: break;
				}
				//LOAD_ALL_OBJECTS_NOW(); 
				if (&extra_option_code != &null) extra_option_code = true;
			}
		}
	}*/
	void RequestControlOfEntity(DWORD entity)
	{
		int tick = 0;
		while (!NETWORK_HAS_CONTROL_OF_ENTITY(entity) && tick <= 500) //999999
		{
			NETWORK_REQUEST_CONTROL_OF_ENTITY(entity);
			tick++;
		}
	}
	// functions:
	Entity grav_partfx, grav_entity, tempEntity, forge_entity; bool grav_toggled = 0, grav_target_locked = 0, forgegun_locked = 0, deletegun_locked;
	Vector3 get_coords_from_cam(float distance)
	{
		Vector3 Rot = GET_GAMEPLAY_CAM_ROT(2);
		Vector3 Coord = GET_GAMEPLAY_CAM_COORD();

		Rot.y = distance * COS(Rot.x);
		Coord.x = Coord.x + Rot.y * SIN(Rot.z * -1.0f);
		Coord.y = Coord.y + Rot.y * COS(Rot.z * -1.0f);
		Coord.z = Coord.z + distance * SIN(Rot.x);

		return Coord;
	}
	void set_gravity_gun()
	{
		DISABLE_PLAYER_FIRING(PLAYER_ID(), 1);
		Hash tempWeap;
		GET_CURRENT_PED_WEAPON(PLAYER_PED_ID(), &tempWeap, 1);
		if (IS_CONTROL_JUST_PRESSED(2, INPUT_FRONTEND_RT) && GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(PLAYER_ID(), &grav_entity) && DOES_ENTITY_EXIST(grav_entity) && tempWeap == WEAPON_PISTOL) { grav_target_locked = !grav_target_locked; if (IS_ENTITY_A_PED(grav_entity) && IS_PED_IN_ANY_VEHICLE(grav_entity, 1)) { grav_entity = GET_VEHICLE_PED_IS_IN(grav_entity, 0); } }
		if (grav_target_locked)
		{
			NETWORK_REQUEST_CONTROL_OF_ENTITY(grav_entity);
			Vector3 camvf = get_coords_from_cam(5.5f);
			SET_ENTITY_COORDS_NO_OFFSET(grav_entity, camvf.x, camvf.y, camvf.z, 0, 0, 0);
			if (IS_ENTITY_A_VEHICLE(grav_entity)) { SET_ENTITY_HEADING(grav_entity, GET_ENTITY_HEADING(PLAYER_PED_ID()) + 90.0f); }
		}
		else
		{
			if (DOES_ENTITY_EXIST(grav_entity) && (IS_ENTITY_A_PED(grav_entity) || IS_ENTITY_A_VEHICLE(grav_entity)))
			{
				SET_ENTITY_HEADING(grav_entity, GET_ENTITY_HEADING(PLAYER_PED_ID()));
				APPLY_FORCE_TO_ENTITY(grav_entity, 1, 0.0f, 350.0f, 2.0f, 2.0f, 0.0f, 0.0f, 0, 1, 1, 1, 0, 1);
				SET_ENTITY_AS_NO_LONGER_NEEDED(&grav_entity);
			}
		}
	}
	void TPtoWaypoint()
	{
		int WaypointHandle = GET_FIRST_BLIP_INFO_ID(8);
		if (DOES_BLIP_EXIST(WaypointHandle))
		{
			Vector3 WaypointPos = GET_BLIP_COORDS(WaypointHandle);
			int Handle = PLAYER_PED_ID();
			if (IS_PED_IN_ANY_VEHICLE(Handle, 0))
				Handle = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
			SET_ENTITY_COORDS(Handle, WaypointPos.x, WaypointPos.y, WaypointPos.z,1, 0, 1, 0);

		}
		else
			ShowSubtitle("~r~Error: No Waypoint Found");
	}
	void Stop()
	{
		CLEAR_PED_TASKS_IMMEDIATELY(PLAYER_PED_ID());

	}
	void ResetAppearance()
	{
		CLEAR_ALL_PED_PROPS(PLAYER_PED_ID());
		CLEAR_PED_DECORATIONS(PLAYER_PED_ID());
		SET_PED_COMPONENT_VARIATION(PLAYER_PED_ID(), 1, 0, 0, 0);
		SET_PED_COMPONENT_VARIATION(PLAYER_PED_ID(), 5, 0, 0, 0);
		SET_PED_COMPONENT_VARIATION(PLAYER_PED_ID(), 9, 0, 0, 0);
	}
	void changeAppearance(char* family, int model, int texture)
	{
		if (!strcmp(family, "HATS") || !strcmp(family, "GLASSES") || !strcmp(family, "EARS"))
		{
			if (!strcmp(family, "HATS"))
				fam = 0;
			else if (!strcmp(family, "GLASSES"))
				fam = 1;
			else if (!strcmp(family, "EARS"))
				fam = 2;
			SET_PED_PROP_INDEX(PLAYER_PED_ID(), fam, model - 1, texture, 0);
		}
		else
		{
			if (!strcmp(family, "FACE"))
				fam = 0;
			else if (!strcmp(family, "MASK"))
				fam = 1;
			else if (!strcmp(family, "HAIR"))
				fam = 2;
			else if (!strcmp(family, "TORSO"))
				fam = 3;
			else if (!strcmp(family, "LEGS"))
				fam = 4;
			else if (!strcmp(family, "HANDS"))
				fam = 5;
			else if (!strcmp(family, "SHOES"))
				fam = 6;
			else if (!strcmp(family, "SPECIAL1"))
				fam = 7;
			else if (!strcmp(family, "SPECIAL2"))
				fam = 8;
			else if (!strcmp(family, "SPECIAL3"))
				fam = 9;
			else if (!strcmp(family, "TEXTURE"))
				fam = 10;
			else if (!strcmp(family, "TORSO2"))
				fam = 11;
			SET_PED_COMPONENT_VARIATION(PLAYER_PED_ID(), fam, model, texture, 0);
		}
	}
	void ExplodeAllPlayers()
	{
		for (int i = 0; i<16; i++)
		{
			Vector3 coord = GET_ENTITY_COORDS(GET_PLAYER_PED(i), 0);
			ADD_EXPLOSION(coord.x,coord.y,coord.z, 29, 0.5f, true, false, 5.0f);
		}
	}
	void PlayerExplodeLobby()
	{
		Vector3 aa = GET_ENTITY_COORDS(GET_PLAYER_PED(0), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), aa.x,aa.y,aa.z, 0x1d, 5, 0, 1, 5);
		Vector3 a = GET_ENTITY_COORDS(GET_PLAYER_PED(1), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), a.x,a.y,a.z, 0x1d, 5, 0, 1, 5);
		Vector3 b = GET_ENTITY_COORDS(GET_PLAYER_PED(2), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), b.x,b.y,b.z, 0x1d, 5, 0, 1, 5);
		Vector3 c = GET_ENTITY_COORDS(GET_PLAYER_PED(3), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), c.x,c.y,c.z, 0x1d, 5, 0, 1, 5);
		Vector3 d = GET_ENTITY_COORDS(GET_PLAYER_PED(4), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), d.x,d.y,d.z, 0x1d, 5, 0, 1, 5);
		Vector3 e = GET_ENTITY_COORDS(GET_PLAYER_PED(5), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), e.x,e.y,e.z, 0x1d, 5, 0, 1, 5);
		Vector3 f = GET_ENTITY_COORDS(GET_PLAYER_PED(6), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), f.x,f.y,f.z, 0x1d, 5, 0, 1, 5);
		Vector3 g = GET_ENTITY_COORDS(GET_PLAYER_PED(7), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), g.x,g.y,g.z, 0x1d, 5, 0, 1, 5);
		Vector3 h = GET_ENTITY_COORDS(GET_PLAYER_PED(8), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), h.x,h.y,h.z, 0x1d, 5, 0, 1, 5);
		Vector3 i = GET_ENTITY_COORDS(GET_PLAYER_PED(9), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), i.x,i.y,i.z, 0x1d, 5, 0, 1, 5);
		Vector3 j = GET_ENTITY_COORDS(GET_PLAYER_PED(10), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), j.x,j.y,j.z, 0x1d, 5, 0, 1, 5);
		Vector3 k = GET_ENTITY_COORDS(GET_PLAYER_PED(11), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), k.x,k.y,k.z, 0x1d, 5, 0, 1, 5);
		Vector3 l = GET_ENTITY_COORDS(GET_PLAYER_PED(12), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), l.x,l.y,l.z, 0x1d, 5, 0, 1, 5);
		Vector3 m = GET_ENTITY_COORDS(GET_PLAYER_PED(13), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), m.x,m.y,m.z, 0x1d, 5, 0, 1, 5);
		Vector3 n = GET_ENTITY_COORDS(GET_PLAYER_PED(14), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), n.x,n.y,n.z, 0x1d, 5, 0, 1, 5);
		Vector3 o = GET_ENTITY_COORDS(GET_PLAYER_PED(15), 1);
		ADD_OWNED_EXPLOSION(GET_PLAYER_PED(selectedPlayer), o.x,o.y,o.z, 0x1d, 5, 0, 1, 5);
	}
	void GiveAllWeaponsEveryone()
	{
		for (int i = 0; i < 16; i++)
		{
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa2719263, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x99b507ea, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x678b81b1, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x4e875f73, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x958a4a8f, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x440e4788, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x84bd7bfd, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x1b06d571, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x5ef9fec4, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x22d8fe39, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x99aeeb3b, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x13532244, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x2be6766b, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xefe7e2df, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xbfefff6d, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x83bf0278, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xaf113f99, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x9d07f764, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7fd62962, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x1d073a89, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7846a318, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xe284c527, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x9d61e50f, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x3656c8c1, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x5fc3c11, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xc472fe2, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa284510b, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x4dd2dc56, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xb1ca77b1, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x687652ce, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x42bf8a85, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x93e220bd, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x2c3731d9, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xfdbc8a50, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa0973d5e, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x24b17070, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x60ec506, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x34a67b97, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x23c9f95c, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x497facc3, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xf9e6aa4b, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x61012683, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xc0a3098d, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xd205520e, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xbfd21232, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7f229f94, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x92a27487, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x83839c4, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa89cb99e, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7f7497e5, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x3aabbbaa, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xc734385a, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x63ab0442, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xaf2208a7, 9999, 1);
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x787f0bb, 9999, 1);
		}
	}
	void SpeedoKPM()
	{
		int car = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
		float speed = GET_ENTITY_SPEED(car);
		speed = speed * 4.30;
		int convertedspeed = (int)speed; /* create an integer object */
		char text[20]; /* create a place to store text */
		snprintf(text, 20, "Speed: ~r~%d (KMH)", convertedspeed); /* create textual representation of 'i' */
		/* and store it in the array 'text' */
		char *p = text; /* create a pointer and assign it the address of */
		/* the first character of the array 'text' */
		SpeedoText(text, 0.070, 0.890);
	}	
	Object ObjectCreater(Hash ObjectHash) {
		Vector3 dim_min, dim_max;
		GET_MODEL_DIMENSIONS(ObjectHash, &dim_min, &dim_max);
		Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PLAYER_PED_ID(), 0.0f, dim_max.y + 6.0f, 0.0f);
		SpawnedObj = CREATE_OBJECT(ObjectHash, coords.x, coords.y, coords.z, 1, 0, 0);
		RequestControlOfEnt(SpawnedObj);
		spawnedObjectCount++;
		return SpawnedObj;
	}
	Object ObjectForOnlinePlayer(Hash ObjectHash) {
		Vector3 coords = GET_ENTITY_COORDS(GET_PLAYER_PED(selectedPlayer),1);
		int obj = CREATE_OBJECT(ObjectHash, coords.x, coords.y, coords.z - 0.5f, 1, 0, 0);
		RequestControlOfEntity(obj);
		FREEZE_ENTITY_POSITION(obj, true);
	}
	void AddObject(char* object, int flag, bool &extra_option_code = null)
	{
		AddOption(object, null);
		if (menu::printingop == menu::currentop)
		{
			if (IsOptionPressed())
			{
				//ptfx code here
				switch (flag)
				{
				case 0: ObjectCreater(GET_HASH_KEY(object)); break;
				case 1: break;
				case 2: break; //no code here plz
				case 3: break;
				case 4: break;
				}
				//LOAD_ALL_OBJECTS_NOW(); 
				if (&extra_option_code != &null) extra_option_code = true;
			}
		}
	}
	void SnapGround()
	{
		RequestControlOfEntity(SpawnedObj);
		PLACE_OBJECT_ON_GROUND_PROPERLY(SpawnedObj);
	}
	bool FreezeSpawned;
	void FreezeObject()
	{
		if (!FreezeSpawned)
		{
			RequestControlOfEntity(SpawnedObj);
			FREEZE_ENTITY_POSITION(SpawnedObj, true);
			FreezeSpawned = true;
		}
		else
		{
			RequestControlOfEntity(SpawnedObj);
			FREEZE_ENTITY_POSITION(SpawnedObj, false);
			FreezeSpawned = false;
		}
	}
	void DeleteObject()
	{
		RequestControlOfEntity(SpawnedObj);
		DELETE_ENTITY(&SpawnedObj);
	}
	void LSC()
	{
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_ENEMYDRIVEBYKILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_USJS_COMPLETED"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_USJS_FOUND"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DB_PLAYER_KILLS"), 1000, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_KILLS_PLAYERS"), 1000, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FMHORDWAVESSURVIVE"), 21, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_CAR_BOMBS_ENEMY_KILLS"), 25, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_TDM_MVP"), 60, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_HOLD_UP_SHOPS"), 20, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_RACES_WON"), 101, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_NO_ARMWRESTLING_WINS"), 21, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FMBBETWIN"), 50000, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_DM_WINS"), 51, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_DM_TOTALKILLS"), 500, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MPPLY_DM_TOTAL_DEATHS"), 412, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MPPLY_TIMES_FINISH_DM_TOP_3"), 36, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_PLAYER_HEADSHOTS"), 623, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_DM_WINS"), 63, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_TDM_WINS"), 13, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_GTA_RACES_WON"), 12, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_GOLF_WON"), 2, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_SHOOTRANG_TG_WON"), 2, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_SHOOTRANG_RT_WON"), 2, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_SHOOTRANG_CT_WON"), 2, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_SHOOTRANG_GRAN_WON"), 2, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_FM_TENNIS_WON"), 2, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MPPLY_TENNIS_MATCHES_WON"), 2, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MPPLY_TOTAL_TDEATHMATCH_WON"), 63, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MPPLY_TOTAL_RACES_WON"), 101, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MPPLY_TOTAL_DEATHMATCH_LOST"), 23, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MPPLY_TOTAL_RACES_LOST"), 36, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_25_KILLS_STICKYBOMBS"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_50_KILLS_GRENADES"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_GRENADE_ENEMY_KILLS"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_AWD_20_KILLS_MELEE"), 50, 1);
	}
	void UnlockAllCharacter1()
	{
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_HAIR_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_JBIB_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_LEGS_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_FEET_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_BERD_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_8"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_9"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_PROPS_10"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_OUTFIT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_HAIR_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_JBIB_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_LEGS_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_FEET_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_BERD_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_8"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_9"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_PROPS_10"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_OUTFIT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_TORSO"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_SPECIAL2_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_DECL"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_TEETH"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_TEETH_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_AVAILAB LE_TEETH_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_TORSO"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_SPECIAL2_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_DECL"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_TEETH"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_TEETH_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CLTHS_ACQUIRE D_TEETH_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_0"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_8"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_9"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_10"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_11"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_12"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_13"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_14"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_15"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_16"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_17"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_18"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_19"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_21"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_22"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_23"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_24"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_24"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_25"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_26"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_27"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_28"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_29"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_30"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_31"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_32"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_33"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_34"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_35"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_36"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_37"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_38"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_39"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_DLC_APPAREL_A CQUIRED_40"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_8"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_9"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_10"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_11"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_12"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_13"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_1"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_10"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_11"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADMIN_CLOTHES _GV_BS_12"), -1, 1);
	}
	void WeaponSkins()
	{
		STAT_SET_INT(GET_HASH_KEY("MP0_MOLOTOV_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CMBTPISTOL_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_PISTOL50_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_APPISTOL_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MICROSMG_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SMG_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ASLTSMG_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ASLTRIFLE_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CRBNRIFLE_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ADVRIFLE_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MG_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CMBTMG_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ASLTMG_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_PUMP_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SAWNOFF_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_BULLPUP_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_ASLTSHTGN_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SNIPERRFL_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_HVYSNIPER_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_GRNLAUNCH_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_RPG_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MINIGUNS_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_GRENADE_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SMKGRENADE_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_STKYBMB_ENEMY_KILLS"), 600, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_MOLOTOV_ENEMY_KILLS"), 600, 1);
	}
	void skills()
	{
		STAT_SET_INT(GET_HASH_KEY("MP0_SCRIPT_INCREASE_STAM"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SCRIPT_INCREASE_STRN"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SCRIPT_INCREASE_LUNG"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SCRIPT_INCREASE_DRIV"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SCRIPT_INCREASE_FLY"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SCRIPT_INCREASE_SHO"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_SCRIPT_INCREASE_STL"), 100, 1);
	}
	void Heists()
	{
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE2"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE3"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE4"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE5"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE6"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE7"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE8"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE9"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE10"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE11"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_FM_PURCHASE12"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_1_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_2_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_3_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_4_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_5_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_6_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_7_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_8_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_9_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_10_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_11_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_KIT_12_FM_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_races_won"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_number_turbo_starts_in_race"), 100, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_usjs_found"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_usjs_completed"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_awd_fmwinairrace"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_awd_fmwinsearace"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_awd_fmrallywonnav"), 50, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_awd_fmrallywondrive"), 500, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_awd_fm_races_fastest_lap"), 500, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_0_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_1_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_2_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_3_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_4_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_5_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_6_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_char_fm_carmod_7_unlck"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_VEHICLE_1_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_VEHICLE_2_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_ABILITY_1_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_ABILITY_2_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_ABILITY_3_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_1_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_2_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_3_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_4_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_5_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_6_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_7_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_8_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_PACKAGE_9_COLLECT"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_HEALTH_1_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_CHAR_FM_HEALTH_2_UNLCK"), -1, 1);
		STAT_SET_INT(GET_HASH_KEY("MP0_HOLDUPS_BITSET"), -1, 1);
	}
	void clearArea()
	{
		Vector3 Pos = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
		CLEAR_AREA(Pos.x, Pos.y, Pos.z, 1000, 0, 0, 0, 0);
		CLEAR_AREA_OF_COPS(Pos.x, Pos.y, Pos.z, 1000, 0);
		CLEAR_AREA_OF_OBJECTS(Pos.x, Pos.y, Pos.z, 1000, 0);
		CLEAR_AREA_OF_VEHICLES(Pos.x, Pos.y, Pos.z, 1000, 0, 0, 0, 0, 0);
		CLEAR_AREA_OF_PEDS(Pos.x, Pos.y, Pos.z, 1000, 0);
		CLEAR_AREA_OF_PROJECTILES(Pos.x, Pos.y, Pos.z, 1000, 0);
	}
	void GiveAllWeaponsSelf()
	{
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa2719263, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x99b507ea, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x678b81b1, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x4e875f73, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x958a4a8f, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x440e4788, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x84bd7bfd, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x1b06d571, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x5ef9fec4, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x22d8fe39, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x99aeeb3b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x13532244, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x2be6766b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xefe7e2df, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xbfefff6d, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x83bf0278, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xaf113f99, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x9d07f764, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7fd62962, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x1d073a89, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7846a318, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xe284c527, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x9d61e50f, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x3656c8c1, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x5fc3c11, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xc472fe2, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa284510b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x4dd2dc56, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xb1ca77b1, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x687652ce, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x42bf8a85, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x93e220bd, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x2c3731d9, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xfdbc8a50, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa0973d5e, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x24b17070, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x60ec506, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x34a67b97, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x23c9f95c, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x497facc3, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xf9e6aa4b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x61012683, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xc0a3098d, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xd205520e, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xbfd21232, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7f229f94, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x92a27487, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x83839c4, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xa89cb99e, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x7f7497e5, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x3aabbbaa, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xc734385a, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x63ab0442, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xaf2208a7, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0x787f0bb, 9999, 1);
	}
	void TakeAllWeapons()
	{
		int pedid = PLAYER_PED_ID();
		REMOVE_ALL_PED_WEAPONS(pedid, 1);
	}
	void ApplyForce()
	{
		Vector3 force = { 0.0f, 0.0f, 100.0f };
		Vector3 force2 = { 0.0f, 0.0f, 0.0f };

		APPLY_FORCE_TO_ENTITY(PLAYER_PED_ID(), true, force.x, force.y, force.z, force2.x, force2.y, force2.z, true, true, true, true, false, true);

	}
	void MaxUpgrade()
	{
		int veh = GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID());
		SET_VEHICLE_MOD_KIT(veh, 0);
		SET_VEHICLE_COLOURS(veh, 120, 120);
		SET_VEHICLE_NUMBER_PLATE_TEXT(veh, "Joshk326");
		SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX(veh, 1);
		TOGGLE_VEHICLE_MOD(veh, 18, 1);
		TOGGLE_VEHICLE_MOD(veh, 22, 1);
		SET_VEHICLE_MOD(veh, 16, 5, 0);
		SET_VEHICLE_MOD(veh, 12, 2, 0);
		SET_VEHICLE_MOD(veh, 11, 3, 0);
		SET_VEHICLE_MOD(veh, 14, 14, 0);
		SET_VEHICLE_MOD(veh, 15, 3, 0);
		SET_VEHICLE_MOD(veh, 13, 2, 0);
		SET_VEHICLE_WHEEL_TYPE(veh, 6);
		SET_VEHICLE_WINDOW_TINT(veh, 5);
		SET_VEHICLE_MOD(veh, 23, 19, 1);
		SET_VEHICLE_MOD(veh, 0, 1, 0);
		SET_VEHICLE_MOD(veh, 1, 1, 0);
		SET_VEHICLE_MOD(veh, 2, 1, 0);
		SET_VEHICLE_MOD(veh, 3, 1, 0);
		SET_VEHICLE_MOD(veh, 4, 1, 0);
		SET_VEHICLE_MOD(veh, 5, 1, 0);
		SET_VEHICLE_MOD(veh, 6, 1, 0);
		SET_VEHICLE_MOD(veh, 7, 1, 0);
		SET_VEHICLE_MOD(veh, 8, 1, 0);
		SET_VEHICLE_MOD(veh, 9, 1, 0);
		SET_VEHICLE_MOD(veh, 10, 1, 0);
	}
	void SemiGMode()
	{
		int object = CREATE_n_ATTACH(GET_HASH_KEY("prop_juicestand"), GET_PLAYER_PED(selectedPlayer), SKEL_Head, 0.2, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 1, 0, 0);
		RequestControlOfEnt(object);
		SET_ENTITY_VISIBLE(object, 0);
	}
	void FreezeProtection()
	{
		Ped PlayerPED;
		Vector3 mycoords = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
		bool nearbyPed = GET_CLOSEST_PED(mycoords.x, mycoords.y, mycoords.z, 200, true,true, &PlayerPED, false, true, -1);
		//bool nearbyVeh = GET_CLOSEST_VEHICLE(mycoords.x, mycoords.y, mycoords.z, 200, );
		if (nearbyPed){
			NETWORK_REQUEST_CONTROL_OF_ENTITY(nearbyPed);
			if (NETWORK_HAS_CONTROL_OF_ENTITY(nearbyPed))
			{
				DETACH_ENTITY(nearbyPed, 1, 0);
				DELETE_ENTITY((Entity *)nearbyPed);
			}
		}
		/*if (nearbyVeh)
		{

		}*/
	}
	void GiveAllaponsClients(int Client)
	{
		int pedid = GET_PLAYER_PED(Client);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xa2719263, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x99b507ea, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x678b81b1, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x4e875f73, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x958a4a8f, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x440e4788, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x84bd7bfd, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x1b06d571, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x5ef9fec4, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x22d8fe39, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x99aeeb3b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x13532244, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x2be6766b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xefe7e2df, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xbfefff6d, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x83bf0278, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xaf113f99, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x9d07f764, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x7fd62962, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x1d073a89, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x7846a318, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xe284c527, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x9d61e50f, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x3656c8c1, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x5fc3c11, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xc472fe2, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xa284510b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x4dd2dc56, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xb1ca77b1, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x687652ce, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x42bf8a85, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x93e220bd, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x2c3731d9, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xfdbc8a50, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xa0973d5e, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x24b17070, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x60ec506, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x34a67b97, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x23c9f95c, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x497facc3, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xf9e6aa4b, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x61012683, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xc0a3098d, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xd205520e, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xbfd21232, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x7f229f94, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x92a27487, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x83839c4, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xa89cb99e, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x7f7497e5, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x3aabbbaa, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xc734385a, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x63ab0442, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0xaf2208a7, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x787f0bb, 9999, 1);
		GIVE_DELAYED_WEAPON_TO_PED(pedid, 0x47757124, 9999, 1);
		ShowSubtitle("Gave All ~r~Weapons");
	}
	void TakeAllWeapons(int client)
	{
		int pedid = GET_PLAYER_PED(client);
		REMOVE_ALL_PED_WEAPONS(pedid, 1);
	}
	void TPAllToSelf()
	{
		for (int i = 0; i < 16; i++)
		{
			int player = GET_PLAYER_PED(i);
			if (player > 0 && player != PLAYER_PED_ID());
			int myplayer = PLAYER_PED_ID();
			Vector3 me = GET_ENTITY_COORDS(myplayer, 1);
			REQUEST_ANIM_DICT("amb@world_human_hiker_standing@female@idle_a");
			HAS_ANIM_DICT_LOADED("amb@world_human_hiker_standing@female@idle_a");
			CLEAR_PED_TASKS_IMMEDIATELY(player);
			int sceneID = NETWORK_CREATE_SYNCHRONISED_SCENE(me.x, me.y, me.z, 0, 0, 0, 2, 0, 1, 1.0f, 0.0f, 1.0f);
			NETWORK_ADD_PED_TO_SYNCHRONISED_SCENE(player, sceneID, "amb@world_human_hiker_standing@female@idle_a", "idle_a", 8.0f, -8.0f, 2, 0, 100.0f);
			NETWORK_START_SYNCHRONISED_SCENE(sceneID);
			WAIT(7000);
			CLEAR_PED_TASKS_IMMEDIATELY(player);
		}
	}
	void DeleteAimingObjectFunction()
	{
		DISABLE_PLAYER_FIRING(PLAYER_ID(), 1);
		//Hash tempWeap;
		//GET_CURRENT_PED_WEAPON(PLAYER_PED_ID(), &tempWeap, 1);
		if (IS_CONTROL_JUST_PRESSED(2, INPUT_FRONTEND_RT) && GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(PLAYER_ID(), &tempEntity) && DOES_ENTITY_EXIST(tempEntity)) { deletegun_locked = !deletegun_locked; if (IS_ENTITY_A_PED(tempEntity) && IS_PED_IN_ANY_VEHICLE(tempEntity, 1)) { tempEntity = GET_VEHICLE_PED_IS_IN(tempEntity, 0); } }
		if (deletegun_locked)
		{
			NETWORK_REQUEST_CONTROL_OF_ENTITY(tempEntity);
			SET_ENTITY_AS_MISSION_ENTITY(tempEntity, 0, 1);
			DELETE_ENTITY(&tempEntity);
		}
		else
		{
			if (DOES_ENTITY_EXIST(tempEntity) && (IS_ENTITY_A_PED(tempEntity) || IS_ENTITY_A_VEHICLE(tempEntity)))
			{
				SET_ENTITY_HEADING(tempEntity, GET_ENTITY_HEADING(PLAYER_PED_ID()));
				SET_ENTITY_AS_NO_LONGER_NEEDED(&tempEntity);
			}
		}
	}
	void ForgeGunFunction()
	{
		
		DISABLE_PLAYER_FIRING(PLAYER_ID(), 1);
		//Hash tempWeap;
		//GET_CURRENT_PED_WEAPON(PLAYER_PED_ID(), &tempWeap, 1);
		if (IS_CONTROL_JUST_PRESSED(2, INPUT_FRONTEND_RT) && GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(PLAYER_ID(), &forge_entity) && DOES_ENTITY_EXIST(forge_entity)) { forgegun_locked = !forgegun_locked; if (IS_ENTITY_A_PED(forge_entity) && IS_PED_IN_ANY_VEHICLE(forge_entity, 1)) { forge_entity = GET_VEHICLE_PED_IS_IN(forge_entity, 0); } }
		if (forgegun_locked)
		{
			NETWORK_REQUEST_CONTROL_OF_ENTITY(forge_entity);
			SET_ENTITY_AS_MISSION_ENTITY(forge_entity, 0, 1);
			SpawnedObj = forge_entity;
		}
		else
		{
			if (DOES_ENTITY_EXIST(forge_entity) && (IS_ENTITY_A_PED(forge_entity) || IS_ENTITY_A_VEHICLE(forge_entity)))
			{
				SET_ENTITY_HEADING(forge_entity, GET_ENTITY_HEADING(PLAYER_PED_ID()));
				SpawnedObj = forge_entity;
				SET_ENTITY_AS_NO_LONGER_NEEDED(&forge_entity);
			}
		}
	}
	void TeleportGun()
	{
		Vector3 Loc[3];
		if (IS_PED_SHOOTING(PLAYER_PED_ID()))
		{
			if (GET_PED_LAST_WEAPON_IMPACT_COORD(PLAYER_PED_ID(), Loc[3]))
			{
				Vector3 Pos;
				Pos = Loc[0]; Pos = Loc[1]; Pos = Loc[2];
				SET_ENTITY_COORDS(PLAYER_PED_ID(), Pos.x, Pos.y, Pos.z, 1, 1, 0, 0);
			}
		}
	}
	void ESPAll()
	{
		for (int i = 0; i < 16; i++)
		{
			Vector3 LocalPed = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
			Vector3 TargetPed = GET_ENTITY_COORDS(GET_PLAYER_PED(i), 1);
			DRAW_LINE(LocalPed.x, LocalPed.y, LocalPed.z, TargetPed.x, TargetPed.y, TargetPed.z, 255, 0, 0, 255);
			DRAW_LINE(TargetPed.x, TargetPed.y, TargetPed.z, LocalPed.x, LocalPed.y, LocalPed.z, 255, 0, 0, 255);
		}
	}
	void TPPlayerToSelf()
	{
		int player = GET_PLAYER_PED(selectedPlayer);
		if (player > 0 && player != PLAYER_PED_ID());
		int myplayer = PLAYER_PED_ID();
		Vector3 me = GET_ENTITY_COORDS(myplayer, 1);
		REQUEST_ANIM_DICT("amb@world_human_hiker_standing@female@idle_a");
		HAS_ANIM_DICT_LOADED("amb@world_human_hiker_standing@female@idle_a");
		CLEAR_PED_TASKS_IMMEDIATELY(player);
		int sceneID = NETWORK_CREATE_SYNCHRONISED_SCENE(me.x, me.y, me.z, 0, 0, 0, 2, 0, 1, 1.0f, 0.0f, 1.0f);
		NETWORK_ADD_PED_TO_SYNCHRONISED_SCENE(player, sceneID, "amb@world_human_hiker_standing@female@idle_a", "idle_a", 8.0f, -8.0f, 2, 0, 100.0f);
		NETWORK_START_SYNCHRONISED_SCENE(sceneID);
		NETWORK_STOP_SYNCHRONISED_SCENE(sceneID);
	}
	void StealOutfit()
	{
		int player = GET_PLAYER_PED(selectedPlayer);
		if (GET_ENTITY_MODEL(PLAYER_PED_ID()) != GET_ENTITY_MODEL(GET_PLAYER_PED(selectedPlayer)))
			SET_PLAYER_MODEL(player, GET_ENTITY_MODEL(GET_PLAYER_PED(selectedPlayer)));

		for (int i = 0; i < 12; i++){
			int e_drawable = GET_PED_DRAWABLE_VARIATION(GET_PLAYER_PED(selectedPlayer), i);
			int e_texture = GET_PED_TEXTURE_VARIATION(GET_PLAYER_PED(selectedPlayer), e_drawable);
			SET_PED_COMPONENT_VARIATION(PLAYER_PED_ID(), i, e_drawable, e_texture, 0);
		}
	}
	void TEST()
	{
		int playerPed = PLAYER_PED_ID();
		REQUEST_NAMED_PTFX_ASSET("scr_exile3");
		_SET_PTFX_ASSET_NEXT_CALL("scr_exile3");
		START_PARTICLE_FX_NON_LOOPED_ON_ENTITY("scr_ex3_water_dinghy_wash", playerPed, 0.0, 0.0, -0.5, 0.0, 0.0, 0.0, 1.0, false, false, false);
	}
	void InitiateFreeze()
	{
		Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(GET_PLAYER_PED(selectedPlayer), 0.0, 6.0, 0.0);
		int car = freezeVehHash = GET_HASH_KEY("cargoplane"); featureFreezeVeh = true;
		/*RequestControlOfEnt(car);
		ATTACH_ENTITY_TO_ENTITY(car, GET_PLAYER_PED(selectedPlayer), SKEL_Spine1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0, 2, 1);
		RequestControlOfEnt(car);
		SET_ENTITY_VISIBLE(car, 0);*/
		ShowSubtitle("~r~Console Frozen");
	}
	void Nocliploop()
	{
		Vector3 rot = GET_GAMEPLAY_CAM_ROT(0);
		Vector3  dir = GET_GAMEPLAY_CAM_COORD();//5.0 
		float d = 0.09;

		if (IS_CONTROL_JUST_PRESSED(0, 52), IS_CONTROL_PRESSED(0, 55))
			d -= 0.0;
		int vehicle = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
		float Position[3];
		GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
		int ent = vehicle == 0 ? PLAYER_PED_ID() : vehicle;
		SET_ENTITY_COLLISION(ent, true, 0);
		SET_ENTITY_ROTATION(ent, rot.x, rot.y, rot.z,0,0);
		SET_ENTITY_COORDS(ent, Position[0], Position[1], Position[2] - 1,0,0,0,0);
		if (IS_CONTROL_PRESSED(2, 0xC6))
		{
			if (ent == vehicle)
			if (ent == PLAYER_PED_ID())
			{
				SET_ENTITY_COLLISION(ent, false, 0);
				SET_ENTITY_ROTATION(ent, rot.x, rot.y, rot.z, 0, 0);
			}
			SET_ENTITY_COORDS(ent, Position[0] + (dir.x), Position[1] + (dir.y), Position[2] + (dir.z),0,0,0,0);
		}
	}
	void JetAttack()
	{
		int Handle = GET_PLAYER_PED(selectedPlayer);
		Hash modelHash1 = 0x625D6958;// guard
		Hash modelHash2 = -1281684762;// Lazer
		if (IS_MODEL_VALID(modelHash1) && IS_MODEL_VALID(modelHash2) && IS_MODEL_A_VEHICLE(modelHash2) && IS_MODEL_IN_CDIMAGE(modelHash1))
		{
			featureJetAttack = 1;
			vehToSpawnHash = modelHash1;
			vehToSpawnHash1 = modelHash2;
			pedHandle = Handle, RequestModel(modelHash1);
			RequestModel(modelHash2);
		}
		else
		{
			ShowSubtitle("~r~Error: Model Isn't Valid");
		}
	}
	void AimbotLoop()
	{
		for (int i = 0; i < 16; i++)
		{
			TASK_AIM_GUN_AT_ENTITY(PLAYER_PED_ID(), GET_PLAYER_PED(i), 5, 0);
		}
	}
	/*void rideanimalloop()
	{
		if (DOES_ENTITY_EXIST(objecthandle) && IS_ENTITY_ATTACHED(PLAYER_PED_ID()))
		{
			int leftAxisXNormal = GET_CONTROL_NORMAL(0, BUTTON_LSTICK_LEFT);
			int leftAxisYNormal = GET_CONTROL_NORMAL(0, 189);
			int animal_speed = 2.0f;
			int range = 4.0f;
			if (menuInputHandler->AButtonDown())
			{
				animal_speed = 20.0f;
				range = 100.0f;
			}
			GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(objecthandle, leftAxisXNormal *range, leftAxisYNormal *range, 0.0f);
			TASK_GO_STRAIGHT_TO_COORD(objecthandle, moveto[0], moveto[1], moveto[2], animal_speed, 20000, 40000, 0.5f);
		}
	}*/
	void AnimationsForAll(char* dict, char* anim)
	{
		for (int i = 0; i < 16; i++)
		{
			int targ_ped = GET_PLAYER_PED(i);
			Vector3 coords = GET_ENTITY_COORDS(targ_ped, 1);
			int sceneID = NETWORK_CREATE_SYNCHRONISED_SCENE(coords.x, coords.y, coords.z, 0, 0, 0, 2, 0, 1, 1.0f, 0.0f, 1.0f);
			REQUEST_ANIM_DICT(dict);
			if (HAS_ANIM_DICT_LOADED(dict))
			{
				Vector3 coords = GET_ENTITY_COORDS(targ_ped, 1);
				int sceneID = NETWORK_CREATE_SYNCHRONISED_SCENE(coords.x, coords.y, coords.z, 0, 0, 0, 2, 0, 1, 1.0f, 0.0f, 1.0f);
				NETWORK_ADD_PED_TO_SYNCHRONISED_SCENE(targ_ped, sceneID, dict, anim, 8.0f, -8.0f, 269, 0, 100.0f);
				NETWORK_START_SYNCHRONISED_SCENE(sceneID);
			}
		}
	}






	}

namespace sub {

	void MainMenu()
	{
		bool onPress = 0;
		//featurePlayerInfo = 0;
		AddTitle("Iridium (BETA)");
		AddOption("Network", null, nullFunc, SUB::NETWORK_MAIN);//NETWORK_PLAYERS
		AddOption("Player Options", null, nullFunc, SUB::PLAYEROPTIONS);
		AddOption("Weapon Options", null, nullFunc, SUB::WEAPONOPTIONS);
		AddOption("Vehicle Options", null, nullFunc, SUB::VEHICLEOPTIONS);
		AddOption("Teleportations", null, nullFunc, SUB::TELEPORTATIONS);
		AddOption("Model Options", null, nullFunc, SUB::MODELOPTIONS);
		AddOption("Ped Options", null, nullFunc, SUB::PEDOPTIONS);
		AddOption("World Options", null, nullFunc, SUB::WORLDOPTIONS);
		AddOption("Object Menu", onPress, nullFunc, SUB::OBJECTMENU);
		AddOption("Miscellaneous", null, nullFunc, SUB::MISCELLANEOUS);
		AddOption("Menu Settings", null, nullFunc, SUB::SETTINGS);

		if (menujustopened)
		{
			ShowNotification("Welcome To ~r~Iridium"); // Your opening message goes here.
			menujustopened = false;
		}
		if (onPress)
		{
			featureObjectHash = 1; return;
		}
		else
		{
			featureObjectHash = 0; return;
		}
	}
	void Settings()
	{

		AddTitle("Menu Settings");

		AddOption("Menu Colors", null, nullFunc, SUB::SETTINGS_COLOURS);
		AddToggle("Center Title", menu::bit_centre_title);
		AddToggle("Center Options", menu::bit_centre_options);
		AddItem("Menu Position", (char **)MenuPosition, &menuPositionIndex, 0, 2);
		switch (menuPositionIndex) { case 0: menuPos = 0.658f; break; case 1: menuPos = 0.358f; break; case 2: menuPos = 0.055f; break; }

	}
	void AddsettingscolOption(char* text, RGBA &feature)
	{
		AddOption(text, null, nullFunc, SUB::SETTINGS_COLOURS2);

		if (menu::printingop == menu::currentop) settings_rgba = &feature;
	}
	void SettingsColors()
	{
		AddTitle("Menu Colors");
		AddsettingscolOption("Background", BG);
		AddsettingscolOption("Top Line", line);
		AddsettingscolOption("Scroll Bar", scroller);
		AddsettingscolOption("Title Text", titletext);
		AddsettingscolOption("Option Text", optiontext);
		AddsettingscolOption("Selected Text", selectedtext);
		AddsettingscolOption("Option Breaks", optionbreaks);
		AddsettingscolOption("Option Count", optioncount);
		AddToggle("Rainbow", loop_RainbowBoxes);
	}
	void SettingsColors2()
	{
		bool settings_r_input = 0, settings_r_plus = 0, settings_r_minus = 0;

		AddTitle("Set Color");
		AddNumber("Red", settings_rgba->R, 0, settings_r_input, settings_r_plus, settings_r_minus);
		AddNumber("Green", settings_rgba->G, 0, settings_r_input, settings_r_plus, settings_r_minus);
		AddNumber("Blue", settings_rgba->B, 0, settings_r_input, settings_r_plus, settings_r_minus);
		AddNumber("Opacity", settings_rgba->A, 0, settings_r_input, settings_r_plus, settings_r_minus);

		switch (menu::currentop)
		{
		case 1: settings_rgba2 = &settings_rgba->R; break;
		case 2: settings_rgba2 = &settings_rgba->G; break;
		case 3: settings_rgba2 = &settings_rgba->B; break;
		case 4: settings_rgba2 = &settings_rgba->A; break;
		}

		if (settings_r_input) {
			StartKeyboard(KB_SETTINGS_RGB2, "", 30);
			return;
		}

		if (settings_r_plus) {
			if (*settings_rgba2 < 255) (*settings_rgba2)++;
			else *settings_rgba2 = 0;
			return;
		}
		else if (settings_r_minus) {
			if (*settings_rgba2 > 0) (*settings_rgba2)--;
			else *settings_rgba2 = 255;
			return;
		}
	}
	void PlayerOptions()
	{
		// Initialise local variables here:
		bool invincibilityOff = 0, superRunOff = 0, invisibilityOff = 0, neverWantedOff = 0, unlimitedAmmoOff = 0, seatbeltOff = 0,
			ragDollOff = 0, supermanON = 0, noclipON = 0, noclipOFF = 0;
		// Options' text here:
		AddTitle("Player Options");

		AddToggle("Godmode", featureInvincibility, null, invincibilityOff);
		AddToggle("Unlimited Ammo", featureUnlimitedAmmo, null, unlimitedAmmoOff);
		AddToggle("Never Wanted", featureNeverWanted, null, neverWantedOff);
		AddToggle("Seat Belt", featureSeatBelt, null, seatbeltOff);
		AddToggle("Super Run", featureSuperRun, null, superRunOff);
		AddToggle("Super Jump", featureSuperJump);
		AddToggle("Invisibility", featureInvisibility, null, invisibilityOff);
		AddToggle("No Rag-Doll", featureRagDoll, null, ragDollOff);
		AddToggle("Superman Mod", featureSuperMan, supermanON);
		AddToggle("Innerforce", featureInnerForce, null);
		AddToggle("Noclip Freecam", featurenoclip, noclipON, noclipOFF);
		AddOption("Recovery Menu", null, nullFunc, SUB::RECOVERYMENU);
		AddOption("Animation Options", null, nullFunc, SUB::ANIMATIONOPTIONS);

		// Options' code here:
		// Always use return; to exit to the switch if you don't have code below that you want executed.
		if (invincibilityOff) { SET_PLAYER_INVINCIBLE(PLAYER_ID(), FALSE); SET_ENTITY_INVINCIBLE(PLAYER_PED_ID(), FALSE); SET_ENTITY_ONLY_DAMAGED_BY_PLAYER(PLAYER_PED_ID(), FALSE); SET_ENTITY_CAN_BE_DAMAGED(PLAYER_PED_ID(), TRUE);  return; }
		if (superRunOff) { SET_PED_CAN_RAGDOLL(PLAYER_PED_ID(), FALSE); SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(PLAYER_PED_ID(), FALSE); SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(PLAYER_PED_ID(), TRUE); return; }
		if (invisibilityOff) { SET_ENTITY_VISIBLE(PLAYER_PED_ID(), TRUE); return; }
		if (neverWantedOff) { SET_MAX_WANTED_LEVEL(5); return; }
		if (ragDollOff){ SET_PED_CAN_RAGDOLL(PLAYER_PED_ID(), TRUE); SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(PLAYER_PED_ID(), TRUE); return; }
		if (unlimitedAmmoOff){ SET_PED_INFINITE_AMMO_CLIP(PLAYER_PED_ID(), FALSE); return; }
		if (seatbeltOff){ SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(PLAYER_PED_ID(), 0); SET_ENTITY_PROOFS(PLAYER_PED_ID(), FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE); featureRagDoll = 0; return; }
		if (supermanON)
		{
			GIVE_DELAYED_WEAPON_TO_PED(PLAYER_PED_ID(), 0xFBAB5776, 1, true); ApplyForce(); ShowSubtitle("Use ~r~R2~s~ To Go Forward, And ~r~L2~s~ To Go Backwards"); return;
		}
		if (noclipON)
		{
			SET_ENTITY_VISIBLE(PLAYER_PED_ID(), 0);
		}
		else if (noclipOFF)
		{
			SET_ENTITY_VISIBLE(PLAYER_PED_ID(), 1);
		}

	}
	void VehicleOptions()
	{
		// Initialise local variables here:
		bool empty = 0, origspeed = 0, chrom3speed = 0, flyOFF = 0, collisionON = 0, collisionOFF = 0, waterDriveON = 0, waterDriveOFF = 0, needSpeedON = 0, torque_plus = 0, torque_minus = 0, rpm_plus = 0, rpm_minus = 0;

		// Options' text here:
		AddTitle("Vehicle Options");

		AddOption("Vehicle Weapons", null, nullFunc, SUB::VEHICLE_WEAPONS);
		AddOption("Vehicle Spawner", null, nullFunc, SUB::VEHICLE_SPAWNER);
		AddOption("Vehicle Editor", null, nullFunc, SUB::VEHICLE_EDITOR);
		AddOption("Repair Vehicle");
		AddOption("Flip Upright");;
		AddOption("Max Upgrades", null, MaxUpgrade);
		AddKeyboard("Change Plate Text", KB_CHANGE_PLATE_TEXT);
		AddToggle("Toggle Invisibility", featureVehInvisibility);
		AddToggle("Toggle Doors", featureToggleDoors);
		AddToggle("Freeze Vehicle", featureFreezeVehicle);
		AddToggle("No Collision", featureNoCollision, collisionON, collisionOFF);
		AddToggle("Vehicle Jump", featureCarJump);
		AddToggle("Drive On Water", featureWaterDrive, waterDriveON, waterDriveOFF);
		AddBreak("---Speedometer Options---");
		AddToggle("Original Speedo", featureSpeedoSpeedText, null, origspeed);
		AddToggle("Chr0m3 x MoDz Speedo", featureSpeedometerSkin, null, chrom3speed);
		AddBreak("---Handling---");
		AddToggle("Need For Speed", featureNeedForSpeed, needSpeedON);
		AddToggle("Fly Vehicle", featureHandlingFly, null, flyOFF);
		AddNumber("Torque Multiplier:", torque, 0, null, torque_plus, torque_minus);
		AddNumber("RPM Multiplier:", rpm, 0, null, rpm_plus, rpm_minus);

		switch (selectedOption())
		{
		case 4: RequestControlOfEntM(myVeh, RC_FIXSELFVEHICLE); break;
		case 5: SET_VEHICLE_ON_GROUND_PROPERLY(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID())); break;
		case 8: RequestControlOfEntM(myVeh, RC_TOGGLEDOORSSELF); break;
		case 9: RequestControlOfEntM(myVeh, RC_TOGGLEDOORSSELF); break;
		case 10: RequestControlOfEntM(myVeh, RC_FREEZESELFVEHICLE); break;
		}
		if (torque_plus) {
			if (torque < 1000) (torque)++;
			else torque = 0;
			_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), torque);
			return;
		}
		else if (torque_minus) {
			if (torque > 0) (torque)--;
			else torque = 1000;
			_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), torque);
			return;
		}
		if (rpm_plus) {
			if (rpm < 1000) (rpm)++;
			else rpm = 0;
			_SET_VEHICLE_ENGINE_POWER_MULTIPLIER(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), rpm);
			return;
		}
		else if (rpm_minus) {
			if (rpm > 0) (rpm)--;
			else rpm = 1000;
			_SET_VEHICLE_ENGINE_POWER_MULTIPLIER(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), rpm);
			return;
		}
		if (waterDriveON)
		{
			featureWaterDrive = 1;
			DriveWater = 1;
			WaterAir = 1.732711;
			SET_VEHICLE_ON_GROUND_PROPERLY(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()));
		}
		else if (waterDriveOFF)
		{
			featureWaterDrive = 0;
			DriveWater = 0;
		}
		if (flyOFF)
		{
			SET_VEHICLE_GRAVITY(myVeh, FALSE);
		}
		else
		{
			SET_VEHICLE_GRAVITY(myVeh, TRUE);
		}
		if (collisionON)
		{
			SET_ENTITY_COLLISION(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), 0, 0);
		}
		else if (collisionOFF)
		{
			SET_ENTITY_COLLISION(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), 1, 1);
		}
		if (needSpeedON)
		{
			SET_VEHICLE_ON_GROUND_PROPERLY(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()));
			
		}
	}
	void VehicleSpawnerOptions()
	{
		// Initialise local variables here:
		bool supers_plus = 0, supers_minus = 0, sports_plus = 0, sports_minus = 0;

		// Options' text here:
		AddTitle("Vehicle Spawner");
		AddOption("Spawn Settings", null, nullFunc, SUB::VEHICLE_SPAWNER_SETTINGS);
		AddBreak("---Cars---");
		AddItem("Supers", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 0], &SuperIndex, 0, 10, 0, 0, 1);
		AddItem("Sports", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 2], &SportsIndex, 0, 23, 0, 0, 1);
		AddItem("Sports Classic", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 3], &SportsClassicIndex, 0, 13, 0, 0, 1);
		AddItem("Coupes", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 7], &CoupesIndex, 0, 12, 0, 0, 1);
		AddItem("Muscle", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 4], &MuscleIndex, 0, 12, 0, 0, 1);
		AddItem("Offroad", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 8], &OffroadIndex, 0, 20, 0, 0, 1);
		AddItem("SUVs", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 10], &SUVsIndex, 0, 17, 0, 0, 1);
		AddItem("Sedans", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 6], &SedansIndex, 0, 19, 0, 0, 1);
		AddItem("Compact", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 5], &CompactIndex, 0, 6, 0, 0, 1);
		AddItem("Vans", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 16], &VansIndex, 0, 15, 0, 0, 1);
		AddItem("Trucks", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 9], &TrucksIndex, 0, 21, 0, 0, 1);
		AddItem("Services", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 14], &ServicesIndex, 0, 6, 0, 0, 1);
		AddItem("Trailers", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 24], &TrailersIndex, 0, 16, 0, 0, 1);
		AddItem("Trains", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 23], &TrainsIndex, 0, 11, 0, 0, 1);
		AddBreak("---Others---");
		AddItem("Emergency", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 12], &EmergencyIndex, 0, 16, 0, 0, 1);
		AddItem("Motocycles", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 17], &MotocyclesIndex, 0, 19, 0, 0, 1);
		AddItem("Cycles", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 18], &CyclesIndex, 0, 5, 0, 0, 1);
		AddItem("Planes", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 19], &PlanesIndex, 0, 16, 0, 0, 1);
		AddItem("Helicopers", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 20], &HelicoptersIndex, 0, 11, 0, 0, 1);
		AddItem("Boats", (char **)vehicleModels[vehiclespawnerActiveLineIndex = 21], &BoatsIndex, 0, 13, 0, 0, 1);
		AddKeyboard("Input Model", KB_DEFAULT);
		// Options' code here: 
		switch (selectedOption()) {
		case 3: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 0][SuperIndex]); featureCreateVehicle = true; break;
		case 4: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 2][SportsIndex]); featureCreateVehicle = true; break;
		case 5: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 3][SportsClassicIndex]); featureCreateVehicle = true; break;
		case 6: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 7][CoupesIndex]); featureCreateVehicle = true; break;
		case 7: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 4][MuscleIndex]); featureCreateVehicle = true; break;
		case 8: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 8][OffroadIndex]); featureCreateVehicle = true; break;
		case 9: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 10][SUVsIndex]); featureCreateVehicle = true; break;
		case 10: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 6][SedansIndex]); featureCreateVehicle = true; break;
		case 11: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 5][CompactIndex]); featureCreateVehicle = true; break;
		case 12: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 16][VansIndex]); featureCreateVehicle = true; break;
		case 13: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 9][TrucksIndex]); featureCreateVehicle = true; break;
		case 14: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 14][ServicesIndex]); featureCreateVehicle = true; break;
		case 15: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 24][TrailersIndex]); featureCreateVehicle = true; break;
		case 17: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 23][TrainsIndex]); featureCreateVehicle = true; break;
		case 18: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 12][EmergencyIndex]); featureCreateVehicle = true; break;
		case 19: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 17][MotocyclesIndex]); featureCreateVehicle = true; break;
		case 20: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 18][CyclesIndex]); featureCreateVehicle = true; break;
		case 21: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 19][PlanesIndex]); featureCreateVehicle = true; break;
		case 22: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 20][HelicoptersIndex]); featureCreateVehicle = true; break;
		case 23: vehicleCustomHash = GET_HASH_KEY((char *)vehicleModels[vehiclespawnerActiveLineIndex = 21][BoatsIndex]); featureCreateVehicle = true; break;
		}


	}
	void VehicleSpawnerSettings()
	{
		AddTitle("Spawn Settings");
		AddToggle("Warp In Spawn", featureVehWarpInSpawned);
		AddToggle("Invincible On Spawn", featureInvincibleVehicleOnSpawn);
		AddToggle("Max All Upgrades", featureMaxAllUpgrades);
	}
	void Teleportations()
	{
		Vector3 Coords = GET_ENTITY_COORDS(PLAYER_PED_ID(),1);
		int VehicleHandle = GET_CLOSEST_VEHICLE(Coords.x,Coords.y,Coords.z, 100.0, 0, 71);
		Entity ent = PLAYER_PED_ID();
		if (IS_PED_IN_ANY_VEHICLE(ent, 0))
			ent = GET_VEHICLE_PED_IS_IN(ent, 0);

		AddTitle("Teleportations");
		AddOption("Teleport To Waypoint", null, TPtoWaypoint);
		AddOption("Teleport In Nearest Vehicle");
		AddTeleport("Eclipse Towers",ent, -775.0500, 312.3200, 85.7000,0);
		AddTeleport("Fort Zancudo", ent, -2012.8470, 2956.5270, 32.8101, 0);
		AddTeleport("FIB Building ~HUD_COLOUR_GREY~(Inside)", ent, 135.5220, -749.0009, 260.0000, 0);
		AddTeleport("Airport", ent, -1102.2910, -2894.5160,13.9467, 0);
		AddTeleport("Trevors Airfield", ent, 1741.4960, 3269.2570, 41.6014, 0);
		AddTeleport("LSC", ent, -354.6175, -121.153, 38.6982, 0);
		AddTeleport("Hangout Spot", ent, -125.6544, 7271.8940, 16.7366, 0);
		AddTeleport("Cannibal Mountain", ent, -935.9363, 4836.7470, 310.5199, 0);
		AddTeleport("Cloths Store", ent, -159.2996, -304.3292, 39.7333, 0);
		AddTeleport("Beach", ent, -1624.1540, -1165.0890, 2.0955, 0);
		switch (selectedOption())
		{
		case 2: SET_PED_INTO_VEHICLE(PLAYER_PED_ID(), VehicleHandle, -1);break;
		}
	}
	void WorldOptions()
	{
		bool mtchilramp = 0, airportramp = 0, hour_plus = 0, hour_minus = 0, BlackON = 0, BlackOFF = 0;

		int hour = GET_CLOCK_HOURS();

		Entity ent = PLAYER_PED_ID(); 
		if (IS_PED_IN_ANY_VEHICLE(ent, 0)) 
			ent = GET_VEHICLE_PED_IS_IN(ent, 0); 

		AddTitle("World Options");

		AddNumber("Hour", GET_CLOCK_HOURS(), 0, null, hour_plus, hour_minus);
		AddItem("Weather", (char**)WeatherTypes, &weatherTypeIndex, 0, 11);
		AddToggle("Black-Out", featureBlackOut, BlackON, BlackOFF);
		AddBreak("---Map Mods---");
		AddOption("Load Mt. Chiliad Ramp", mtchilramp);
		AddTeleport("Teleport to Mt. Chiliad Ramp", ent, 509.8423f, 5589.2422f, 792.0000f, 0);
		AddOption("Load Airport Ramp", airportramp);
		AddTeleport("Teleport to Airport Ramp", ent, -1102.2910, -2894.5160, 13.9467, 0);

		if (BlackON)
		{
			_SET_BLACKOUT(1); return;
		}
		else if (BlackOFF)
		{
			_SET_BLACKOUT(0); return;
		}
		if (mtchilramp) {
			Hash tempHashcont = GET_HASH_KEY("prop_container_01a");
			PlaceObject(tempHashcont, 509.8423f, 5589.2422f, 791.0656f, 0.1410f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 520.5002f, 5584.3774f, 790.5033f, 5.4410f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 531.0571f, 5579.5405f, 788.6912f, 12.4410f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 541.3253f, 5574.8403f, 785.4896f, 19.4409f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 551.0662f, 5570.3701f, 780.7990f, 27.5407f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 560.1738f, 5566.2046f, 774.6979f, 35.0403f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 568.6718f, 5562.3198f, 767.4281f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 576.9716f, 5558.5269f, 759.5663f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 585.2493f, 5554.7471f, 751.7451f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 593.5072f, 5550.9722f, 743.9170f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 601.7770f, 5547.1912f, 736.0764f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 610.0651f, 5543.3994f, 728.2167f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 618.3337f, 5539.6226f, 720.3861f, 40.7936f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 626.6017f, 5535.8477f, 712.5473f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 634.8616f, 5532.0669f, 704.7252f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 643.1213f, 5528.2861f, 696.8940f, 40.7936f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 651.3914f, 5524.5059f, 689.0526f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 659.6512f, 5520.7275f, 681.2211f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 667.9110f, 5516.9424f, 673.3893f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 676.1708f, 5513.1670f, 665.5580f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 684.4307f, 5509.3789f, 657.7266f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 692.6906f, 5505.6079f, 649.9052f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 700.9504f, 5501.8271f, 642.0737f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 709.2201f, 5498.0464f, 634.2426f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 717.4602f, 5494.2759f, 626.4309f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 725.7202f, 5490.4980f, 618.5996f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 733.9800f, 5486.7226f, 610.7783f, 40.7396f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 742.5997f, 5482.7764f, 603.1669f, 36.9395f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 751.8304f, 5478.5518f, 596.3347f, 31.0392f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 761.7103f, 5474.0220f, 590.6132f, 24.5989f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 772.0702f, 5469.2827f, 586.0803f, 18.9288f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 782.8400f, 5464.3433f, 582.8604f, 11.5788f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 793.8899f, 5459.2847f, 581.1174f, 5.0787f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 805.1001f, 5454.1479f, 580.8762f, -2.5212f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 816.1702f, 5449.0796f, 581.9746f, -7.6213f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 827.1907f, 5444.0405f, 584.5823f, -16.6212f, 0.0f, 65.3998f, 0);
			PlaceObject(tempHashcont, 837.6807f, 5439.2407f, 588.8990f, -24.4210f, 0.0f, 65.3998f, 0);
			ShowNotification(AddStrings("Map by ~b~", "XBLToothpik"));
			return;
		}
		if (airportramp){
			Hash tempHashcont = GET_HASH_KEY("prop_lev_des_barge_02");
			PlaceObject(tempHashcont, -1426.886, -2497.702, 19.9720, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1413.186, -2469.902, 38.3891, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1399.487, -2442.101, 56.8062, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1385.687, -2414.402, 75.1943, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1371.987, -2386.702, 93.5514, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1358.257, -2358.993, 111.9185, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1344.457, -2331.294, 130.3056, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1330.557, -2303.493, 148.7926, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1316.857, -2275.794, 167.1497, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1303.057, -2248.085, 185.5368, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1289.257, -2220.385, 203.9238, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1275.457, -2192.685, 222.3109, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1261.758, -2164.887, 240.7280, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1247.958, -2137.185, 259.1151, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1234.238, -2109.405, 277.5222, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1220.538, -2081.704, 295.8792, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1206.868, -2054.017, 314.2263, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1193.068, -2026.417, 332.5634, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1179.368, -1998.676, 350.9403, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1165.648, -1970.866, 369.3675, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1151.948, -1943.167, 387.7346, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1138.148, -1915.467, 406.1316, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1124.449, -1887.667, 424.5489, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1110.749, -1859.937, 442.9361, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1097.009, -1832.238, 461.3032, 30.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1082.210, -1802.338, 470.6203, 0.7213, 0.1092, -26.3437,0);
			PlaceObject(tempHashcont, -1090.710, -1798.137, 470.6374, 0.7213, 0.1092, -26.3437,0);
			PlaceObject(0x9C762726, -1073.909, -1806.737, 470.6545, 0.7213, 0.1092, -26.3437,0);
			return;
		}

		if (hour_plus) {
			if (hour + 1 == 24) NETWORK_OVERRIDE_CLOCK_TIME(0, 0, 0);
			else NETWORK_OVERRIDE_CLOCK_TIME((hour + 1), 0, 0);
			return;
		}
		else if (hour_minus) {
			if ((hour - 1) == -1) NETWORK_OVERRIDE_CLOCK_TIME(23, 0, 0);
			else NETWORK_OVERRIDE_CLOCK_TIME((hour - 1), 0, 0);
			return;
		}
		switch (selectedOption())
		{
		case 2: SET_OVERRIDE_WEATHER((char *)WeatherTypes[weatherTypeIndex]); break;
		}

	}
	void StoredObjects()
	{
		AddTitle("Stored Objects");
		AddObject("p_spinning_anus_s", 0);
		AddObject("prop_Ld_ferris_wheel", 0);
		AddObject("p_ld_soc_ball_01", 0);
		AddObject("prop_weed_01", 0);
		AddObject("p_spinning_anus_s", 0);
		AddObject("prop_weed_pallet", 0);
		AddObject("hei_prop_heist_tug", 0);
		AddObject("prop_juicestand", 0);
		AddObject("prop_dummy_01", 0);
		AddObject("prop_windmill_01", 0);
		AddObject("prop_mp_ramp_03", 0);
		AddObject("prop_air_bigradar", 0);
		AddObject("prop_large_gold", 0);
		AddObject("prop_container_01a", 0);
	}
	void ObjectMenu()
	{
		int vehicle = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
		bool ForgeGun = 0, object_z_plus = 0, object_z_minus = 0, object_y_plus = 0, object_y_minus = 0, object_x_plus = 0, object_x_minus = 0, object_pitch_plus = 0, object_pitch_minus = 0, object_roll_plus = 0, object_roll_minus = 0, object_yaw_plus = 0, object_yaw_minus, snap = 0, freezeObj = 0, attachObjOFF = 0, attachObjON = 0;
		Vector3 enityCoords = GET_ENTITY_COORDS(SpawnedObj, 1), enityRot = GET_ENTITY_ROTATION(SpawnedObj, 2);
		AddTitle("Object Menu");
		AddOption("Stored Objects", null, nullFunc, SUB::STORED_OBJECTS);
		AddKeyboard("Input Object", KB_CUSTOM_OBJECT);
		AddToggle("Show Object Hash",featureObjectHash);
		AddToggle("Get Object Handle By Aim", featureForgeGun, ForgeGun);
		AddBreak("---Edit Last Spawned Object---");
		AddItem("Presision Level", (char**)Precisionlvl, &precisionlvlIndex, 0, 6);
		AddNumber("Z Axis", enityCoords.z, 5, null, object_z_plus, object_z_minus);
		AddNumber("Y Axis", enityCoords.y, 5, null, object_y_plus, object_y_minus);
		AddNumber("X Axis", enityCoords.x, 5, null, object_x_plus, object_x_minus);
		AddNumber("Pitch", enityRot.x, 5, null, object_pitch_plus, object_pitch_minus);
		AddNumber("Roll", enityRot.y, 5, null, object_roll_plus, object_roll_minus);
		AddNumber("Yaw", enityRot.z, 5, null, object_yaw_plus, object_yaw_minus);
		AddToggle("Snap To Ground", featureSnapGround, null, snap);
		AddToggle("Freeze Object", featureFreezeObj, null, freezeObj);
		AddToggle("Attach Object To Vehicle", featureAttachObjVeh, attachObjON, attachObjOFF);
		AddOption("Delete Object", null, DeleteObject);

		switch (selectedOption()) {
		case 2:	StartKeyboard(KB_CUSTOM_OBJECT, "Input Object", 15); DISPLAY_ONSCREEN_KEYBOARD(0, "PM_NAME_CHALL", "", "", "", "", "", 40); keyboardActive = true; break;
		}

		switch (precisionlvlIndex) { case 0: precisionlvlAmount = 1.0; break; case 1: precisionlvlAmount = 2.0; break; case 2: precisionlvlAmount = 5.0; break; case 3: precisionlvlAmount = 0.5; break; case 4: precisionlvlAmount = 0.1; break; case 5: precisionlvlAmount = 0.01; break; case 6: precisionlvlAmount = 0.001; break; }
		if (attachObjON)
		{
			if (IS_PED_IN_VEHICLE)
			{
				ATTACH_ENTITY_TO_ENTITY(SpawnedObj, vehicle, 0, enityCoords.x, enityCoords.y, enityCoords.z, enityRot.x, enityRot.y, enityRot.z, 0, 0, 1, 1, 2, 0);
			}
			else
			{
				ShowSubtitle("~r~Error: You Are Not In A Vehicle");
			}
		}
		else if (attachObjOFF)
		{
			if (IS_PED_IN_VEHICLE)
			{
				DETACH_ENTITY(SpawnedObj, 0, 1);
			}
			else
			{
				ShowSubtitle("~r~Error: You Are Not In A Vehicle");
			}
		}

		if (object_z_plus)
		{
			if (enityCoords.z >= 9000) //max
				enityCoords.z = -9000; //min
			else
				enityCoords.z = enityCoords.z + precisionlvlAmount;//increment

			_SET_ENTITY_COORDS_NO_OFFSET(SpawnedObj, enityCoords.x, enityCoords.y, enityCoords.z, 1, 0, 0);
			return;
		}
		else if (object_z_minus)
		{
			if (enityCoords.z <= -9000)//min
				enityCoords.z = 9000; //max
			else
				enityCoords.z = enityCoords.z - precisionlvlAmount;//decrement
			_SET_ENTITY_COORDS_NO_OFFSET(SpawnedObj, enityCoords.x, enityCoords.y, enityCoords.z, 1, 0, 0);
				return;
		}
		if (object_y_plus)
		{
			if (enityCoords.y >= 9000) //max
				enityCoords.y = -9000; //min
			else
				enityCoords.y = enityCoords.y + precisionlvlAmount;//increment

			_SET_ENTITY_COORDS_NO_OFFSET(SpawnedObj, enityCoords.x, enityCoords.y, enityCoords.z, 1, 0, 0);
			return;
		}
		else if (object_y_minus)
		{
			if (enityCoords.y <= -9000)//min
				enityCoords.y = 9000; //max
			else
				enityCoords.y = enityCoords.y - precisionlvlAmount;//decrement
			_SET_ENTITY_COORDS_NO_OFFSET(SpawnedObj, enityCoords.x, enityCoords.y, enityCoords.z, 1, 0, 0);
			return;
		}
		if (object_x_plus)
		{
			if (enityCoords.x >= 9000) //max
				enityCoords.x = -9000; //min
			else
				enityCoords.x = enityCoords.x + precisionlvlAmount;//increment

			_SET_ENTITY_COORDS_NO_OFFSET(SpawnedObj, enityCoords.x, enityCoords.y, enityCoords.z, 1, 0, 0);
			return;
		}
		else if (object_x_minus)
		{
			if (enityCoords.x <= -9000)//min
				enityCoords.x = 9000; //max
			else
				enityCoords.x = enityCoords.x - precisionlvlAmount;//decrement
			_SET_ENTITY_COORDS_NO_OFFSET(SpawnedObj, enityCoords.x, enityCoords.y, enityCoords.z, 1, 0, 0);
			return;
		}

	
		if (object_pitch_plus)
		{
			if (enityRot.x >= 360) //max
				enityRot.x = -360; //min
			else
				enityRot.x = enityRot.x + precisionlvlAmount;//increment

			SET_ENTITY_ROTATION(SpawnedObj, enityRot.x, enityRot.y, enityRot.z, 2, 1);
			return;
		}
		else if (object_pitch_minus)
		{
			if (enityRot.x <= -360)//min
				enityRot.x = 360; //max
			else
				enityRot.x = enityRot.x - precisionlvlAmount;//decrement
			SET_ENTITY_ROTATION(SpawnedObj, enityRot.x, enityRot.y, enityRot.z, 2, 1);
			return;
		}

		if (object_roll_plus)
		{
			if (enityRot.y >= 360) //max
				enityRot.y = -360; //min
			else
				enityRot.y = enityRot.y + precisionlvlAmount;//increment

			SET_ENTITY_ROTATION(SpawnedObj, enityRot.x, enityRot.y, enityRot.z, 2, 1);
			return;
		}
		else if (object_roll_minus)
		{
			if (enityRot.y <= -360)//min
				enityRot.y = 360; //max
			else
				enityRot.y = enityRot.y - precisionlvlAmount;//decrement
			SET_ENTITY_ROTATION(SpawnedObj, enityRot.x, enityRot.y, enityRot.z, 2, 1);
			return;
		}
		if (object_yaw_plus)
		{
			if (enityRot.z >= 360) //max
				enityRot.z = -360; //min
			else
				enityRot.z = enityRot.z + precisionlvlAmount;//increment

			SET_ENTITY_ROTATION(SpawnedObj, enityRot.x, enityRot.y, enityRot.z, 2, 1);
			return;
		}
		else if (object_yaw_minus)
		{
			if (enityRot.z <= -360)//min
				enityRot.z = 360; //max
			else
				enityRot.z = enityRot.z - precisionlvlAmount;//decrement
			SET_ENTITY_ROTATION(SpawnedObj, enityRot.x, enityRot.y, enityRot.z, 2, 1);
			return;
		}

		
				
	}
	void ModelOptions()
	{
		bool model;
		AddTitle("Model Options");
		AddItem("Stored Models", (char **)ModelName, &modelTypeIndex, 0, 13);
		AddKeyboard("Custom Model Input", KB_SKINCHANGER);
		switch (selectedOption()){
		case 1: featureSkinChanger = 1; break;

		}
	} 
	void WeaponOptions()
	{
		AddTitle("Weapon Options");
		AddOption("Give All Weapons", null, GiveAllWeaponsSelf);
		AddOption("Remove All Weapons", null ,TakeAllWeapons);
		AddOption("Projectile Menu", null, nullFunc, SUB::POJECTILEMENU);

	}
	void Projectile()
	{
		bool MoneygunON = 0, Weedgun = 0, Cargun = 0, Teleportgun = 0, Deletegun = 0, Hydrantgun = 0, gravity_gun_on = 0, expBull = 0;
		AddTitle("Projectile Menu");
		AddToggle("$40k Gun", featureCashGun, null, MoneygunON);
		AddToggle("Hydrant Gun", null, null, Hydrantgun);
		AddToggle("Car Gun", null, null, Cargun);
		AddToggle("Explosive Bullets", featureExplosiveBullets, null, expBull);
		AddToggle("Teleport Gun", featureTeleportGun, null, Teleportgun);
		AddToggle("Delete Entity Gun", featureDeleteGun, null, Deletegun);
		AddToggle("Gravity Gun", featureGravityGun, gravity_gun_on);
		AddBreak("---Prop Gun Options---");
		AddItem("Prop Gun Object", (char **)GunObjectName, &propObjectTypeIndex, 0, 1);
		AddToggle("Enable Prop Gun", featurePropGun);

		if (gravity_gun_on) {
			ShowSubtitle("Use ~r~Pistol~s~ for Gravity Gun!");
			return;
		}
	}
	void NetworkPlayers()
	{
		// Initialise local variables here:
		bool onPress = 0; featurePlayerInfo = 1; playerinfoSelected = 0;
		// Options' text here:
		AddTitle("Online Players");
		for (int i = 0; i < 18; i++)
		{
			char *playerName = _GET_PLAYER_NAME(i);
			if (PLAYER_ID() == i)
				AddOption(playerName, onPress, nullFunc, -1, "~r~[YOU]");
			else if (NETWORK_GET_HOST_OF_SCRIPT("freemode", -1, 0) == i)
				AddOption(playerName, onPress, nullFunc, -1, "~r~[HOST]");
			else if (IsPlayerFriend(i))
				AddOption(playerName, onPress, nullFunc, -1, "~r~[FRIEND]");
			else
				AddOption(playerName, onPress);
		}
		// Options' code here:
		if (onPress && NETWORK_IS_PLAYER_ACTIVE((Player)menu::currentop - 1))
		{
			selectedPlayer = (Player)menu::currentop - 1;
			if (SUB::NETWORK_PLAYERS_OPTIONS != -1)	menu::SetSub_delayed = SUB::NETWORK_PLAYERS_OPTIONS; 
			return; // Either use return; to exit to the switch if you don't have code below that you want executed.
		}

	}
	void OnlinePlayersOptions()
	{
		// Initialise local variables here:
		playerinfoSelected = 1;
		bool networkKickPlayer = 0;

		// Options' text here:
		AddTitle(_GET_PLAYER_NAME(selectedPlayer));
		
		AddOption("Main Options",null, nullFunc, SUB::NETWORK_PLAYER_OPTIONS);
		AddOption("Weapon Options", null, nullFunc, SUB::NETWORK_WEAPON_OPTIONS);
		AddOption("Vehicle Options", null, nullFunc, SUB::NETWORK_VEHICLE_OPTIONS);
		AddOption("Teleportation Options", null, nullFunc, SUB::NETWORK_TELEPORT_OPTIONS);
		AddOption("Animation Options", null, nullFunc, SUB::NETWORK_ANIMATION_OPTIONS);
		AddOption("Naughty Options", null, nullFunc, SUB::NETWORK_NAUGHTY_OPTIONS);
		AddOption("Attachment Options", null, nullFunc, SUB::NETWORK_ATTACH_OPTIONS);
		AddOption("~r~Advance Options", null, nullFunc, SUB::ADVANCE_OPTIONS);
		AddOption("Miscellaneous", null, nullFunc, SUB::NETWORK_MISC);
	}
	void NetworkPlayerOptions()
	{
		// Initialise local variables here:
		bool moneyDrop = 0, propMoneyDrop = 0;
		// Options' text here:
		AddTitle("Main Options" /*_GET_PLAYER_NAME(selectedPlayer) */); //use Player Options as tittle or player name

		AddOption("Semi God Mode", null, SemiGMode);
		AddOption("Clone Player");
		AddOption("Increase Wanted Level", individualPlayer[selectedPlayer].featurewantedlevel);
		AddOption("Steal Players Outfit", null, StealOutfit);
		AddToggle("Enable Red Boxes", individualPlayer[selectedPlayer].featureRedBoxes);
		AddBreak("---Money Drop Options---");
		AddItem("Money Drop Prop", (char**)MoneyPropName, &moneyPropTypeIndex, 0, 2);
		AddToggle("Enable Prop Money Drop", individualPlayer[selectedPlayer].featurePropMoneyDrop, propMoneyDrop);
		AddToggle("Enable $40k Drop", individualPlayer[selectedPlayer].featureMoneyDrop, moneyDrop);
		switch (selectedOption())
		{
		case 2: CLONE_PED(GET_PLAYER_PED(selectedPlayer), 0, 0, 0); break;
		}
		
	}
	void NetworkVehicleOptions()
	{
		// Initialise local variables here:
		bool repair = 0;
		Ped networkPlayer = GET_PLAYER_PED(selectedPlayer); Vehicle playersVehicle = GET_VEHICLE_PED_IS_IN(GET_PLAYER_PED(selectedPlayer), 0); //if late param is 1 = last vehicle
		Vector3 clientCoord = GET_ENTITY_COORDS(networkPlayer, 1);
		AddTitle("Vehicle Options");

		AddOption("Repair Vehicle");
		AddOption("Delete Vehicle");
		AddOption("Hijack Vehicle");
		AddOption("Kick From Vehicle");
		AddOption("Sling Shot Players Vehicle");
		AddBreak("----Tow Truck----");
		AddOption("RC Vehicle");
		AddOption("Detach RC Vehicle");
		switch (selectedOption())
		{
		case 1: if (IS_PED_IN_ANY_VEHICLE(networkPlayer, 0)) RequestControlOfEntM(playersVehicle, RC_FIXNETWORKVEHICLE); else ShowSubtitle("~r~Error: Player not in vehicle"); break;
		case 2: if (IS_PED_IN_ANY_VEHICLE(networkPlayer, 0)) RequestControlOfEntM(playersVehicle, RC_DELETENETWORKVEHICLE); else ShowSubtitle("~r~Error: Player not in vehicle"); break;
		case 3: CLEAR_PED_TASKS_IMMEDIATELY(networkPlayer);	_SET_ENTITY_COORDS_NO_OFFSET(PLAYER_PED_ID(), clientCoord.x, clientCoord.y, clientCoord.z, 0, 0, 1); if (IS_PED_IN_ANY_VEHICLE(networkPlayer, 0)) SET_PED_INTO_VEHICLE(PLAYER_PED_ID(), playersVehicle, SEAT_PASSENGER); break;
		case 4: CLEAR_PED_TASKS_IMMEDIATELY(networkPlayer); break;
		case 5: if (IS_PED_IN_ANY_VEHICLE(networkPlayer, 0)) RequestControlOfEntM(playersVehicle, RC_SLINGSHOTNETWORKVEHICLE); else ShowSubtitle("~r~Error: Player not in vehicle"); break;
		case 7: if (IS_PED_IN_ANY_VEHICLE(networkPlayer, 0)) RequestControlOfEntM(playersVehicle, RC_REMOTCONTROLNETWORKVEHICLE); else ShowSubtitle("~r~Error: Player not in vehicle"); break;
		case 8: if (IS_PED_IN_ANY_VEHICLE(networkPlayer, 0)) RequestControlOfEntM(playersVehicle, RC_DETACHNETWORKVEHICLE); else ShowSubtitle("~r~Error: Player not in vehicle"); break;
		}
	}
	void NetworkTeleportOptions()
	{
		// Initialise local variables here:
		bool teleportIntoSeat = 0;
		Entity ent = PLAYER_PED_ID(); 
		if (IS_PED_IN_ANY_VEHICLE(ent, 0))
			ent = GET_VEHICLE_PED_IS_IN(ent, 1);
		Vector3 clientCoord = GET_ENTITY_COORDS(GET_PLAYER_PED(selectedPlayer), 1);
		Vector3 myCoords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PLAYER_PED_ID(), 0.0, 1.5, 0.0);


		// Options' text here:
		AddTitle("Teleport Options");

		AddTeleport("Self To Player", ent, clientCoord.x, clientCoord.y, clientCoord.z, 0);
		AddTeleport("Self Into Their Vehicle", ent, clientCoord.x, clientCoord.y, clientCoord.z, 2);
		AddOption("Telport Player To Self", null, TPPlayerToSelf);

		switch (selectedOption())
		{
		case 2: 
			SET_ENTITY_COORDS_NO_OFFSET(PLAYER_PED_ID(), clientCoord.x, clientCoord.y, clientCoord.z, 0, 0, 1);
			if (IS_PED_IN_ANY_VEHICLE(GET_PLAYER_PED(selectedPlayer), 0))
				SET_PED_INTO_VEHICLE(PLAYER_PED_ID(), GET_VEHICLE_PED_IS_IN(GET_PLAYER_PED(selectedPlayer), 1), SEAT_RANDOM);
			else
				ShowSubtitle("~r~Error: Player not in vehicle");
			break;
		}


	}
	void NetworkAttachOptions()
	{
		// Initialise local variables here:
		bool attachtoPlayer = 0, attachtoVehicle = 0, attachtoVehicleON = 0, attachtoVehicleOFF = 0;
		Entity myEnt = PLAYER_PED_ID();
		if (IS_PED_IN_ANY_VEHICLE(myEnt, 0)) { myEnt = GET_VEHICLE_PED_IS_USING(myEnt); }
		Entity hisEnt = GET_PLAYER_PED(selectedPlayer);
		if (IS_PED_IN_ANY_VEHICLE(hisEnt, 0)) { hisEnt = GET_VEHICLE_PED_IS_USING(hisEnt); }

		// Options' text here:
		AddTitle("Attachment Options");

		AddLocal("Attach Self To Player", IS_ENTITY_ATTACHED(myEnt), null, null);
		AddToggle("Attach Self To Vehicle", featureAttachToVehicle, attachtoVehicleON, attachtoVehicleOFF);
		AddOption("Detach Last Objects");
		AddOption("Alien Egg");
		AddOption("Cone Head");
		AddOption("Hotdog Penis");
		AddOption("UFO Room");
		AddOption("Katana");
		AddOption("Snowman");
		AddOption("Neon Sign");

		switch (selectedOption())
		{
		case 1:
			if (!IS_ENTITY_ATTACHED(myEnt))
			{
				if ((hisEnt != myEnt)) //so you dont attach to self and freeze
					ATTACH_ENTITY_TO_ENTITY(myEnt, hisEnt, -1, 0, -0.35, 0, 0, 0, 0, 1, 1, 0, 1, 2, 1);
				SET_ENTITY_NO_COLLISION_ENTITY(hisEnt, myEnt, TRUE);
			}
			else
			{
				DETACH_ENTITY(myEnt, 1, 1);
			} break;
		case 3: NETWORK_REQUEST_CONTROL_OF_ENTITY(curObject); if (NETWORK_HAS_CONTROL_OF_ENTITY(curObject)) { DETACH_ENTITY(curObject, 1, 1); }break;
		case 4: CREATE_n_ATTACH(GET_HASH_KEY("prop_alien_egg_01"), GET_PLAYER_PED(selectedPlayer), SKEL_Head, 0.2, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 0, 0, 0); break; //egg
		case 5: CREATE_n_ATTACH(GET_HASH_KEY("prop_mp_cone_01"), GET_PLAYER_PED(selectedPlayer), SKEL_Head, 0.2, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 0, 0, 0); break; //cone head
		case 6: CREATE_n_ATTACH(GET_HASH_KEY("prop_cs_hotdog_01"), GET_PLAYER_PED(selectedPlayer), SKEL_Pelvis, 0.4, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 0, 0, 0); break; //hotdog
		case 7: CREATE_n_ATTACH(GET_HASH_KEY("p_spinning_anus_s"), GET_PLAYER_PED(selectedPlayer), SKEL_Spine_Root, 0.2, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 0, 0, 0); break; //UFO
		case 8: CREATE_n_ATTACH(GET_HASH_KEY("prop_cs_katana_01"), GET_PLAYER_PED(selectedPlayer), SKEL_R_Hand, 0.2, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 0, 0, 0); break; //Katana
		case 9: CREATE_n_ATTACH(GET_HASH_KEY("prop_prlg_snowpile"), GET_PLAYER_PED(selectedPlayer), SKEL_Head, 0.2, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 0, 0, 0); break; //snowman
		case 10: CREATE_n_ATTACH(GET_HASH_KEY("prop_beer_neon_01"), GET_PLAYER_PED(selectedPlayer), SKEL_Spine_Root, 0.0, 0.0, 0.0, 0.0f, 90.0f, 0.50f, 0, 0, 0); break; //neon sign
		}

		if (attachtoVehicleON)
		{
			if (!IS_ENTITY_ATTACHED(myEnt))
			{ 
				int vehid = GET_VEHICLE_PED_IS_IN(GET_PLAYER_PED(selectedPlayer), 0);
				if ((hisEnt != myEnt))
				ATTACH_ENTITY_TO_ENTITY(myEnt, vehid, -1, 0.0f, 0.001f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 2, 1);
				SET_ENTITY_COLLISION(myEnt, 0, 0);
			}
		}
		else if (attachtoVehicleOFF)
		{
			int player = GET_PLAYER_PED(selectedPlayer);
			int vehid = GET_VEHICLE_PED_IS_IN(GET_PLAYER_PED(selectedPlayer), 0);
			DETACH_ENTITY(myEnt, player, 1);
		}

	}
	void Misc()
	{
		bool noIdleON = 0, noIdleOFF = 0;
		AddTitle("Miscellaneous");
		AddToggle("Modder Protection", featureModderProtection);
		AddToggle("Display FPS", featureDisplayFPS);
		AddOption("Clear Area",null, clearArea);
		AddOption("Particle FX Options", null, nullFunc, SUB::PARTICLEFX);
		AddBreak("---Tunables---");
		AddItem("Fake Bounty", (char **)BountyType, &bountyTypeIndex, 0, 3);
		AddToggle("No Idle Kick", null, noIdleON, noIdleOFF);
		switch (selectedOption())
		{
		case 7: SetTunable(4872, 0 ,0); SetTunable(2312, (int)BountyType[bountyTypeIndex], 0); SetTunable(2313, (int)BountyType[bountyTypeIndex], 0); SetTunable(2314, (int)BountyType[bountyTypeIndex], 0); SetTunable(2315, (int)BountyType[bountyTypeIndex], 0);  SetTunable(2316, (int)BountyType[bountyTypeIndex], 0); break;
		}
		if (noIdleON)
		{
			SetTunable(73, 2147483647, 0); SetTunable(74, 2147483647, 0); SetTunable(75, 2147483647, 0); SetTunable(76, 2147483647, 0);
		}
		else if (noIdleOFF)
		{
			SetTunable(73, 120000, 0); SetTunable(74, 120000, 0); SetTunable(75, 120000, 0); SetTunable(76, 120000, 0);
		}
	}
	void Recovery()
	{
		bool  settings_cash_input = 0, settings_cash_plus = 0, settings_cash_minus = 0, santa = 0, dunce = 0, police = 0;
		AddTitle("Recovery Menu");
		AddItem("Rank", (char **)LevelNames, &levelTypeIndex, 0, 998);
		AddNumber("Add $ to Bank", settings_cash, 0, settings_cash_input, settings_cash_plus, settings_cash_minus);
		AddOption("Unlock All LSC", null, LSC);
		AddOption("Unlock All Awards");
		AddOption("Unlock All Heist", null, Heists);
		AddOption("100% Skills", null, skills);
		AddOption("Buy All Clothing", null, UnlockAllCharacter1);
		AddBreak("---Outfits---");
		AddOption("Outfit Creator", null, nullFunc, SUB::OUTFITDESIGNER);
		AddOption("Santa Outfit", santa);
		AddOption("Cop Outfit", police);
		AddOption("Dunce Cap", dunce);
		switch (selectedOption())
		{
		case 1: _STAT_SET_INT("CHAR_XP_FM", (int)Levels[levelTypeIndex], 1);  break;
		}

		if (santa)
		{
			ResetAppearance();
			changeAppearance("MASK", 8, 0);
			changeAppearance("TORSO", 12, 0);
			changeAppearance("LEGS", 19, 0);
			changeAppearance("SHOES", 4, 4);
			changeAppearance("SPECIAL1", 10, 0);
			changeAppearance("SPECIAL2", 21, 2);
			changeAppearance("TORSO2", 19, 0);
			return;
		}
		if (police)
		{
			ResetAppearance();
			changeAppearance("TORSO", 0, 0);
			changeAppearance("GLASSES", 6, 1);
			changeAppearance("LEGS", 35, 0);
			changeAppearance("SHOES", 25, 0);
			changeAppearance("SPECIAL2", 58, 0);
			changeAppearance("TORSO2", 55, 0);
			return;
		}
		if (dunce)
		{
			ResetAppearance();
			changeAppearance("HATS", 2, 0);
		}

		if (settings_cash_input) {
			StartKeyboard(KB_SETTINGS_CASH, "", 30);
			return;
		}

		if (settings_cash_plus) {
			if (settings_cash < 10000000000) (settings_cash)++;
			else settings_cash = 0;
			NETWORK_EARN_FROM_ROCKSTAR(settings_cash);
			return;
		}
		else if (settings_cash_minus) {
			if (settings_cash > 0) (settings_cash)--;
			else settings_cash = 10000000000;
			NETWORK_EARN_FROM_ROCKSTAR(settings_cash);
			return;
		}
	}
	void MainNetwork()
	{
		featurePlayerInfo = 0;
		bool showtalkOff = 0, onPress = 0;
		AddTitle("Network");
		AddToggle("Show Talking Players", featureTalkingPlayers, null, showtalkOff);
		AddOption("Online Name Editor", null, nullFunc, SUB::NAME_EDITOR);
		AddOption("Online Players", null, nullFunc, SUB::NETWORK_PLAYERS);
		AddOption("All Online Players", onPress, nullFunc, SUB::ALL_NETWORK);

		if (onPress)
		{
			if (SUB::ALL_NETWORK != -1)	menu::SetSub_delayed = SUB::ALL_NETWORK; featureESPAll = 1; return;
		}
		else
		{
			featureESPAll = 0; return;
		}
	}
	void All()
	{
		int i = 0; i < 16; i++;
		Vehicle playersVehicle = GET_VEHICLE_PED_IS_IN(GET_PLAYER_PED(i), 0);
		bool explode = 0, weaponsall = 0, cashDrop = 0;
		AddTitle("All Online Players");
		AddToggle("$40k Cash Drop", featureCashAll, null, cashDrop);
		AddOption("Explode All Players", explode);
		AddOption("Paint All Cars Pink");
		AddOption("Give All Players Weapons", weaponsall);
		AddOption("Teleport All To Me");
		AddOption("Animation Options",null, nullFunc, SUB::NETWORK_ALL_ANIM);
		switch (selectedOption())
		{
		case 2: RequestControlOfEntM(playersVehicle, RC_PAINTALLPINK); break;
		case 5: TPAllToSelf(); break;
		}
		if (explode)
		{
			ExplodeAllPlayers();
			ShowSubtitle("~r~All Players Exploded");
		}
		if (weaponsall)
		{
			GiveAllWeaponsEveryone();
			ShowSubtitle("~r~ Weapons Given To All Players");
		}	
	}
	void Naughty()
	{
		bool clearTask = 0, hydrate = 0, rapeOff = 0, explode = 0, rapeOn = 0;
		Entity myEnt = PLAYER_PED_ID();
		if (IS_PED_IN_ANY_VEHICLE(myEnt, 0)) { myEnt = GET_VEHICLE_PED_IS_USING(myEnt); }
		Entity hisEnt = GET_PLAYER_PED(selectedPlayer);
		if (IS_PED_IN_ANY_VEHICLE(hisEnt, 0)) { hisEnt = GET_VEHICLE_PED_IS_USING(hisEnt); }

		AddTitle("Naughty Options");
		AddToggle("Explode Player", individualPlayer[selectedPlayer].featureExplodePlayer, hydrate);
		AddToggle("Rape Player", featureRapePlayer, rapeOn, rapeOff);
		AddOption("Make Player Explode Lobby", null, PlayerExplodeLobby);
		AddToggle("Freeze Player", individualPlayer[selectedPlayer].featureStopTask, clearTask);
		AddToggle("Hydrate Player", individualPlayer[selectedPlayer].featureHydratePlayer, hydrate);
		AddOption("Trap In Cage");
		AddOption("Spawn Attack Jet", null, JetAttack);


		//option codes
		switch (selectedOption())
		{
		case 6: ObjectForOnlinePlayer(959275690); break;
		}
		

		if (rapeOn)
		{
			if (!IS_ENTITY_ATTACHED(myEnt))
			{
				if ((hisEnt != myEnt))
					ATTACH_ENTITY_TO_ENTITY(myEnt, hisEnt, -1, 0, -0.35, 0, 0, 0, 0, 1, 1, 0, 1, 2, 1);
				SET_ENTITY_NO_COLLISION_ENTITY(hisEnt, myEnt, TRUE);
				TaskPlayAnimationM(PLAYER_PED_ID(), "rcmpaparazzo_2", "shag_loop_a");
				CLEAR_PED_TASKS_IMMEDIATELY(GET_PLAYER_PED(selectedPlayer));
				Vector3 pedcoords = GET_ENTITY_COORDS(GET_PLAYER_PED(selectedPlayer), 1);
				int netscene = NETWORK_CREATE_SYNCHRONISED_SCENE(pedcoords.x, pedcoords.y, pedcoords.z, 0.0, 0.0, 0.0, 2, 0, 0, 0x3f800000, 0, 0x3f800000);
				NETWORK_ADD_PED_TO_SYNCHRONISED_SCENE(GET_PLAYER_PED(selectedPlayer), netscene, "rcmpaparazzo_2", "shag_loop_poppy", 8.0f, 0.0f, 269, 9, 0);
				NETWORK_START_SYNCHRONISED_SCENE(netscene);
			}
		}
		else if (rapeOff)
		{
			DETACH_ENTITY(myEnt, 1, 1);
			CLEAR_PED_TASKS_IMMEDIATELY(PLAYER_PED_ID());
			CLEAR_PED_TASKS_IMMEDIATELY(GET_PLAYER_PED(selectedPlayer));
			//NETWORK_STOP_SYNCHRONISED_SCENE(netscene);
		}




	}
	void NetworkWeapon()
	{
		bool expBull = 0;
		AddTitle("Weapon Options");
		AddOption("Give All Weapons");
		AddOption("Take All Weapons");
		AddToggle("Explosive Bullets", individualPlayer[selectedPlayer].featureNetworkExp, null, expBull);
		switch (selectedOption())
		{
		case 1: GiveAllaponsClients(selectedPlayer); break;
		case 2: TakeAllWeapons(selectedPlayer); break;
		}
	}
	void VehicleWeapons()
	{
		bool weaponsON = 0, weaponsOFF = 0;
		AddTitle("Vehicle Weapons");
		AddToggle("Enable Weapons", featureVehicleWeapons, weaponsON, weaponsOFF);
		AddItem("Weapon", (char **)WeaponAssetName, &vehicleWeaponIndex, 0, 6);
		if (weaponsON)
		{
			featureWeaponLaser = 1;
		}
		else if (weaponsOFF)
		{
			featureWeaponLaser = 0;
		}
	}
	void OutfitDesigner()
	{
		float type, texture;
		bool type_plus = 0, type_minus = 0, tex_plus = 0, tex_minus = 0;
		AddTitle("Outfit Creator");
		AddItem("Component:", (char **)ComponentType, &ComponentTypeIndex, 0, 14);
		AddNumber("Type:", type, 0, null, type_plus, type_minus);
		AddNumber("Texture:", texture, 0, null, tex_plus, tex_minus);
		AddOption("Set Outfit");
		if (type_plus) {
			if (type < 60) (type)++;
			else type = 0;
			ResetAppearance(); changeAppearance((char*)ComponentType[ComponentTypeIndex], type, texture);
			return;
		}
		else if (type_minus) {
			if (type > 0) (type)--;
			else type = 60;
			ResetAppearance(); changeAppearance((char*)ComponentType[ComponentTypeIndex], type, texture);
			return;
		}
		if (tex_plus) {
			if (texture < 60) (type)++;
			else texture = 0;
			ResetAppearance(); changeAppearance((char*)ComponentType[ComponentTypeIndex], type, texture);
			return;
		}
		else if (tex_minus) {
			if (texture > 0) (type)--;
			else texture = 60;
			ResetAppearance(); changeAppearance((char*)ComponentType[ComponentTypeIndex], type, texture);
			return;
		}
	}
	void ParticleFX()
	{
		AddTitle("Particle FX Options");
		AddOption("Particle FX");
		AddOption("FX Speed");
		AddOption("Test", null, TEST);
		AddToggle("Ghost Rider", featureGhostRider);
	}
	void VehicleEditor()
	{
		bool turboON = 0, xenonON = 0, UNK17ON = 0, UNK19ON = 0, UNK21ON = 0;
		int veh = GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID());
		AddTitle("Vehicle Editor");
		AddOption("Color Editor", null, nullFunc, SUB::VEHICLE_PAINT);
		AddItem("Spoilers", (char **)SpoilerTypes, &spoilerTypeIndex, 0, 3);
		AddItem("Front Bumpers", (char**)BumberTypes, &frontBumberTypeIndex, 0, 5);
		AddItem("Rear Bumpers", (char**)BumberTypes, &rearBumberTypeIndex, 0, 5);
		AddItem("Side Skirt", (char**)SideSkirtTypes, &sideSkirtTypeIndex, 0, 2);
		AddItem("Exhaust", (char**)ExhaustTypes, &exhaustTypeIndex, 0, 1);
		AddItem("Frame", (char**)FrameTypes, &frameTypeIndex, 0, 1);
		AddItem("Grille", (char**)GrilleTypes, &grilleTypeIndex, 0, 3);
		AddItem("Hood", (char**)HoodTypes, &hoodTypeIndex, 0, 3);
		AddItem("Fender", (char**)FenderTypes, &fenderTypeIndex, 0, 1);
		AddItem("Right Fender", (char**)RightFenderTypes, &rightFenderTypeIndex, 0, 1);
		AddItem("Roof", (char**)RoofTypes, &roofTypeIndex, 0, 1);
		AddItem("Window Tent", (char**)WindowTypes, &windowtentTypeIndex, 0, 4);
		AddItem("Engine", (char**)EngineTypes, &engineTypeIndex, 0, 3);
		AddItem("Brakes", (char**)BrakeTypes, &breakTypeIndex, 0, 3);
		AddItem("Transmission", (char**)TransmissionTypes, &transmissionTypeIndex, 0, 3);
		AddItem("Horns", (char**)HornTypes, &hornsTypeIndex, 0, 53);
		AddItem("Suspension", (char**)SuspensionTypes, &SuspensionTypeIndex, 0, 3);
		AddItem("Armor", (char**)ArmorTypes, &armorTypeIndex, 0, 4);
		AddToggle("Turbo", featureTurbo, turboON);
		AddToggle("Xenon Headlight", featureXenon, xenonON);
		AddToggle("UNK17", featureUNK17, UNK17ON);
		AddToggle("UNK19", featureUNK19, UNK19ON);
		AddToggle("UNK21", featureUNK21, UNK21ON);
		switch (selectedOption())
		{
		case 2: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 0, (int)SpoilerTypes[spoilerTypeIndex], 0);
		case 3: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 1, (int)BumberTypes[frontBumberTypeIndex], 0);
		case 4: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 2, (int)BumberTypes[rearBumberTypeIndex], 0);
		case 5: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 3, (int)SideSkirtTypes[sideSkirtTypeIndex], 0);
		case 6: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 4, (int)ExhaustTypes[exhaustTypeIndex], 0);
		case 7: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 5, (int)FrameTypes[frameTypeIndex], 0);
		case 8:  SET_VEHICLE_MOD(veh, 6, (int)GrilleTypes[grilleTypeIndex], 0);
		case 9: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 7, (int)HoodTypes[hoodTypeIndex], 0);
		case 10: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 8, (int)FenderTypes[fenderTypeIndex], 0);
		case 11: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 9, (int)RightFenderTypes[rightFenderTypeIndex], 0);
		case 12: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 10, (int)RoofTypes[roofTypeIndex], 0);
		case 13: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_WINDOW_TINT(veh, (int)WindowTypes[windowtentTypeIndex]);
		case 14: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 11, (int)EngineTypes[engineTypeIndex], 0);
		case 15: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 12, (int)BrakeTypes[breakTypeIndex], 0);
		case 16: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 13, (int)TransmissionTypes[transmissionTypeIndex], 0);
		case 17: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 14, (int)HornTypes[hornsTypeIndex], 0);
		case 18: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 15, (int)SuspensionTypes[SuspensionTypeIndex], 0);
		case 19: SET_VEHICLE_MOD_KIT(veh, 0); SET_VEHICLE_MOD(veh, 16, (int)ArmorTypes[armorTypeIndex], 0);
		}
		if (turboON){ SET_VEHICLE_MOD_KIT(veh, 0); TOGGLE_VEHICLE_MOD(veh, 18, 1); }
		if (xenonON){ SET_VEHICLE_MOD_KIT(veh, 0); TOGGLE_VEHICLE_MOD(veh, 22, 1); }
		if (UNK17ON){ SET_VEHICLE_MOD_KIT(veh, 0); TOGGLE_VEHICLE_MOD(veh, 17, 1); }
		if (UNK19ON){ SET_VEHICLE_MOD_KIT(veh, 0); TOGGLE_VEHICLE_MOD(veh, 19, 1); }
		if (UNK21ON){ SET_VEHICLE_MOD_KIT(veh, 0); TOGGLE_VEHICLE_MOD(veh, 21, 1); }
	}
	void VehicleColor()
	{
		AddTitle("Color Editor");
		AddOption("Primary Color", null, nullFunc, SUB::VEHICLE_PRIMARY);
		AddOption("Secondary Color", null, nullFunc, SUB::VEHICLE_SECONDARY);
		AddOption("Set Chrome Color");
		switch (selectedOption())
		{
		case 3:	SET_VEHICLE_COLOURS(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), 120, 120);
		}

	}
	void VehiclePrimary()
	{
		bool  settings_primaryR_input = 0, settings_primaryR_plus = 0, settings_primaryR_minus = 0, settings_primaryG_input = 0, settings_primaryG_plus = 0, settings_primaryG_minus = 0, settings_primaryB_input = 0, settings_primaryB_plus = 0, settings_primaryB_minus = 0;
		AddTitle("Primary Color");
		AddNumber("Red", primaryR, 0, settings_primaryR_input, settings_primaryR_plus, settings_primaryR_minus);
		AddNumber("Green", primaryG, 0, settings_primaryG_input, settings_primaryG_plus, settings_primaryG_minus);
		AddNumber("Blue", primaryB, 0, settings_primaryB_input, settings_primaryB_plus, settings_primaryB_minus);
		if (settings_primaryR_input) {
			StartKeyboard(KB_DEFAULT, "", 30);
			return;
		}

		if (settings_primaryR_plus) {
			if (primaryR < 255) (primaryR)++;
			else primaryR = 0;
			SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), primaryR, primaryG, primaryB);
			return;
		}
		else if (settings_primaryR_minus) {
			if (primaryR > 0) (primaryR)--;
			else primaryR = 255;
			SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), primaryR, primaryG, primaryB);
			return;
		}
		if (settings_primaryG_input) {
			StartKeyboard(KB_DEFAULT, "", 30);
			return;
		}

		if (settings_primaryG_plus) {
			if (primaryG < 255) (primaryG)++;
			else primaryG = 0;
			SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), primaryR, primaryG, primaryB);
			return;
		}
		else if (settings_primaryG_minus) {
			if (primaryG > 0) (primaryG)--;
			else primaryG = 255;
			SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), primaryR, primaryG, primaryB);
			return;
		}
		if (settings_primaryB_input) {
			StartKeyboard(KB_DEFAULT, "", 30);
			return;
		}

		if (settings_primaryB_plus) {
			if (primaryB < 255) (primaryB)++;
			else primaryB = 0;
			SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), primaryR, primaryG, primaryB);
			return;
		}
		else if (settings_primaryB_minus) {
			if (primaryB > 0) (primaryB)--;
			else primaryB = 255;
			SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), primaryR, primaryG, primaryB);
			return;
		}
	}
	void VehicleSecondary()
	{
		bool  settings_secondaryR_input = 0, settings_secondaryR_plus = 0, settings_secondaryR_minus = 0, settings_secondaryG_input = 0, settings_secondaryG_plus = 0, settings_secondaryG_minus = 0, settings_secondaryB_input = 0, settings_secondaryB_plus = 0, settings_secondaryB_minus = 0;
		AddTitle("Primary Color");
		AddNumber("Red", secondaryR, 0, settings_secondaryR_input, settings_secondaryR_plus, settings_secondaryR_minus);
		AddNumber("Green", secondaryG, 0, settings_secondaryG_input, settings_secondaryG_plus, settings_secondaryG_minus);
		AddNumber("Blue", secondaryB, 0, settings_secondaryB_input, settings_secondaryB_plus, settings_secondaryB_minus);
		if (settings_secondaryR_input) {
			StartKeyboard(KB_DEFAULT, "", 30);
			return;
		}

		if (settings_secondaryR_plus) {
			if (secondaryR < 255) (secondaryR)++;
			else secondaryR = 0;
			SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), secondaryR, secondaryG, secondaryB);
			return;
		}
		else if (settings_secondaryR_minus) {
			if (secondaryR > 0) (secondaryR)--;
			else secondaryR = 255;
			SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), secondaryR, secondaryG, secondaryB);
			return;
		}
		if (settings_secondaryG_input) {
			StartKeyboard(KB_DEFAULT, "", 30);
			return;
		}

		if (settings_secondaryG_plus) {
			if (secondaryG < 255) (secondaryG)++;
			else secondaryG = 0;
			SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), secondaryR, secondaryG, secondaryB);
			return;
		}
		else if (settings_secondaryG_minus) {
			if (secondaryG > 0) (secondaryG)--;
			else secondaryG = 255;
			SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), secondaryR, secondaryG, secondaryB);
			return;
		}
		if (settings_secondaryB_input) {
			StartKeyboard(KB_DEFAULT, "", 30);
			return;
		}

		if (settings_secondaryB_plus) {
			if (secondaryB < 255) (secondaryB)++;
			else secondaryB = 0;
			SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), secondaryR, secondaryG, secondaryB);
			return;
		}
		else if (settings_secondaryB_minus) {
			if (secondaryB > 0) (secondaryB)--;
			else secondaryB = 255;
			SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID()), secondaryR, secondaryG, secondaryB);
			return;
		}
	}
	void PedOptions()
	{
		AddTitle("Ped Options");
		AddOption("Stored Peds");
		AddOption("Input Peds");//keyboard
		AddOption("Traffic Manager");
		AddOption("Ridable Animals");
	}
	void AdvanceOptions()
	{
		AddTitle("~r~Advance Options");
		AddOption("Initiate Console Freeze", null, InitiateFreeze);
	}
	void NetworkAnimations()
	{
		AddTitle("Animation Options");
		AddOption("Stop Animations");
		AddItem("Animal Animations", (char **)AnimalAnime, &animalAnimTypeIndex, 0, 4);
		AddItem("Dancing Animations", (char **)DanceAnime, &danceAnimTypeIndex, 0, 4);
		AddItem("Misc Animations", (char **)MiscAnime, &miscAnimTypeIndex, 0, 4);
		AddItem("Naughty Animations", (char **)NaughtyAnime, &naughtyAnimTypeIndex, 0, 4);
		AddItem("Sitting Animations", (char **)SittingAnime, &sittingAnimTypeIndex, 0, 4);
		AddItem("Sports Animations", (char **)SportsAnime, &sportAnimTypeIndex, 0, 4);

		switch (selectedOption())
		{
		case 1: CLEAR_PED_TASKS_IMMEDIATELY(GET_PLAYER_PED(selectedPlayer)); break;
		case 2: featureAnimaAction = 1; TaskPlayAnimationM(GET_PLAYER_PED(selectedPlayer), (char *)AnimalAnimeType[animalAnimTypeIndex], (char *)AnimalAnimeID[animalAnimTypeIndex]); break;
		case 3: featureAnimaAction = 1; TaskPlayAnimationM(GET_PLAYER_PED(selectedPlayer), (char *)DanceAnimeType[danceAnimTypeIndex], (char *)DanceAnimeID[danceAnimTypeIndex]); break;
		case 4: featureAnimaAction = 1; TaskPlayAnimationM(GET_PLAYER_PED(selectedPlayer), (char *)MiscAnimeType[miscAnimTypeIndex], (char *)MiscAnimeID[miscAnimTypeIndex]); break;
		case 5: featureAnimaAction = 1; TaskPlayAnimationM(GET_PLAYER_PED(selectedPlayer), (char *)NaughtAnimeType[naughtyAnimTypeIndex], (char *)NaughtyAnimeID[naughtyAnimTypeIndex]); break;
		case 6: featureAnimaAction = 1; TaskPlayAnimationM(GET_PLAYER_PED(selectedPlayer), (char *)SittingAnimeType[sittingAnimTypeIndex], (char *)SittingAnimeID[sittingAnimTypeIndex]); break;
		case 7: featureAnimaAction = 1; TaskPlayAnimationM(GET_PLAYER_PED(selectedPlayer), (char *)SportsAnimeType[sportAnimTypeIndex], (char *)SportsAnimeID[sportAnimTypeIndex]); break;
		}

	}
	void AnimationOptions()
	{
		AddTitle("Animation Options");
		AddOption("Stop Animations", null, Stop);
		AddItem("Animal Animations", (char **)AnimalAnime, &animalAnimTypeIndex, 0, 4);
		AddItem("Dancing Animations", (char **)DanceAnime, &danceAnimTypeIndex, 0, 4);
		AddItem("Misc Animations", (char **)MiscAnime, &miscAnimTypeIndex, 0, 4);
		AddItem("Naughty Animations", (char **)NaughtyAnime, &naughtyAnimTypeIndex, 0, 4);
		AddItem("Sitting Animations", (char **)SittingAnime, &sittingAnimTypeIndex, 0, 4);
		AddItem("Sports Animations", (char **)SportsAnime, &sportAnimTypeIndex, 0, 4);

		switch (selectedOption())
		{
		case 2: TaskPlayAnimationM(PLAYER_PED_ID(), (char *)AnimalAnimeType[animalAnimTypeIndex], (char *)AnimalAnimeID[animalAnimTypeIndex]); break;
		case 3: TaskPlayAnimationM(PLAYER_PED_ID(), (char *)DanceAnimeType[danceAnimTypeIndex], (char *)DanceAnimeID[danceAnimTypeIndex]); break;
		case 4: TaskPlayAnimationM(PLAYER_PED_ID(), (char *)MiscAnimeType[miscAnimTypeIndex], (char *)MiscAnimeID[miscAnimTypeIndex]); break;
		case 5: TaskPlayAnimationM(PLAYER_PED_ID(), (char *)NaughtAnimeType[naughtyAnimTypeIndex], (char *)NaughtyAnimeID[naughtyAnimTypeIndex]); break;
		case 6: TaskPlayAnimationM(PLAYER_PED_ID(), (char *)SittingAnimeType[sittingAnimTypeIndex], (char *)SittingAnimeID[sittingAnimTypeIndex]); break;
		case 7: TaskPlayAnimationM(PLAYER_PED_ID(), (char *)SportsAnimeType[sportAnimTypeIndex], (char *)SportsAnimeID[sportAnimTypeIndex]); break;
		}
	}
	void NetworkAllAnimations()
	{
		int OnlinePlyr;
		for (int i = 0; i < 16; i++)
		{
			OnlinePlyr = GET_PLAYER_PED(i);
		}
		AddTitle("Animation Options");
		AddOption("Stop Animations");
		AddItem("Animal Animations", (char **)AnimalAnime, &animalAnimTypeIndex, 0, 4);
		AddItem("Dancing Animations", (char **)DanceAnime, &danceAnimTypeIndex, 0, 4);
		AddItem("Misc Animations", (char **)MiscAnime, &miscAnimTypeIndex, 0, 4);
		AddItem("Naughty Animations", (char **)NaughtyAnime, &naughtyAnimTypeIndex, 0, 4);
		AddItem("Sitting Animations", (char **)SittingAnime, &sittingAnimTypeIndex, 0, 4);
		AddItem("Sports Animations", (char **)SportsAnime, &sportAnimTypeIndex, 0, 4);

		switch (selectedOption())
		{
		case 1: for (int i = 0; i < 16; i++){ CLEAR_PED_TASKS_IMMEDIATELY(GET_PLAYER_PED(i)); } break;
		case 2:  AnimationsForAll((char *)AnimalAnimeType[animalAnimTypeIndex], (char *)AnimalAnimeID[animalAnimTypeIndex]); break;
		case 3:  AnimationsForAll((char *)DanceAnimeType[danceAnimTypeIndex], (char *)DanceAnimeID[danceAnimTypeIndex]); break;
		case 4:  AnimationsForAll((char *)MiscAnimeType[miscAnimTypeIndex], (char *)MiscAnimeID[miscAnimTypeIndex]); break;
		case 5:  AnimationsForAll((char *)NaughtAnimeType[naughtyAnimTypeIndex], (char *)NaughtyAnimeID[naughtyAnimTypeIndex]); break;
		case 6:  AnimationsForAll((char *)SittingAnimeType[sittingAnimTypeIndex], (char *)SittingAnimeID[sittingAnimTypeIndex]); break;
		case 7:  AnimationsForAll((char *)SportsAnimeType[sportAnimTypeIndex], (char *)SportsAnimeID[sportAnimTypeIndex]); break;
		}
	}

	void NetworkMisc()
	{
		AddTitle("Miscellaneous");
		AddOption("Message Options", null, nullFunc, SUB::NETWORK_MESSAGE);
		AddOption("Kick Player ~HUD_COLOUR_GREY~(Host Only)");
		switch (selectedOption())
		{
		case 2:	if (NETWORK_IS_HOST()) { if (NETWORK_IS_SESSION_STARTED()) { if (selectedPlayer != PLAYER_ID() && !IsPlayerFriend(selectedPlayer)) { NETWORK_SESSION_KICK_PLAYER(selectedPlayer); } } }
				else ShowSubtitle("~r~Error: You are not the host"); break;
		}
	}
	void NetworkMessage()
	{
		AddTitle("Message Options");
		AddItem("Text", (char**)messagesText, &messagingTypeIndex, 0, 9);
		AddItem("Color", (char**)Colors, &colorTypeIndex, 0, 7);
		AddOption("Preview:", null, nullFunc, -1, MMSpreview);
		AddOption("Send Message");
		switch (selectedOption())
		{
		case 1: 
			if (messagingTypeIndex == 9)//custom text
			{
				StartKeyboard(KB_TEXT_MESSAGE, MMSpreview, 60);
			}
			break;
		case 4:
			int netHandle;
			NETWORK_HANDLE_FROM_PLAYER(selectedPlayer, &netHandle, 13);
			NETWORK_SEND_TEXT_MESSAGE((messagingTypeIndex == 9) ? MMSpreview : (char *)messagesText[messagingTypeIndex], &netHandle);
			ShowNotification(AddStrings("Text Message Sent To:\n", _GET_PLAYER_NAME(selectedPlayer))); break; 
		}
	}
	void NameChanger()
	{
		AddTitle("Online Name Editor");
		AddItem("Color", (char**)Colors, &colorTypeIndex, 0, 7);
		AddKeyboard("Change Online Name", KB_CHANGE_NAME);
	}









}

void networkspawner()
{
	if (featureCreateVehicle)
	{
		Hash vehicleModelHash = vehicleCustomHash;
		Vector3 v0, v1;
		GET_MODEL_DIMENSIONS(vehicleModelHash, &v0, &v1);
		Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PLAYER_PED_ID(), 0.0, v1.y + 6.0, 0.0);
		Vehicle vehicle = CreateVehicleM(vehicleModelHash, coords.x, coords.y, coords.z);
		if (DOES_ENTITY_EXIST(vehicle))
		{
			if (featureVehWarpInSpawned)
			{
				SET_ENTITY_HEADING(vehicle, GET_ENTITY_HEADING(PLAYER_PED_ID()));
				SET_PED_INTO_VEHICLE(PLAYER_PED_ID(), vehicle, -1);
			}
			if (featureMaxAllUpgrades)
			{
				int veh = GET_VEHICLE_PED_IS_USING(PLAYER_PED_ID());
				SET_VEHICLE_MOD_KIT(veh, 0);
				SET_VEHICLE_COLOURS(veh, 120, 120);
				SET_VEHICLE_NUMBER_PLATE_TEXT(veh, "Joshk326");
				SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX(veh, 1);
				TOGGLE_VEHICLE_MOD(veh, 18, 1);
				TOGGLE_VEHICLE_MOD(veh, 22, 1);
				SET_VEHICLE_MOD(veh, 16, 5, 0);
				SET_VEHICLE_MOD(veh, 12, 2, 0);
				SET_VEHICLE_MOD(veh, 11, 3, 0);
				SET_VEHICLE_MOD(veh, 14, 14, 0);
				SET_VEHICLE_MOD(veh, 15, 3, 0);
				SET_VEHICLE_MOD(veh, 13, 2, 0);
				SET_VEHICLE_WHEEL_TYPE(veh, 6);
				SET_VEHICLE_WINDOW_TINT(veh, 5);
				SET_VEHICLE_MOD(veh, 23, 19, 1);
				SET_VEHICLE_MOD(veh, 0, 1, 0);
				SET_VEHICLE_MOD(veh, 1, 1, 0);
				SET_VEHICLE_MOD(veh, 2, 1, 0);
				SET_VEHICLE_MOD(veh, 3, 1, 0);
				SET_VEHICLE_MOD(veh, 4, 1, 0);
				SET_VEHICLE_MOD(veh, 5, 1, 0);
				SET_VEHICLE_MOD(veh, 6, 1, 0);
				SET_VEHICLE_MOD(veh, 7, 1, 0);
				SET_VEHICLE_MOD(veh, 8, 1, 0);
				SET_VEHICLE_MOD(veh, 9, 1, 0);
				SET_VEHICLE_MOD(veh, 10, 1, 0);
			}
			if (featureInvincibleVehicleOnSpawn)
			{
				SET_ENTITY_INVINCIBLE(vehicle, 1);
			}


			featureCreateVehicle = false;
		}
	}
	if (featureCreatePed)
	{
		Hash PedModelName = pedCustomHash;
		Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PLAYER_PED_ID(), 0.0, 6.0, 0.0);
		Ped ped = CreatePedM(PedModelName, coords.x, coords.y, coords.z);
		if (DOES_ENTITY_EXIST(ped))
		{
			if (featureInvinciblePedOnSpawn)
			{
				SET_ENTITY_INVINCIBLE(ped, 1);
			}
			if (featureMakePedPresistent)
			{
				SET_ENTITY_AS_MISSION_ENTITY(ped, 1, 1);
			}


			featureCreatePed = false;
		}
	}
	if (featurePedAttack)
	{
		Hash PedModelName = attackPedHash;
		Ped ped = CREATE_PED(0x1A, vehToSpawnHash, Position[0], Position[1] + 2, Position[2] + 299, 204.8112f, 1, 0);
		featurePedAttack = false;
	}
	if (featureVehAttack)
	{
		Hash vehicleModelHash = attackVehHash;
		Vector3 v0, v1;
		GET_MODEL_DIMENSIONS(vehicleModelHash, &v0, &v1);
		Vehicle vehicle = CreateVehicleM(vehicleModelHash, Position[0], Position[1] + 2, Position[2] + 299);
		featureVehAttack = false;
	}
	if (featureFreezeVeh)
	{
		Hash vehicleModelHash = freezeVehHash;
		Vector3 v0, v1;
		GET_MODEL_DIMENSIONS(vehicleModelHash, &v0, &v1);
		Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(GET_PLAYER_PED(selectedPlayer), 0.0, v1.y + 6.0, 0.0);
		Vehicle vehicle = CreateVehicleM(vehicleModelHash, coords.x, coords.y, coords.z);
		featureFreezeVeh = false;
	}

}
void menu::update_status_two() {
	if (GET_GAME_TIMER() > waitTimetwo) {
		waitTimetwo = GET_GAME_TIMER() + 50;
		//50ms features

		for (int i = 0; i < 16; i++)
		{
			if (DOES_ENTITY_EXIST(GET_PLAYER_PED(i)))
			{
				if (individualPlayer[i].featureMoneyDrop)
				{
					Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(GET_PLAYER_PED(i), 0.0f, 0.0f, 1.5f);
					Hash hash = 289396019;//289396019  GET_HASH_KEY((char*)MoneyPropType[moneyPropTypeIndex])
					if (!HAS_MODEL_LOADED(hash))
						REQUEST_MODEL(hash);
					else
					{
						CREATE_AMBIENT_PICKUP(3463437675, coords.x, coords.y, coords.z, 0, 40000, hash, FALSE, TRUE);
						SET_MODEL_AS_NO_LONGER_NEEDED(hash);
					}
				}
				if (individualPlayer[i].featurePropMoneyDrop)
				{
					Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(GET_PLAYER_PED(i), 0.0f, 0.0f, 1.5f);
					Hash hash = GET_HASH_KEY((char*)MoneyPropType[moneyPropTypeIndex]);//289396019  GET_HASH_KEY((char*)MoneyPropType[moneyPropTypeIndex])
					if (!HAS_MODEL_LOADED(hash))
						REQUEST_MODEL(hash);
					else
					{
						CREATE_AMBIENT_PICKUP(3463437675, coords.x, coords.y, coords.z, 0, 40000, hash, FALSE, TRUE);
						SET_MODEL_AS_NO_LONGER_NEEDED(hash);
					}
				}
				if (individualPlayer[i].featureExplodePlayer)
				{
					Vector3 coords = GET_ENTITY_COORDS(GET_PLAYER_PED(i), 1);
					ADD_OWNED_EXPLOSION(GET_PLAYER_PED(i), coords.x, coords.y, coords.z, 29, 999.9f, 1, 0, 0.0f);
				}
				if (individualPlayer[i].featureHydratePlayer)
				{
					Vector3 coords = GET_ENTITY_COORDS(GET_PLAYER_PED(i), 1);
					ADD_OWNED_EXPLOSION(GET_PLAYER_PED(i), coords.x, coords.y, coords.z, 13, 999.9f, 1, 0, 0.0f);
				}
			}
			else
			{
				//if player no longer in session then turn off features
				individualPlayer[i].featureMoneyDrop = individualPlayer[i].featureHydratePlayer = individualPlayer[i].featurePropMoneyDrop = false;
			}
		}



	}
}
void menu::update_status_five() {
	if (GET_GAME_TIMER() > waitTimeFive) {
		waitTimeFive = GET_GAME_TIMER() + 200; //350
		//200ms features


	}
} 
void menu::update_status_six()
{
	if (GET_GAME_TIMER() > waitTimeSix) {
		waitTimeSix = GET_GAME_TIMER() + 10;
		//10ms features
		for (int i = 0; i < 16; i++)
		{
			if (DOES_ENTITY_EXIST(GET_PLAYER_PED(i)))
			{
				if (individualPlayer[i].featureStopTask) { CLEAR_PED_TASKS_IMMEDIATELY(GET_PLAYER_PED(i)); }
				if (individualPlayer[i].featureESPTracer)
				{
					Vector3 LocalPed = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
					Vector3 TargetPed = GET_ENTITY_COORDS(GET_PLAYER_PED(selectedPlayer), 1);
					DRAW_LINE(LocalPed.x, LocalPed.y, LocalPed.z, TargetPed.x, TargetPed.y, TargetPed.z, 255, 0, 0, 255);
					DRAW_LINE(TargetPed.x, TargetPed.y, TargetPed.z, LocalPed.x, LocalPed.y, LocalPed.z, 255, 0, 0, 255);
				}
				if (individualPlayer[i].featureRedBoxes)
				{
					Vector3 handleCoords = GET_ENTITY_COORDS(GET_PLAYER_PED(selectedPlayer), 1);
					DRAW_LINE(handleCoords.x + 0.5, handleCoords.y + 0.5, handleCoords.z + 0.75, handleCoords.x + 0.5, handleCoords.y - 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x + 0.5, handleCoords.y - 0.5, handleCoords.z + 0.75, handleCoords.x - 0.5, handleCoords.y - 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x - 0.5, handleCoords.y - 0.5, handleCoords.z + 0.75, handleCoords.x - 0.5, handleCoords.y + 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x - 0.5, handleCoords.y + 0.5, handleCoords.z + 0.75, handleCoords.x + 0.5, handleCoords.y + 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);

					DRAW_LINE(handleCoords.x + 0.5, handleCoords.y + 0.5, handleCoords.z - 0.75, handleCoords.x + 0.5, handleCoords.y - 0.5, handleCoords.z - 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x + 0.5, handleCoords.y - 0.5, handleCoords.z - 0.75, handleCoords.x - 0.5, handleCoords.y - 0.5, handleCoords.z - 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x - 0.5, handleCoords.y - 0.5, handleCoords.z - 0.75, handleCoords.x - 0.5, handleCoords.y + 0.5, handleCoords.z - 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x - 0.5, handleCoords.y + 0.5, handleCoords.z - 0.75, handleCoords.x + 0.5, handleCoords.y + 0.5, handleCoords.z - 0.75, 255, 0, 0, 255);

					DRAW_LINE(handleCoords.x + 0.5, handleCoords.y + 0.5, handleCoords.z - 0.75, handleCoords.x + 0.5, handleCoords.y + 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x + 0.5, handleCoords.y - 0.5, handleCoords.z - 0.75, handleCoords.x + 0.5, handleCoords.y - 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x - 0.5, handleCoords.y - 0.5, handleCoords.z - 0.75, handleCoords.x - 0.5, handleCoords.y - 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);
					DRAW_LINE(handleCoords.x - 0.5, handleCoords.y + 0.5, handleCoords.z - 0.75, handleCoords.x - 0.5, handleCoords.y + 0.5, handleCoords.z + 0.75, 255, 0, 0, 255);
				}
				if (individualPlayer[i].featurewantedlevel)
				{
					int player = GET_PLAYER_PED(selectedPlayer);
					SET_DISPATCH_COPS_FOR_PLAYER(player, 1);
					SET_PLAYER_WANTED_LEVEL_NOW(player, 1);
					SET_PLAYER_WANTED_LEVEL_NO_DROP(player, 1, 6);
					SET_PLAYER_WANTED_LEVEL(player, 6, 1);
					SET_WANTED_LEVEL_MULTIPLIER(10);
					SET_WANTED_LEVEL_DIFFICULTY(player, 10);
					REPORT_CRIME(player, 36, GET_WANTED_LEVEL_THRESHOLD(6));
				}
				if (individualPlayer[i].featureNetworkExp)
				{
					Ped player = GET_PLAYER_PED(selectedPlayer);
					if (IS_PED_SHOOTING(player) || tryAgain) {
						Vector3 coords;
						GET_PED_LAST_WEAPON_IMPACT_COORD(player, coords);
						if (coords.x != 0 || coords.y != 0 || coords.z != 0) {
							ADD_OWNED_EXPLOSION(player, coords.x, coords.y, coords.z, 5, .75, true, false, .1);
							tryAgain = false;
						}
						else {
							tryAgain = true;
						}
					}
				}
			}
			else
			{
				//if player no longer in session then turn off features
				individualPlayer[i].featurewantedlevel = individualPlayer[i].featureRedBoxes = individualPlayer[i].featureESPTracer = individualPlayer[i].featureStopTask = individualPlayer[i].featureNetworkExp = false;
			}
		}

		if (featureESPAll) ESPAll();
		if (featureWeaponLaser)
		{
			Vector3 v0, v1;
			Vehicle veh = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
			Vector3 coords0from;
			Vector3 coords1from;
			Vector3 coords0to;
			Vector3 coords1to;
			coords0from = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -(v1.x + 0.25f), v1.y + 1.25f, 0.1);
			coords1from = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, (v1.x + 0.25f), v1.y + 1.25f, 0.1);
			coords0to = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -v1.x, v1.y + 100.0f, 0.1f);
			coords1to = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, v1.x, v1.y + 100.0f, 0.1f);
			DRAW_LINE(coords0from.x, coords0from.y, coords0from.z, coords0to.x, coords0to.y, coords0to.z, 255, 0, 0, 255);
			DRAW_LINE(coords1from.x, coords1from.y, coords1from.z, coords1to.x, coords1to.y, coords1to.z, 255, 0, 0, 255);
		}
	}
}
void menu::update_vehicle_guns() {
	if (GET_GAME_TIMER() > waitTimeGuns) {
		waitTimeGuns = GET_GAME_TIMER() + 150;
		//150ms features

		//vehicle weapons
		if (featureVehicleWeapons)
		{
			DisableControls();
			Vector3 coords0from;
			Vector3 coords1from;
			Vector3 coords0to;
			Vector3 coords1to;
			if (IS_CONTROL_JUST_PRESSED(2, INPUT_SCRIPT_LB) || IS_CONTROL_PRESSED(2, INPUT_SCRIPT_LB))
			{
				if (IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
				{
					Vehicle veh = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
					Vector3 v0, v1;
					GET_MODEL_DIMENSIONS(GET_ENTITY_MODEL(veh), &v0, &v1);
					coords0from = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -(v1.x + 0.25f), v1.y + 1.25f, 0.1);
					coords1from = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, (v1.x + 0.25f), v1.y + 1.25f, 0.1);
					coords0to = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -v1.x, v1.y + 100.0f, 0.1f);
					coords1to = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, v1.x, v1.y + 100.0f, 0.1f);

					Vector3 getcoords1 = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, 0.6f, 0.6707f, 0.3711f);
					Vector3 getcoords2 = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -0.6f, 0.6707f, 0.3711f);
					Vector3 getcoords3 = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, 0.6f, 25.0707f, 0.3711f);
					Vector3 getcoords4 = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -0.6f, 25.0707f, 0.3711f);
					DRAW_LINE(getcoords1.x, getcoords1.y, getcoords1.z, getcoords2.x, getcoords2.y, getcoords2.z, 255, 0, 0, 255);
					DRAW_LINE(getcoords3.x, getcoords3.y, getcoords3.z, getcoords4.x, getcoords4.y, getcoords4.z, 255, 0, 0, 255);


					Hash weaponAsset = GET_HASH_KEY((char *)WeaponAsset[vehicleWeaponIndex]);
					if (!HAS_WEAPON_ASSET_LOADED(weaponAsset))
						REQUEST_WEAPON_ASSET(weaponAsset, 31, 0);

					SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords0from.x, coords0from.y, coords0from.z,
						coords0to.x, coords0to.y, coords0to.z,
						250, 1, weaponAsset, PLAYER_PED_ID(), 1, 0, -1.0);
					SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords1from.x, coords1from.y, coords1from.z,
						coords1to.x, coords1to.y, coords1to.z,
						250, 1, weaponAsset, PLAYER_PED_ID(), 1, 0, -1.0);
				}
			}
		}
		
		

	}
}

void menu::update_features()
{
	menu::update_status_two();
	menu::update_status_five();
	menu::update_status_six();
	menu::update_vehicle_guns();

	myVeh = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0); // Store current vehicle
	int waterobj;
	int obj;
	int veh;

	if (keyboardActive)
	{
		if (UPDATE_ONSCREEN_KEYBOARD() == 1)
		{
			keyboardActive = false;
			ShowSubtitle(GET_ONSCREEN_KEYBOARD_RESULT());
			switch (keyboardAction)
			{
			case KB_DEFAULT: *keyboardVar = StringToInt(GET_ONSCREEN_KEYBOARD_RESULT()); break;
			case KB_DEFAULT_FLOAT: *keyboardVarFloat = StringToInt(GET_ONSCREEN_KEYBOARD_RESULT()); break;
			case KB_SETTINGS_RGB2: int tempHash = ABSI(StringToInt(GET_ONSCREEN_KEYBOARD_RESULT())); if (!(tempHash >= 0 && tempHash <= 255)) PrintError_InvalidInput(); else *settings_rgba2 = tempHash; break;
			case KB_CHANGE_NAME:
				strncpy((char*)0x41143344, AddStrings((char*)ColorsID[colorTypeIndex], GET_ONSCREEN_KEYBOARD_RESULT()), strlen(AddStrings((char*)ColorsID[colorTypeIndex], GET_ONSCREEN_KEYBOARD_RESULT())));
				*(char*)(0x41143344 + strlen(AddStrings((char*)ColorsID[colorTypeIndex], GET_ONSCREEN_KEYBOARD_RESULT()))) = 0;
				strncpy((char*)0x01FF248C, AddStrings((char*)ColorsID[colorTypeIndex], GET_ONSCREEN_KEYBOARD_RESULT()), strlen(AddStrings((char*)ColorsID[colorTypeIndex], GET_ONSCREEN_KEYBOARD_RESULT())));
				*(char*)(0x01FF248C + strlen(AddStrings((char*)ColorsID[colorTypeIndex], GET_ONSCREEN_KEYBOARD_RESULT()))) = 0;
				break;
			case KB_SKINCHANGER: featureCustomSkin = GET_ONSCREEN_KEYBOARD_RESULT(); featureIsCustomSkin = 1, featureSkinChanger = 1; break;
			case KB_CHANGE_PLATE_TEXT: SET_VEHICLE_NUMBER_PLATE_TEXT(myVeh, GET_ONSCREEN_KEYBOARD_RESULT()); break;
			case KB_CUSTOM_OBJECT: ObjectCreater(GET_HASH_KEY(GET_ONSCREEN_KEYBOARD_RESULT())); break;
			case KB_TEXT_MESSAGE: MMSpreview = AddStrings((char*)ColorsID[colorTypeIndex], GET_ONSCREEN_KEYBOARD_RESULT()); break;
			}
		}
		else if (UPDATE_ONSCREEN_KEYBOARD() == 2 || UPDATE_ONSCREEN_KEYBOARD() == 3)
		{
			keyboardActive = false;
			ShowSubtitle("~r~Error: Keyboard Cancelled");
		}
	}
	if (featureRequestControlOfEnt)
	{
		if (!NETWORK_HAS_CONTROL_OF_ENTITY(featureReqEnt))
			NETWORK_REQUEST_CONTROL_OF_ENTITY(featureReqEnt);
		else
		{
			NETWORK_REQUEST_CONTROL_OF_ENTITY(featureReqEnt);
			Vehicle networkPlayerVeh = GET_VEHICLE_PED_IS_IN(GET_PLAYER_PED(selectedPlayer), 1);
			switch (featureRequestControlAction)
			{
			case RC_DEFAULT_NONE: break;
			case RC_SETENTITYCOORDS: SET_ENTITY_COORDS_NO_OFFSET(featureReqEnt, setcoordsx, setcoordsy, setcoordsz, setxAxis, setyAxis, setzAxis); break;
			case RC_FIXSELFVEHICLE: SET_VEHICLE_FIXED(myVeh); SET_VEHICLE_DIRT_LEVEL(myVeh, 0.0f); _SET_VEHICLE_PAINT_FADE(myVeh, 0.0f); break;
			case RC_TOGGLEDOORSSELF: TOGGLE_ALL_DOORS(myVeh, featureToggleDoors); break;
			case RC_FREEZESELFVEHICLE: FREEZE_ENTITY_POSITION(myVeh, featureFreezeVehicle); break;
			case RC_DELETEVEHICLESELF: SET_ENTITY_AS_MISSION_ENTITY(featureReqEnt, 0, 1); DELETE_VEHICLE(&featureReqEnt); break;
			case RC_FIXNETWORKVEHICLE: SET_VEHICLE_FIXED(networkPlayerVeh); SET_VEHICLE_DIRT_LEVEL(networkPlayerVeh, 0.0f); break;
			case RC_DELETENETWORKVEHICLE: SET_ENTITY_AS_MISSION_ENTITY(networkPlayerVeh, 0, 1); DELETE_VEHICLE(&networkPlayerVeh); break;
			case RC_SLINGSHOTNETWORKVEHICLE: APPLY_FORCE_TO_ENTITY(networkPlayerVeh, 1, 0, 0, 100, 0, 0, 0, 1, 0, 1, 1, 1, 1); break;
			case RC_REMOTCONTROLNETWORKVEHICLE: if (IS_PED_IN_ANY_VEHICLE(GET_PLAYER_PED(selectedPlayer), 0) && (networkPlayerVeh != myVeh)) { SET_VEHICLE_DOORS_LOCKED(networkPlayerVeh, 1); ATTACH_ENTITY_TO_ENTITY(networkPlayerVeh, myVeh, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0, 2, 1); } break;
			case RC_DETACHNETWORKVEHICLE: DETACH_ENTITY(networkPlayerVeh, 1, 1); SET_VEHICLE_DOORS_LOCKED(networkPlayerVeh, 0); break;
			case RC_PAINTALLPINK: for (int i = 0; i < 16; i++){ SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(GET_VEHICLE_PED_IS_USING(GET_PLAYER_PED(i)), 255, 0, 255); SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(GET_VEHICLE_PED_IS_USING(GET_PLAYER_PED(i)), 255, 0, 255);}break;
			case RC_VEHINVISIBLITY: SET_ENTITY_VISIBLE(myVeh, featureVehInvisibility); break;
			}
			featureRequestControlOfEnt = false;
		}
	}
	if (featureDoAnim)
	{
		REQUEST_ANIM_DICT(featureAnimDic);
		if (HAS_ANIM_DICT_LOADED(featureAnimDic))
		{
			if (featureAnimaAction == 1)
			{
				CLEAR_PED_TASKS_IMMEDIATELY(featureAnimPed);
				Vector3 pedcoords = GET_ENTITY_COORDS(featureAnimPed, 1);
				int netscene = NETWORK_CREATE_SYNCHRONISED_SCENE(pedcoords.x, pedcoords.y, pedcoords.z, 0.0, 0.0, 0.0, 269, 0, 1, 0x3f800000, 0, 0x3f800000);	
				NETWORK_ADD_PED_TO_SYNCHRONISED_SCENE(featureAnimPed, netscene, featureAnimDic, featureAnimName, 8.0f, 0.0f, 6, 0, 100);
				NETWORK_START_SYNCHRONISED_SCENE(netscene);
				featureDoAnim = 0, featureAnimaAction = 0; //reset
			}
			else
			{
				if (!NETWORK_REQUEST_CONTROL_OF_ENTITY(featureAnimPed))
					NETWORK_HAS_CONTROL_OF_ENTITY(featureAnimPed);
				else
				{
					TASK_PLAY_ANIM(featureAnimPed, featureAnimDic, featureAnimName, featureAnimSpeed, featureanimSpeedMuli, -1, featureAnimFlag, 0, 0, 0, 0);
					featureDoAnim = 0;
				}
			}
		}
	}
	if (featureCashAll)
	{
		for (int i = 0; i < 16; i++)
		{
			Vector3 coords = GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(GET_PLAYER_PED(i), 0.0f, 0.0f, 1.5f);
			Hash hash = 289396019;//289396019
			if (!HAS_MODEL_LOADED(hash))
				REQUEST_MODEL(hash);
			else
			{
				CREATE_AMBIENT_PICKUP(3463437675, coords.x, coords.y, coords.z, 0, 40000, hash, FALSE, TRUE);
				SET_MODEL_AS_NO_LONGER_NEEDED(hash);
			}
		}
	}
	if (featureInvincibility) { SET_PLAYER_INVINCIBLE(PLAYER_ID(), TRUE); SET_ENTITY_INVINCIBLE(PLAYER_PED_ID(), TRUE); SET_ENTITY_ONLY_DAMAGED_BY_PLAYER(PLAYER_PED_ID(), TRUE); SET_ENTITY_CAN_BE_DAMAGED(PLAYER_PED_ID(), FALSE);}
	if (featureSuperJump) { SET_SUPER_JUMP_THIS_FRAME(PLAYER_ID()); }
	if (featureSuperRun) { if (IS_CONTROL_PRESSED(2, INPUT_SCRIPT_RDOWN)) { APPLY_FORCE_TO_ENTITY(PLAYER_PED_ID(), 1, 0.0f, 2.8f, 0.0, 0.0f, 0.0f, 0.0f, 1, 1, 1, 1, 0, 1); SET_PED_CAN_RAGDOLL(PLAYER_PED_ID(), FALSE);  SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(PLAYER_PED_ID(), FALSE); SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(PLAYER_PED_ID(), 1); } }
	if (featureInvisibility) { SET_ENTITY_VISIBLE(PLAYER_PED_ID(), FALSE); }
	if (featureNeverWanted) { CLEAR_PLAYER_WANTED_LEVEL(PLAYER_ID()); }
	if (featureUnlimitedAmmo){ SET_PED_INFINITE_AMMO_CLIP(PLAYER_PED_ID(), TRUE); }
	if (featureSeatBelt){ SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(PLAYER_PED_ID(), TRUE); SET_ENTITY_PROOFS(PLAYER_PED_ID(), TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);  featureRagDoll = 1; }
	if (featureRagDoll){ SET_PED_CAN_RAGDOLL(PLAYER_PED_ID(), FALSE); SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(PLAYER_PED_ID(), FALSE); }
	if (featureTalkingPlayers)
	{
		memset(Talk, 0, sizeof(Talk));
		char* talker;
		for (int i = 0; i < 18; i++)
		{
			if (NETWORK_IS_PLAYER_TALKING(i))
			{
				talker = GET_PLAYER_NAME(i);
				snprintf(Talk, 100, "~r~Talking: ~s~%s", talker);
				Chat(Talk, i);
			}
		}
	}
	if (featureCarJump)
	{
		if (IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
		{
			if (IS_CONTROL_PRESSED(0, INPUT_FRONTEND_X))
			{
				APPLY_FORCE_TO_ENTITY(GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0), true, 0, 0, 1.0f, 0, 0, 0, true, true, true, true, false, true);
			}
		}
		else
		{
			ShowSubtitle("~r~Error: ~s~You Must Be In A Vehicle");
		}
	}
	if (featureGhostRider)
	{
		REQUEST_NAMED_PTFX_ASSET("scr_rcbarry2");
		if (!HAS_NAMED_PTFX_ASSET_LOADED("scr_rcbarry2"))
		{
			_SET_PTFX_ASSET_NEXT_CALL("scr_rcbarry2");
		}
		else
		{
			float f1;
			f1 = 1.0;
			_SET_PTFX_ASSET_NEXT_CALL("scr_rcbarry2");
			_START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE_2("ptfx_smoke_billow_anim_rgba", PLAYER_PED_ID(), 0, 0, 0, 0, 0, 0, SKEL_Head, 0.1f, 0, 0, 0);
			_START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE_2("ptfx_smoke_billow_anim_rgba", PLAYER_PED_ID(), 0, 0, 0, 0, 0, 0, SKEL_L_Hand, 0.1f, 0, 0, 0);
			_START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE_2("ptfx_smoke_billow_anim_rgba", PLAYER_PED_ID(), 0, 0, 0, 0, 0, 0, SKEL_R_Hand, 0.1f, 0, 0, 0);
			_START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE_2("ptfx_smoke_billow_anim_rgba", PLAYER_PED_ID(), 0, 0, 0, 0, 0, 0, SKEL_L_Foot, 0.1f, 0, 0, 0);
			_START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE_2("ptfx_smoke_billow_anim_rgba", PLAYER_PED_ID(), 0, 0, 0, 0, 0, 0, SKEL_R_Foot, 0.1f, 0, 0, 0);
			_START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE_2("ptfx_smoke_billow_anim_rgba", PLAYER_PED_ID(), 0, 0, 0, 0, 0, 0, SKEL_Spine_Root, 0.1f, 0, 0, 0);
			SET_PARTICLE_FX_NON_LOOPED_COLOUR(72, 236, 232);
		}
	}
	if (featureSkinChanger)
	{
		Hash model;
		featureIsCustomSkin ? model = GET_HASH_KEY(featureCustomSkin) : model = GET_HASH_KEY((char *)ModelTypes[modelTypeIndex]);
		if (!HAS_MODEL_LOADED(model))
			REQUEST_MODEL(model);
		else
		{
			SET_PLAYER_MODEL(PLAYER_ID(), model);
			//SET_PED_RANDOM_COMPONENT_VARIATION(PLAYER_PED_ID(), FALSE);
			SET_PED_DEFAULT_COMPONENT_VARIATION(PLAYER_PED_ID());
			SET_MODEL_AS_NO_LONGER_NEEDED(model);
			featureIsCustomSkin = 0, featureSkinChanger = 0; //reset both
		}
	}
	if (featureInnerForce)
	{
		if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_X))
		{
			Vector3 mycoords = GET_ENTITY_COORDS(PLAYER_PED_ID(), true);
			int closestVehicleToPlayer = GET_CLOSEST_VEHICLE(mycoords.x, mycoords.y, mycoords.z, 1000.0, 0, 100);
			for (int tire = 0; tire <= 7; tire++)
			TaskPlayAnimationM(PLAYER_PED_ID(), "rcmcollect_paperleadinout@", "meditiate_idle");
			APPLY_FORCE_TO_ENTITY(closestVehicleToPlayer, true, 0, 0, 1.0f, 0, 0, 0, true, true, true, true, false, true);
							
		}
	}
	if (featureDisplayFPS)
	{
		float LastFrameTime = GET_FRAME_TIME();
		int getFPS = (1.0f / LastFrameTime);
		char FPStext[60];
		snprintf(FPStext, sizeof(FPStext), "FPS: ~r~ %d", getFPS);
		setupdraw();
		SET_TEXT_FONT(6);
		SET_TEXT_SCALE(0.5f, 0.5f);
		SET_TEXT_COLOUR(255, 255, 255, 255);
		SET_TEXT_CENTRE(0);
		drawstring(FPStext, 0.040f, 0.030f);
	}
	if (featureObjectHash)
	{
		int obj = GET_ENTITY_MODEL(SpawnedObj);
		char objtext[60];
		sprintf(objtext, "Object Hash: ~r~ %d", obj);
		setupdraw();
		SET_TEXT_FONT(4);
		SET_TEXT_SCALE(0.35f, 0.35f);
		SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
		SET_TEXT_CENTRE(0);
		drawstring(!strcmp(objtext, "Object Hash: ~r~ 0") ? (char *)"Object Hash: ~r~ No Object" : objtext, -0.042f + menuPos, 0.130f);
		DRAW_RECT(-0.008f + menuPos, 0.142f, 0.085f, 0.04f, BG.R, BG.G, BG.B, BG.A);
	}
	if (featureSpeedometerSkin)
	{
		int car = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 0);
		float speed = GET_ENTITY_SPEED(car);
		speed = speed * 4.30;
		int convertedspeed = (int)speed; // float to int		
		DrawSprite("mpmissmarkers256", "last_team_standing_icon", 0.12f, 0.2f, 0.2f, 0.3f, 0.0f, 255, 255, 255, 225);
		if (speed > 293) //max speed
		{
			DrawSprite("mpmissmarkers256", "darts_icon", 0.12f, 0.2f, 0.2f, 0.3f, 294 - 40, 255, 255, 255, 255);
		}
		else
		{
			DrawSprite("mpmissmarkers256", "darts_icon", 0.12f, 0.2f, 0.2f, 0.3f, speed - 40, 255, 255, 255, 255);
		}
	}

	if (featureCashGun)
	{
		Vector3 ImpactCoords999999;
		if (GET_PED_LAST_WEAPON_IMPACT_COORD(PLAYER_PED_ID(), ImpactCoords999999))
		{
			featureCashGunToggle = 1;
		}
		if (featureCashGunToggle)
		{
			REQUEST_MODEL(0x113FD533);
			if (HAS_MODEL_LOADED(0x113FD533) == 1)
			{
				CREATE_AMBIENT_PICKUP(0xCE6FDD6B, ImpactCoords999999.x, ImpactCoords999999.y, ImpactCoords999999.z, 0, 40000, 0x113FD533, 0, 1);
				SET_MODEL_AS_NO_LONGER_NEEDED(0x113FD533);
			}
		}
		featureCashGunToggle = 0;
	}
	if (featureExplosiveBullets)
	{
		Ped player = PLAYER_PED_ID();
		if (IS_PED_SHOOTING(player) || tryAgain) {
			Vector3 coords;
			GET_PED_LAST_WEAPON_IMPACT_COORD(player, coords);
			if (coords.x != 0 || coords.y != 0 || coords.z != 0) {
				ADD_OWNED_EXPLOSION(player, coords.x, coords.y, coords.z, 5, .75, true, false, .1);
				tryAgain = false;
			}
			else {
				tryAgain = true;
			}
		}
	}
	if (featureNeedForSpeed)
	{
		if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_SCRIPT_RB))
		{
			SET_VEHICLE_BOOST_ACTIVE(GET_VEHICLE_PED_IS_IN((Ped)PLAYER_PED_ID, 1), 1);
		}
		else
		{
			SET_VEHICLE_BOOST_ACTIVE(GET_VEHICLE_PED_IS_IN((Ped)PLAYER_PED_ID, 1), 0);
		}
	}
	if (featureHandlingFly)
	{
		DisableControls();
		if (myVeh != NULL)
		{
			SET_VEHICLE_GRAVITY(myVeh, FALSE); // No more falling.
			fly_pitch = GET_ENTITY_PITCH(myVeh);
			float curSpeed = GET_ENTITY_SPEED(myVeh);
			char Title[60];
			char Forward[60];
			char Backward[60];
			char Left[60];
			char Right[60];
			char PitchUP[60];
			char PitchDN[60];
			char Stop[60];
			snprintf(Title, sizeof(Title), "~r~Fly Vehicle Controls:");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(titletext.R, titletext.G, titletext.B, titletext.A);
			SET_TEXT_CENTRE(0);
			drawstring(Title, 0.040f, 0.030f);
			snprintf(Forward, sizeof(Forward), "Forward: ~r~Dpad Up");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
			SET_TEXT_CENTRE(0);
			drawstring(Forward, 0.040f, 0.060f);
			snprintf(Backward, sizeof(Backward), "Backward: ~r~Dpad Down");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
			SET_TEXT_CENTRE(0);
			drawstring(Backward, 0.040f, 0.090f);// +20
			snprintf(Left, sizeof(Left), "Left: ~r~Dpad Left");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
			SET_TEXT_CENTRE(0);
			drawstring(Left, 0.040f, 0.120f);
			snprintf(Right, sizeof(Right), "Right: ~r~Dpad Right");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
			SET_TEXT_CENTRE(0);
			drawstring(Right, 0.040f, 0.150f);
			snprintf(PitchUP, sizeof(PitchUP), "Pitch Up: ~r~L1");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
			SET_TEXT_CENTRE(0);
			drawstring(PitchUP, 0.040f, 0.180f);
			snprintf(PitchDN, sizeof(PitchDN), "Pitch Down: ~r~R1");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
			SET_TEXT_CENTRE(0);
			drawstring(PitchDN, 0.040f, 0.210f);
			snprintf(Stop, sizeof(Stop), "Stop: ~r~X");
			setupdraw();
			SET_TEXT_FONT(6);
			SET_TEXT_SCALE(0.5f, 0.5f);
			SET_TEXT_COLOUR(optiontext.R, optiontext.G, optiontext.B, optiontext.A);
			SET_TEXT_CENTRE(0);
			drawstring(Stop, 0.040f, 0.240f);
			bool isBackward;
			if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_SCRIPT_PAD_UP)) /* W DOWN - GO FORWARD */
			{
				isBackward = false;
				SET_ENTITY_ROTATION(myVeh, fly_pitch, 0.0f, fly_yaw, 2, TRUE);
				NETWORK_REQUEST_CONTROL_OF_ENTITY(myVeh);
				SET_VEHICLE_FORWARD_SPEED(myVeh, curSpeed += 3.0f);
			}
			else if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_SCRIPT_PAD_DOWN)) /* S DOWN - GO BACKWARD */
			{
				isBackward = true;
				SET_ENTITY_ROTATION(myVeh, fly_pitch, 0.0f, fly_yaw, 2, TRUE);
				NETWORK_REQUEST_CONTROL_OF_ENTITY(myVeh);
				SET_VEHICLE_FORWARD_SPEED(myVeh, (curSpeed * -1.0f) - 3.0f);
			}
			else
			{
				SET_ENTITY_ROTATION(myVeh, fly_pitch, fly_roll, fly_yaw, 2, TRUE);
				if (curSpeed > 3.0f && isBackward)
				{
					SET_VEHICLE_FORWARD_SPEED(myVeh, curSpeed - 3.0f);
				}
				else
				{
					SET_VEHICLE_FORWARD_SPEED(myVeh, 0.0f);
				}
			}

			if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_SCRIPT_PAD_LEFT)) /* A DOWN - TURN LEFT */
			{
				SET_ENTITY_ROTATION(myVeh, fly_pitch, fly_roll, fly_yaw += 4.5f, 2, TRUE);
			}

			if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_SCRIPT_PAD_RIGHT)) /* D DOWN - TURN RIGHT */
			{
				SET_ENTITY_ROTATION(myVeh, fly_pitch, fly_roll, fly_yaw -= 4.5f, 2, TRUE);
			}

			if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_RB)) /* SHIFT DOWN - PITCH UP */
			{
				SET_ENTITY_ROTATION(myVeh, fly_pitch -= 4.5f, fly_roll, fly_yaw, 2, TRUE);
			}

			if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_LB)) /* CTRL DOWN - PITCH DOWN */
			{
				SET_ENTITY_ROTATION(myVeh, fly_pitch += 4.5f, fly_roll, fly_yaw, 2, TRUE);
			}

			if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_ACCEPT)) /* SPACE DOWN - STOP */
			{
				SET_ENTITY_ROTATION(myVeh, 0.0f, fly_roll, fly_yaw, 2, TRUE);
				NETWORK_REQUEST_CONTROL_OF_ENTITY(myVeh);
				SET_VEHICLE_FORWARD_SPEED(myVeh, 0.0f);
			}
		}
	}
	if (loop_RainbowBoxes && GET_GAME_TIMER() >= livetimer)
	{
		line.R = RandomRGB(); line.G = RandomRGB(); line.B = RandomRGB();
		BG.R = RandomRGB(); BG.G = RandomRGB(); BG.B = RandomRGB();
		selectedtext.R = RandomRGB(); selectedtext.G = RandomRGB(); selectedtext.B = RandomRGB();
	};

	if (featureGravityGun) set_gravity_gun();
	if (featureSuperMan)
	{
		if (IS_CONTROL_PRESSED(2, INPUT_SCRIPT_RT))
		{
			APPLY_FORCE_TO_ENTITY(PLAYER_PED_ID(), 1, 0.0f, 0.0f, 2.000f, 0.0f, 0.0f, 0.0f, 1, 1, 1, 1, 0, 1);
		}
		else
		if (IS_CONTROL_PRESSED(2, INPUT_SCRIPT_LT))
		{
			APPLY_FORCE_TO_ENTITY(PLAYER_PED_ID(), 1, 0.0f, 0.0f, -2.000f, 0.0f, 0.0f, 0.0f, 1, 1, 1, 1, 0, 1);
		}
	}
	if (featureSnapGround) SnapGround();
	if (featureFreezeObj) FreezeObject();
	if (featureSpeedoSpeedText) SpeedoKPM();
	if (featurePropGun)
	{
		int PedHandle = PLAYER_PED_ID(), ObjectHash = GET_HASH_KEY((char*)GunObjectTypes[propObjectTypeIndex]);
		Vector3 Coords;
		Vector3 Pos;
		float Loc[2];
		if (featurePropGun)
		{
			if (IS_PED_SHOOTING(PedHandle))
			{
				GET_PED_LAST_WEAPON_IMPACT_COORD(PedHandle, Coords);
				Object obj = CREATE_OBJECT(ObjectHash, Pos.x, Pos.y, Pos.z, 1, 1, 1);
				FREEZE_ENTITY_POSITION(obj, 1);
			}
		}
	}
	if (featureWaterDrive)
	{
		veh = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 1);
		Vector3 mycoords = GET_ENTITY_COORDS(veh, 1);
		mycoords.z = WaterAir;
		SET_ENTITY_COORDS(waterobj, mycoords.x, mycoords.y, mycoords.z, 1, 0, 0, 1);
		SET_ENTITY_ROTATION(waterobj, 180, 90, 190, 2, 1);
		FREEZE_ENTITY_POSITION(waterobj, true);
	}
	if (DriveWater)
	{
		veh = GET_VEHICLE_PED_IS_IN(PLAYER_PED_ID(), 1);
		Vector3 mycoords = GET_ENTITY_COORDS(veh, 1);
		mycoords.z = mycoords.z - 2.75;
		mycoords.x = mycoords.x - 3;
		obj = GET_HASH_KEY("prop_ld_ferris_wheel");
		waterobj = CREATE_OBJECT(obj, mycoords.x,mycoords.y,mycoords.z, 1, 1, 0);
		SET_ENTITY_VISIBLE(waterobj, 0);
	}
	if (featureModderProtection)
	{	
		FreezeProtection();
		int modleHash;
		Vector3 mycoords = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
		Hash modelHashes[] = { 0x2E28CA22, 0xA50DDDD0, 0xEFC4165A, 0x8E8C7A5B, 0x456AA864, 0xBE862050, 0xB20E5785, 0x58D3B4EA, 0xC42C019A, 0x8AF58425, 0x3DC31836, 0xA9BD0D16, 0x1AFA6A0A, 0x4B3D240F, 0x40F52369, 0xF830B63E, 0xD541462D, 0x532B1DD1, 0x0E3BA450, 0xFB631122, 0x5571173D, 0x6AD326C2, 0x7FFBC1E2, 0x678FC2DB, 0x5869A8F8, 0xE6CB661E, 0x2AE13DFA, 0x29CB0F3C, 0x922C2A43, 0xFA686C0E, 0x1F550C17, 0x5B5C4263, 0x39885BB5, 0x16A39A90, 0xE3CE09E2, 0x927A5723, 0x34D5D3FD, 0xB467C540, 0x745F3383, 0x392D62AA, 0x07121AC4, 0x0E8032E4, 0xD44295DD, 0x6F9939C7, 0x9C762726, 0x8973A868, 0xC2BC19CD, 0x858BB1D0, 0x3B21C5E7, 0xDFF68A19, 0x69E5D9CC, 0x6F204E3A, 0x9B862E76, 0x16A39A90, 0xDCA9A809, 0x7C035CA2, 0xE3CE09E2, 0xC3F13FCC, 0xC079B265, 0x7121AC4, 0x745F3383, 0x51709ADC, 0x392D62AA, 0x1B6ED610, 0xE92E717E, 0x82826CD2, 0xEB2E00E0, 0x8DA1C0E, 0xEFC4165A, 0x1081FBDD, 0x4AF2CCB6, 0xE2BA016F, 0x2468F271, 0x8E1E7CCF, 0xC40BBB0B, 0x0A61994, 0x1AB39621, 0xDF9841D7, 0x874B5974, 0xC9751EF7, 0xC7C649FF, 0x74E9F5BB, 0x336E5E2A, 0xC42C019A, 0xDF5F9F7F, 0x98EE1ACD, 0x6B795EBC, 0x9BE9742E, 0x2D4768B4, 0x54BBA095, 0xA7CF17C4, 0x7E6CAA3B, 0x997021A9, 0x14E3D6EB, 0x6AE93235, 0x63F9CEA3, 0x66477EB0, 0xAE588C5F, 0x7EA4A671, 0x47D2164, 0xE2BA016F, 0xF01B4D4, 0xC6FED6DC, 0xC89630B8, 0xA2CE7D2, 0x80D6E7F4, 0x2C98B0ED, 0x2C98B0ED, 0xD3674F13, 0x2C98B0ED, 0xD3674F13, 0x2C98B0ED, 0x2C98B0ED, 0x5A8F8CD2, 0x97A58869, 0x6DB9599A, 0x8A451C5C, 0x69CA00DD, 0x683475EE, 0x5A9789A0, 0xE1C17F6F, 0x531135E6, 0x3042936E, 0xF87EEF6, 0xE0264F5D, 0xBB55760A, 0x58BA1208, 0xC80467C6 };
		if (Hash modleHash = (Hash)modelHashes)
		{
		Object obj = GET_CLOSEST_OBJECT_OF_TYPE(mycoords.x, mycoords.y, mycoords.z, 2.0f, modleHash, 1, 0, 0);
		if (DOES_ENTITY_EXIST(obj))
		{
			DELETE_ENTITY(&obj);
		}
		}
	}
	if (featureDeleteGun){ DeleteAimingObjectFunction(); }
	if (featureForgeGun){ ForgeGunFunction(); }
	if (featureTeleportGun){ TeleportGun(); }
	if (featurenoclip){ Nocliploop(); }
	if (featureJetAttack)
	{
		if (HAS_MODEL_LOADED(vehToSpawnHash))//JetAttackFunction
		if (HAS_MODEL_LOADED(vehToSpawnHash1))
		{
			int group;
			int spawnvehicle;
			GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
			attackVehHash = vehToSpawnHash1; featureCreateVehicle = true;
			spawnvehicle = attackVehHash = vehToSpawnHash1; featureVehAttack = true;
			int chop = attackPedHash = vehToSpawnHash; featurePedAttack = true;
			if (DOES_ENTITY_EXIST(spawnvehicle)){
				if (DOES_ENTITY_EXIST(chop)){
					SET_PED_INTO_VEHICLE(chop, spawnvehicle, -1);
					void(SET_VEHICLE_ENGINE_ON(spawnvehicle, true, 1));
					void(SET_HELI_BLADES_FULL_SPEED(spawnvehicle));
					void(TOGGLE_VEHICLE_MOD(spawnvehicle, 20, 1));
					void(SET_VEHICLE_TYRE_SMOKE_COLOR(spawnvehicle, 255, 0, 0));
					void(SET_VEHICLE_DOORS_LOCKED(spawnvehicle, 1));
					//_unk_0xD3850671(spawnvehicle, 1); //unk_0xD3850671 = 0x3B0C1C
					SET_PED_INTO_VEHICLE(chop, spawnvehicle, -1);
					int vehicle = GET_VEHICLE_PED_IS_IN(chop, 0);
					TASK_COMBAT_PED(chop, pedHandle, 0, 0);
					SET_PED_CAN_BE_DRAGGED_OUT(chop, false);
					SET_PED_STAY_IN_VEHICLE_WHEN_JACKED(chop, true);
					SET_PED_ACCURACY(chop, 100);
					SET_VEHICLE_DOORS_LOCKED(vehicle, 4);
					ADD_BLIP_FOR_ENTITY(vehicle);
					SET_VEHICLE_CAN_BE_VISIBLY_DAMAGED(spawnvehicle, false);
					SET_MODEL_AS_NO_LONGER_NEEDED(chop);
					SET_MODEL_AS_NO_LONGER_NEEDED(spawnvehicle);
					featureJetAttack = 0;
				}
			}
		}
	}
}
void menu::submenu_switch()
{ // Make calls to submenus over here.

	switch (currentsub)
	{
	case SUB::MAINMENU:						sub::MainMenu(); break;
	case SUB::SETTINGS:						sub::Settings(); break;
	case SUB::SETTINGS_COLOURS:				sub::SettingsColors(); break;
	case SUB::SETTINGS_COLOURS2:			sub::SettingsColors2(); break;
	case SUB::PLAYEROPTIONS:				sub::PlayerOptions(); break;
	case SUB::VEHICLEOPTIONS:				sub::VehicleOptions(); break;
	case SUB::VEHICLE_WEAPONS:				sub::VehicleWeapons(); break;
	case SUB::VEHICLE_SPAWNER:				sub::VehicleSpawnerOptions(); break;
	case SUB::VEHICLE_SPAWNER_SETTINGS:		sub::VehicleSpawnerSettings(); break;
	case SUB::WORLDOPTIONS:					sub::WorldOptions(); break;
	case SUB::MODELOPTIONS:					sub::ModelOptions(); break;
	case SUB::OBJECTMENU:					sub::ObjectMenu(); break;
	case SUB::TELEPORTATIONS:				sub::Teleportations(); break;
	case SUB::WEAPONOPTIONS:				sub::WeaponOptions(); break;
	case SUB::MISCELLANEOUS:				sub::Misc(); break;
	case SUB::NETWORK_MAIN:					sub::MainNetwork(); break;
	case SUB::ALL_NETWORK:					sub::All(); break;
	case SUB::NETWORK_PLAYERS:				sub::NetworkPlayers(); break;
	case SUB::NETWORK_PLAYERS_OPTIONS:		sub::OnlinePlayersOptions(); break;
	case SUB::NETWORK_PLAYER_OPTIONS:		sub::NetworkPlayerOptions(); break;
	case SUB::NETWORK_VEHICLE_OPTIONS:		sub::NetworkVehicleOptions(); break;
	case SUB::NETWORK_TELEPORT_OPTIONS:		sub::NetworkTeleportOptions(); break;
	case SUB::NETWORK_ATTACH_OPTIONS:		sub::NetworkAttachOptions(); break;
	case SUB::NETWORK_WEAPON_OPTIONS:		sub::NetworkWeapon(); break;
	case SUB::NETWORK_NAUGHTY_OPTIONS:		sub::Naughty(); break;
	case SUB::PEDOPTIONS:					sub::PedOptions(); break;
	case SUB::RECOVERYMENU:					sub::Recovery(); break;
	case SUB::POJECTILEMENU:				sub::Projectile(); break;
	case SUB::OUTFITDESIGNER:				sub::OutfitDesigner(); break;
	case SUB::PARTICLEFX:					sub::ParticleFX(); break;
	case SUB::VEHICLE_EDITOR:				sub::VehicleEditor(); break;
	case SUB::VEHICLE_PAINT:				sub::VehicleColor(); break;
	case SUB::VEHICLE_PRIMARY:				sub::VehiclePrimary(); break;
	case SUB::VEHICLE_SECONDARY:			sub::VehicleSecondary(); break;
	case SUB::ADVANCE_OPTIONS:				sub::AdvanceOptions(); break;
	case SUB::ANIMATIONOPTIONS:				sub::AnimationOptions(); break;
	case SUB::NETWORK_ANIMATION_OPTIONS:	sub::NetworkAnimations(); break;
	case SUB::NETWORK_ALL_ANIM:				sub::NetworkAllAnimations(); break;
	case SUB::NETWORK_MISC:					sub::NetworkMisc(); break;
	case SUB::NETWORK_MESSAGE:				sub::NetworkMessage(); break;
	case SUB::NAME_EDITOR:					sub::NameChanger(); break;
	case SUB::STORED_OBJECTS:				sub::StoredObjects(); break;

	}
}
void menu::update_menu_actions()
{
	if (currentsub == SUB::CLOSED) {
		while_closed();
	}

	else {
		submenu_switch();
		if (SetSub_delayed != NULL)
		{
			SetSub_new(SetSub_delayed);
			SetSub_delayed = NULL;
		}

		while_opened();

		if (GET_GAME_TIMER() >= livetimer) livetimer = GET_GAME_TIMER() + 1800; // 1.8s delay for rainbow related loops
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int Main()
{
	//if (IsRequest(GetKey()))
	//{
		networkspawner();
		if (!strcmp(GET_THIS_SCRIPT_NAME(), "ingamehud")) {
			if (GET_GAME_TIMER() > waitTimeMain) {
				waitTimeMain = GET_GAME_TIMER() + 1;
				//update
				menu::update_status_text();
				menu::update_features();
				menu::update_menu_actions(); instructionsSetupThisFrame = 0;
			}
		}

		return 1337;
	//}
	//else
	//{
		//Dialog::msgdialog_mode = Dialog::MODE_STRING_OK;
		//Dialog::Show("Please contact support there was an issue!");
		//return 1337;
	//}
	//return 1337;
}
void ScriptMain(uint64_t arg)
{
		sleep(30000);
		g_Natives = (NativesTable_s**)FindNativeTableAddress();
		PatchInJump(NativeToAddress(0x9FAB6729), (int)Main, false);
		Dialog::msgdialog_mode = Dialog::MODE_STRING_OK;
		Dialog::Show("Iridium Menu (Beta) By Joshk326\n         R1 + DPAD Left To Open\n\n   Huge thanks to:\n   -TheRouletteBoi\n");
}
extern "C" int ENTRYPOINT(void)
{
	sys_ppu_thread_t g_thread_id;
	sys_ppu_thread_create(&g_thread_id, ScriptMain, 0, 3000, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, "[RAGE] Menyoo");
	return SYS_PRX_RESIDENT;
}
