#include "DirectOverlay.h"
#include "D3XD/d3dx9math.h"
#include "utils.h"
#include "driver.h"
#define M_PI 3.14159265358979323846264338327950288419716939937510

#define OFFSET_UWORLD 0xc640158 //48 8b 0d ? ? ? ? 48 85 c9 74 30 e8 ? ? ? ? 48 8b f8

bool Menu = true;
bool Aimbot = true;
bool EnemyESP = true;
bool BoxESP = true;
bool LineESP = true;
bool DistanceESP = true;

DWORD processID;
HWND hwnd = NULL;

int width;
int height;
int localplayerID;


uint64_t base_address;

DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Ulevel;

FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray = read<DWORD_PTR>(mesh + 0x590);
	if (bonearray == NULL) {
		bonearray = read<DWORD_PTR>(mesh + 0x590 + 0x10);
	}
	return read<FTransform>(bonearray + (index * 0x60));
}

Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = read<FTransform>(mesh + 0x240);

	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());

	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	double radPitch = (rot.x * float(M_PI) / 180.f);
	double radYaw = (rot.y * float(M_PI) / 180.f);
	double radRoll = (rot.z * float(M_PI) / 180.f);

	double SP = sinf(radPitch);
	double CP = cosf(radPitch);
	double SY = sinf(radYaw);
	double CY = cosf(radYaw);
	double SR = sinf(radRoll);
	double CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}

struct CamewaDescwipsion
{
	float FieldOfView;
	Vector3 Rotation;
	Vector3 Location;
};

CamewaDescwipsion UndetectedCamera(__int64 a1)
{
	CamewaDescwipsion VirtualCamera;
	__int64 v1;
	__int64 v6;
	__int64 v7;
	__int64 v8;

	v1 = read<__int64>(Localplayer + 0xC8);
	__int64 v9 = read<__int64>(v1 + 8);

	VirtualCamera.FieldOfView = 80.f / (read<double>(v9 + 0x690) / 1.19f);

	VirtualCamera.Rotation.x = read<double>(v9 + 0x7E0);
	VirtualCamera.Rotation.y = read<double>(a1 + 0x148);

	v6 = read<__int64>(Localplayer + 0x70);
	v7 = read<__int64>(v6 + 0x98);
	v8 = read<__int64>(v7 + 0x180);

	VirtualCamera.Location = read<Vector3>(v8 + 0x20);
	return VirtualCamera;
}
float Width = GetSystemMetrics(SM_CXSCREEN), Height = GetSystemMetrics(SM_CYSCREEN);
Vector3 ProjectWorldToScreen(Vector3 WorldLocation)
{
	CamewaDescwipsion vCamera = UndetectedCamera(Rootcomp);
	vCamera.Rotation.x = (asin(vCamera.Rotation.x)) * (180.0 / M_PI);

	D3DMATRIX tempMatrix = Matrix(vCamera.Rotation);

	Vector3 vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	Vector3 vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	Vector3 vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	Vector3 vDelta = WorldLocation - vCamera.Location;
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	return Vector3((Width / 2.0f) + vTransformed.x * (((Width / 2.0f) / tanf(vCamera.FieldOfView * (float)M_PI / 360.f))) / vTransformed.z, (Height / 2.0f) - vTransformed.y * (((Height / 2.0f) / tanf(vCamera.FieldOfView * (float)M_PI / 360.f))) / vTransformed.z, 0);
}

