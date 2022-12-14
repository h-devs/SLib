#include <slib.h>
#include <windows.h>

using namespace slib;
using namespace std;

sl_uint32 CalcHashValue(sl_uint8* plain, int len)
{
	//plain[i] < 0x7F because plain consists of capital or digital
	//hash[0..1] is 14bit >> 1 becomes 0 ~ 8192
	sl_uint8 hash[4] = { 0 };
	for (int i = 0; i < len; i++)
	{
		hash[1] ^= hash[0];
		hash[0] ^= hash[3];
		hash[3] ^= hash[2];
		hash[2] = plain[i] ^ hash[1];
		i++;
	}
	*(sl_uint16*)&hash[0] = (hash[0] | (hash[1] << 7)) >> 1;
	*(sl_uint16*)&hash[2] = (hash[2] | (hash[3] << 7)) >> 1;

	return *(sl_uint32*)hash;
}

int main(int argc, const char * argv[])
{
	String diskSN = Disk::getSerialNumber(0);
	sl_uint32 diskHash = CalcHashValue((sl_uint8*)diskSN.getData(), diskSN.getLength());

	sl_uint16 s1 = *(sl_uint16*)&diskHash;
	sl_uint16 s2 = *((sl_uint16*)&diskHash + 1);
	
	String boardSN = Device::getBoardSerialNumber();
	sl_uint32 boardHash = CalcHashValue((sl_uint8*)boardSN.getData(), boardSN.getLength());

	sl_uint16 s3 = *(sl_uint16*)&boardHash;
	sl_uint16 s4 = *((sl_uint16*)&boardHash + 1);
	sl_uint16 checksum = s1 ^ s2 ^ s3 ^ s4;
	//RemoveSpecialChar(diskSN);

	Println("DiskSN: %s", diskSN);
	Println("BoardSN: %s", boardSN);
	Println("MachineSN: %04d-%04d-%04d-%04d-%04d", s1, s2, s3, s4, checksum);
	
	while (1);
	return 0;
}
