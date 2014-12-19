
#include <windows.h>
#include <mmsystem.h>

extern "C" int playsound()
{
	return PlaySoundW(L"TWEET", GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
}