void menu()
{
	if (Menu)
	{
		DrawBox(5, 10, 150, 120, 0.0f, 0.f, 0.f, 0.f, 100, true);
        DrawString(_xor_("ScreamWare - Public").c_str(), 17, 10, 8, 255.f, 255.f, 255.f, 255.f);

		if (EnemyESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 110, 10 + 20, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 110, 10 + 20, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F1] Enemy ESP >>").c_str(), 13, 10, 10 + 20, 255.f, 255.f, 255.f, 255.f);

		if (BoxESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 93, 10 + 40, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 93, 10 + 40, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F2] Box ESP >>").c_str(), 13, 10, 10 + 40, 255.f, 255.f, 255.f, 255.f);

		if (LineESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 95, 10 + 60, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 95, 10 + 60, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F3] Line ESP >>").c_str(), 13, 10, 10 + 60, 255.f, 255.f, 255.f, 255.f);

		if (DistanceESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 120, 10 + 80, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 120, 10 + 80, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F4] Distance ESP >>").c_str(), 13, 10, 10 + 80, 255.f, 255.f, 255.f, 255.f);

		if (Aimbot)
			DrawString(_xor_("ON").c_str(), 13, 10 + 90, 10 + 100, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 90, 10 + 100, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F5] Aimbot >>").c_str(), 13, 10, 10 + 100, 255.f, 255.f, 255.f, 255.f);
	}
}

DWORD Menuthread(LPVOID in)
{
	while (1)
	{
		if (GetAsyncKeyState(VK_INSERT) & 1) {
			Menu = !Menu;
		}

		if (Menu)
		{
			if (GetAsyncKeyState(VK_F1) & 1) {
				EnemyESP = !EnemyESP;
			}

			if (GetAsyncKeyState(VK_F2) & 1) {
				BoxESP = !BoxESP;
			}

			if (GetAsyncKeyState(VK_F3) & 1) {
				LineESP = !LineESP;
			}

			if (GetAsyncKeyState(VK_F4) & 1) {
				DistanceESP = !DistanceESP;
			}

			if (GetAsyncKeyState(VK_F5) & 1) {
				Aimbot = !Aimbot;
			}
		}
	}
}

void drawLoop(int width, int height) {
	menu();

	Uworld = read<DWORD_PTR>(base_address + OFFSET_UWORLD);
	//printf(_xor_("Uworld: %p.\n").c_str(), Uworld);

	DWORD_PTR Gameinstance = read<DWORD_PTR>(Uworld + 0x1a8);

	if (Gameinstance == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Gameinstance: %p.\n").c_str(), Gameinstance);

	DWORD_PTR LocalPlayers = read<DWORD_PTR>(Gameinstance + 0x38);

	if (LocalPlayers == (DWORD_PTR)nullptr)
		return;

//	printf(_xor_("LocalPlayers: %p.\n").c_str(), LocalPlayers);

	Localplayer = read<DWORD_PTR>(LocalPlayers);

	if (Localplayer == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayer: %p.\n").c_str(), Localplayer);

	PlayerController = read<DWORD_PTR>(Localplayer + 0x30);

	if (PlayerController == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("playercontroller: %p.\n").c_str(), PlayerController);

	LocalPawn = read<uint64_t>(PlayerController + 0x310);

	if (LocalPawn == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Pawn: %p.\n").c_str(), LocalPawn);

	Rootcomp = read<uint64_t>(LocalPawn + 0x188);

	if (Rootcomp == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Rootcomp: %p.\n").c_str(), Rootcomp);

	if (LocalPawn != 0) {
		localplayerID = read<int>(LocalPawn + 0x18);
		//std::cout << _xor_("localplayerID = ").c_str() << localplayerID << std::endl;
	}

	Ulevel = read<DWORD_PTR>(Uworld + 0x30);
	//printf(_xor_("Ulevel: %p.\n").c_str(), Ulevel);

	if (Ulevel == (DWORD_PTR)nullptr)
		return;

	DWORD ActorCount = read<DWORD>(Ulevel + 0xA0);
	//printf(_xor_("ActorCount: %p.\n").c_str(), ActorCount);

	DWORD_PTR AActors = read<DWORD_PTR>(Ulevel + 0x98);
	//printf(_xor_("AActors: %p.\n").c_str(), AActors);

	if (AActors == (DWORD_PTR)nullptr)
		return;

	for (int i = 0; i < ActorCount; i++)
	{
		uint64_t CurrentActor = read<uint64_t>(AActors + i * 0x8);

		if (CurrentActor == (uint64_t)nullptr || CurrentActor == -1 || CurrentActor == NULL)
			continue;

		int curactorid = read<int>(CurrentActor + 0x18);

		if (read<float>(CurrentActor + 0x3f60) == 10)
		{
			uint64_t CurrentActorRootComponent = read<uint64_t>(CurrentActor + 0x188);

			if (CurrentActorRootComponent == (uint64_t)nullptr || CurrentActorRootComponent == -1 || CurrentActorRootComponent == NULL)
				continue;

			uint64_t currentactormesh = read<uint64_t>(CurrentActor + 0x2F0);

			if (currentactormesh == (uint64_t)nullptr || currentactormesh == -1 || currentactormesh == NULL)
				continue;

			Vector3 Headpos = GetBoneWithRotation(currentactormesh, 98);
			
			Vector3 localactorpos = read<Vector3>(Rootcomp + 0x128);

			float distance = localactorpos.Distance(Headpos) / 100.f;

			if (distance < 1.5f)
				continue;

			//W2S
			Vector3 HeadposW2s = ProjectWorldToScreen(Headpos);
			Vector3 bone0 = GetBoneWithRotation(currentactormesh, 0);
			Vector3 bottom = ProjectWorldToScreen(bone0);
			Vector3 Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15));
			std::cout << Headbox.x << "\n";

			float Height1 = abs(Headbox.y - bottom.y);
			float Width1 = Height1 * 0.65;

			if (BoxESP)
				DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 255.f, 255.f, 0.f, 0.f, 200.f, false);

			if (EnemyESP)
				DrawString(_xor_("Enemy").c_str(), 13, HeadposW2s.x, HeadposW2s.y - 25, 0, 1, 1);

			if (DistanceESP)
			{
				CHAR dist[50];
				sprintf_s(dist, _xor_("[%.f]").c_str(), distance);

				DrawString(dist, 13, HeadposW2s.x + 40, HeadposW2s.y - 25, 0, 1, 1);
			}

			if (LineESP)
				DrawLine(width / 2, height, bottom.x, bottom.y, 2.f, 255.f, 0.f, 0.f, 200.f);

			if (Aimbot)
			{
				float headX = HeadposW2s.x - width / 2;
				float headY = HeadposW2s.y - height / 2;

				if (headX >= -100 && headX <= 100 && headY >= -100 && headY <= 100 && distance < 350) {

					if (GetAsyncKeyState(VK_XBUTTON1)) {
						mouse_event(MOUSEEVENTF_MOVE, headX, headY, NULL, NULL);
					}
				}
			}
		}
	}
	Sleep(2);
}

void main()
{
	
	printf(("[!] Open driver connection: "));
	if (driver->Init(FALSE)) {
		printf(("Success!\n"));
		driver->Attach((L"FortniteClient-Win64-Shipping.exe"));
		while (true)
		{
			while (hwnd == NULL)
			{
				hwnd = FindWindowA(0, _xor_("Fortnite  ").c_str());

				printf(_xor_("Looking for process...\n").c_str());
				Sleep(10);
			}

			
			processID = driver->GetProcessId((L"FortniteClient-Win64-Shipping.exe"));
			RECT rect;
			if (GetWindowRect(hwnd, &rect))
			{
				width = rect.right - rect.left;
				height = rect.bottom - rect.top;
			}

			

		

			
			
			

			base_address = driver->GetModuleBase((L"FortniteClient-Win64-Shipping.exe"));

			std::printf(_xor_("Process base address: %p.\n").c_str(), (void*)base_address);

			CreateThread(NULL, NULL, Menuthread, NULL, NULL, NULL);

			DirectOverlaySetOption(D2DOV_DRAW_FPS | D2DOV_FONT_COURIER);
			DirectOverlaySetup(drawLoop, FindWindow(NULL, _xor_(L"Fortnite  ").c_str()));
			getchar();
		}
		return;
	}
	printf(("Failed!\n"));
	system(("pause"));
	return;
}
